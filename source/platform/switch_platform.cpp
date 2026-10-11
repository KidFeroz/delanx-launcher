// Delanx — implementacion de la capa de plataforma (libnx).
#include "platform/switch_platform.hpp"

#include <switch.h>

#include <malloc.h>
#include <sys/stat.h>

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "core/paths.hpp"
#include "core/strings.hpp"
#include "core/time.hpp"

namespace delanx::platform {
namespace {

bool g_sm_ready = false;
bool g_psm_ready = false;
bool g_nifm_ready = false;
bool g_setsys_ready = false;

// --- log --------------------------------------------------------------------

// Tope del fichero de log: al superarlo se conserva solo la mitad final.
constexpr size_t kMaxLogBytes = 256u * 1024u;

void append_log_line(const char* text) {
  const std::string path = paths::log_file();
  std::string content;
  if (paths::exists(path)) {
    const std::string current = paths::read_file(path).value_or(std::string());
    if (current.size() > kMaxLogBytes) {
      content = current.substr(current.size() - kMaxLogBytes / 2u);
      content.insert(0, "[log recortado]\n");
    } else {
      content = current;
    }
  } else {
    paths::make_dirs(paths::dirname(path));
  }
  content += text;
  paths::write_file(path, content);
}

// --- romfs ------------------------------------------------------------------

bool g_romfs_tried = false;
bool g_romfs_ready = false;

bool ensure_romfs() {
  if (!g_romfs_tried) {
    g_romfs_tried = true;
    // libnx suele montar el romfs propio en __appInit; si no, se monta aqui.
    g_romfs_ready = R_SUCCEEDED(romfsInit()) || paths::exists("romfs:/");
  }
  return g_romfs_ready;
}

std::string romfs_path(const char* path) {
  if (path == nullptr) return std::string();
  std::string value(path);
  if (str::starts_with(value, "romfs:/")) return value;
  if (!value.empty() && value[0] == '/') return std::string("romfs:") + value;
  return "romfs:/" + value;
}

// --- heap -------------------------------------------------------------------

// uordblks = memoria realmente en uso (lo que ve el usuario como "consumido"),
// arena = memoria total pedida al sistema por el asignador.
uint64_t heap_used_bytes() {
  const struct mallinfo info = mallinfo();
  return static_cast<uint64_t>(info.uordblks);
}

uint64_t heap_total_bytes() {
  const struct mallinfo info = mallinfo();
  uint64_t total = static_cast<uint64_t>(info.arena);
  if (total == 0) {
    // Sin datos del asignador: el limite del proceso sirve como referencia.
    u64 limit = 0;
    if (R_SUCCEEDED(svcGetInfo(&limit, InfoType_TotalMemorySize, CUR_PROCESS_HANDLE, 0)) && limit > 0) {
      total = static_cast<uint64_t>(limit);
    }
  }
  return total;
}

// "12,4" a partir de bytes con un decimal y coma decimal (estilo espanol).
std::string one_decimal_mb(uint64_t bytes) {
  const uint64_t tenths = (bytes * 10u + (512u * 1024u)) / (1024u * 1024u);
  char buf[32];
  snprintf(buf, sizeof(buf), "%llu,%llu", static_cast<unsigned long long>(tenths / 10u),
           static_cast<unsigned long long>(tenths % 10u));
  return std::string(buf);
}

// --- arranque ---------------------------------------------------------------

std::string detect_boot_origin() {
  if (is_applet_mode()) return "album";

  if (std::getenv("DELANX_LAUNCHER") != nullptr) return "forwarder";
  // Los forwarders NSP de hbloader inyectan /nextNroPath y /nextArgv en su romfs.
  if (romfs_exists("nextNroPath") || romfs_exists("nextArgv")) return "forwarder";

  const std::vector<std::string> args = launch_args();
  if (!args.empty()) {
    const std::string origin = str::lower(args[0]);
    if (str::icontains(origin, "sphaira")) return "sphaira";
  }

  if (envHasNextLoad()) return "hbmenu";
  return "unknown";
}

}  // namespace

bool init() {
  if (!g_sm_ready) {
    g_sm_ready = R_SUCCEEDED(smInitialize());
  }

  if (!g_psm_ready) {
    // la bateria solo esta disponible si el proceso puede abrir psm.
    g_psm_ready = R_SUCCEEDED(psmInitialize());
  }
  if (!g_nifm_ready) {
    // nifm:s es el que expone el estado de conexion; si falla, se prueba nifm:u.
    g_nifm_ready = R_SUCCEEDED(nifmInitialize(NifmServiceType_System)) ||
                   R_SUCCEEDED(nifmInitialize(NifmServiceType_User));
  }
  if (!g_setsys_ready) {
    g_setsys_ready = R_SUCCEEDED(setsysInitialize());
  }

  // Carpetas de trabajo: sin ellas no hay log ni staging.
  paths::ensure_layout();
  platform::log("platform: init (psm=%d nifm=%d setsys=%d)", static_cast<int>(g_psm_ready),
                static_cast<int>(g_nifm_ready), static_cast<int>(g_setsys_ready));
  return true;
}

void shutdown() {
  if (g_setsys_ready) {
    setsysExit();
    g_setsys_ready = false;
  }
  if (g_nifm_ready) {
    nifmExit();
    g_nifm_ready = false;
  }
  if (g_psm_ready) {
    psmExit();
    g_psm_ready = false;
  }
  if (g_sm_ready) {
    smExit();
    g_sm_ready = false;
  }
  append_log_line("[fin de sesion]\n");
}

bool is_applet_mode() { return appletGetAppletType() == AppletType_LibraryApplet; }

bool is_application_mode() {
  const AppletType type = appletGetAppletType();
  return type == AppletType_Application || type == AppletType_SystemApplication;
}

int battery_percent() {
  if (!g_psm_ready) return -1;
  u32 percent = 0;
  if (R_FAILED(psmGetBatteryChargePercentage(&percent))) return -1;
  if (percent > 100) percent = 100;
  return static_cast<int>(percent);
}

bool charging() {
  if (!g_psm_ready) return false;
  PsmChargerType charger = PsmChargerType_Unconnected;
  if (R_FAILED(psmGetChargerType(&charger))) return false;
  return charger != PsmChargerType_Unconnected;
}

bool network_connected() {
  if (!g_nifm_ready) return false;
  NifmInternetConnectionType type = NifmInternetConnectionType_WiFi;
  u32 strength = 0;
  NifmInternetConnectionStatus status = NifmInternetConnectionStatus_ConnectingUnknown1;
  if (R_FAILED(nifmGetInternetConnectionStatus(&type, &strength, &status))) return false;
  return status == NifmInternetConnectionStatus_Connected;
}

std::string firmware_version() {
  if (!g_setsys_ready) return std::string();
  SetSysFirmwareVersion version;
  memset(&version, 0, sizeof(version));
  if (R_FAILED(setsysGetFirmwareVersion(&version))) return std::string();

  // display_version es lo que muestra el sistema ("18.1.0"); si viene vacio se
  // reconstruye desde los campos numericos.
  if (version.display_version[0] != '\0') {
    char buf[sizeof(version.display_version) + 1];
    memset(buf, 0, sizeof(buf));
    memcpy(buf, version.display_version, sizeof(version.display_version));
    return std::string(buf);
  }
  char buf[32];
  snprintf(buf, sizeof(buf), "%u.%u.%u", version.major, version.minor, version.micro);
  return std::string(buf);
}

std::string device_nickname() {
  if (!g_setsys_ready) return std::string();
  SetSysDeviceNickName nickname;
  memset(&nickname, 0, sizeof(nickname));
  if (R_FAILED(setsysGetDeviceNickname(&nickname))) return std::string();
  char buf[sizeof(nickname.nickname) + 1];
  memset(buf, 0, sizeof(buf));
  memcpy(buf, nickname.nickname, sizeof(nickname.nickname));
  return std::string(buf);
}

std::string boot_origin() { return detect_boot_origin(); }

namespace {
std::vector<std::string>& launch_args_store() {
  static std::vector<std::string> store;
  return store;
}
}  // namespace

void set_launch_args(int argc, char** argv) {
  std::vector<std::string>& store = launch_args_store();
  store.clear();
  for (int index = 0; index < argc; ++index) {
    if (argv[index] != nullptr) store.emplace_back(argv[index]);
  }
}

std::vector<std::string> launch_args() { return launch_args_store(); }

std::string read_romfs_text(const char* path) {
  if (path == nullptr || path[0] == '\0') return std::string();
  if (!ensure_romfs()) return std::string();
  const std::string full = romfs_path(path);
  if (!paths::exists(full)) return std::string();
  return paths::read_file(full).value_or(std::string());
}

bool romfs_exists(const char* path) {
  if (path == nullptr || path[0] == '\0') return false;
  if (!ensure_romfs()) return false;
  return paths::exists(romfs_path(path));
}

void log(const char* fmt, ...) {
  if (fmt == nullptr) return;

  char body[768];
  va_list args;
  va_start(args, fmt);
  vsnprintf(body, sizeof(body), fmt, args);
  va_end(args);

  char line[896];
  const std::string stamp = clock_hhmm_local();
  snprintf(line, sizeof(line), "[%s] %s\n", stamp.c_str(), body);

  // El log nunca debe tumbar la aplicacion: si no se puede escribir, se ignora.
  append_log_line(line);
}

std::string heap_usage_human() {
  const uint64_t used = heap_used_bytes();
  const uint64_t total = heap_total_bytes();
  if (total == 0) return one_decimal_mb(used) + " MB en uso";
  return one_decimal_mb(used) + " / " + one_decimal_mb(total) + " MB";
}

// --- entrada directa por HID de libnx ---

namespace {

PadState& pad_state() {
  static PadState pad;
  static bool ready = false;
  if (!ready) {
    hidInitialize();  // idempotente; asegura el servicio antes de leer el pad
    padInitializeDefault(&pad);
    ready = true;
  }
  return pad;
}

}  // namespace

bool pad_poll(PadSnapshot* out) {
  if (out == nullptr) return false;
  PadState& pad = pad_state();
  padUpdate(&pad);
  out->down = padGetButtons(&pad);
  out->pressed = padGetButtonsDown(&pad);
  out->released = padGetButtonsUp(&pad);
  const HidAnalogStickState left = padGetStickPos(&pad, 0);
  const HidAnalogStickState right = padGetStickPos(&pad, 1);
  out->stick_lx = left.x;
  out->stick_ly = left.y;
  out->stick_rx = right.x;
  out->stick_ry = right.y;
  return true;
}

bool touch_poll(TouchSnapshot* out) {
  if (out == nullptr) return false;
  static bool ready = false;
  if (!ready) {
    // Ojo: hidInitializeTouchScreen() aborta el proceso si el servicio falla (no hay API con
    // Result), asi que el tactil no puede degradarse solo; se registra para diagnostico.
    hidInitializeTouchScreen();
    ready = true;
    log("input: pantalla tactil inicializada");
  }
  HidTouchScreenState state = {};
  const size_t count = hidGetTouchScreenStates(&state, 1);
  if (count > 0 && state.count > 0) {
    out->active = true;
    out->x = static_cast<int>(state.touches[0].x);
    out->y = static_cast<int>(state.touches[0].y);
  } else {
    // Sin dedo encima. Es importante devolver true igualmente: si aqui se devolviera false,
    // el llamante no podria distinguir "no hay toque" de "no se pudo consultar" y el boton
    // tactil se quedaria pulsado para siempre (el tactil moriria tras el primer toque).
    out->active = false;
  }
  return true;
}

}  // namespace delanx::platform

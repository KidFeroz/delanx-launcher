// Delanx — capa de plataforma para Horizon OS (libnx).
// Aqui vive todo lo que depende del hardware y del entorno de arranque: modo
// applet/aplicacion, bateria, red, version de firmware, argumentos y log.
#pragma once

#include <string>
#include <vector>

namespace delanx::platform {

// Inicializa los servicios de libnx que Delanx necesita (PSM, NIFM, setsys, sm).
// NO inicializa sockets: eso es responsabilidad de net::global_init(). Tampoco
// toca nada de SDL. Es seguro llamarla mas de una vez.
bool init();

// Cierra los servicios abiertos por init(). Seguro sin init() previo.
void shutdown();

bool is_applet_mode();       // appletGetAppletType() == AppletType_LibraryApplet
bool is_application_mode();

int battery_percent();       // -1 si no disponible
bool charging();
bool network_connected();    // nifmGetInternetConnectionStatus

std::string firmware_version();
std::string device_nickname();

// "hbmenu" | "sphaira" | "forwarder" | "album" | "unknown"
std::string boot_origin();

// Argumentos con los que nos lanzo el loader (argv[0] incluido).
std::vector<std::string> launch_args();

// Los guarda main() al arrancar: libnx no expone argv fuera de main de forma portable.
void set_launch_args(int argc, char** argv);

// Lee un fichero de romfs por ruta relativa ("catalog/x.json") o con prefijo
// ("romfs:/catalog/x.json"). Devuelve "" si no existe.
std::string read_romfs_text(const char* path);
bool romfs_exists(const char* path);

// Escribe una linea en sdmc:/switch/Delanx/delanx.log. Segura antes de init():
// en ese caso solo formatea y lo intenta al primer uso de la ruta.
void log(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

// "12,4 / 39,0 MB" para la pantalla de ajustes/diagnostico.
std::string heap_usage_human();

// --- entrada directa por HID de libnx ---
// SDL2 en Switch no expone los mandos como SDL_GameController, asi que la interfaz lee el
// pad y la pantalla tactil directamente de libnx. `down/pressed/released` usan la mascara
// de HidNpadButton; los sticks van de -32767 a 32767.
struct PadSnapshot {
  uint64_t down = 0;
  uint64_t pressed = 0;
  uint64_t released = 0;
  int stick_lx = 0;
  int stick_ly = 0;
  int stick_rx = 0;
  int stick_ry = 0;
};
bool pad_poll(PadSnapshot* out);

struct TouchSnapshot {
  bool active = false;
  int x = 0;
  int y = 0;
};
bool touch_poll(TouchSnapshot* out);

}  // namespace delanx::platform

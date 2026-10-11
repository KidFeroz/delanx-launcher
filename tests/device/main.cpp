// Delanx — autotest de dispositivo (se ejecuta EN la Switch, sin red).
//
// Comprueba en hardware lo que no se puede verificar desde el PC: vectores SHA-256 reales del
// toolchain, parser ISO-8601, extraccion de archivos con libarchive (incluido rechazo de
// path traversal y 7z), busqueda del .nro lanzable, metadatos de instalacion compatibles con
// Quiver, conservacion de los ficheros que aporta el usuario, y la capa de plataforma
// (applet mode, bateria, firmware, origen de arranque).
//
// Al terminar escribe el informe en sdmc:/switch/Delanx/instcheck-report.txt.
// Compilar y enviar:  make -C tests/device && nxlink -s tests/device/DelanxSelfTest.nro
#include <switch.h>
#include <switch/runtime/pad.h>

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "core/paths.hpp"
#include "core/sha256.hpp"
#include "core/strings.hpp"
#include "core/time.hpp"
#include "model/app_entry.hpp"
#include "model/catalog.hpp"
#include "platform/switch_platform.hpp"
#include "services/installer.hpp"
#include "services/launcher.hpp"

namespace {

int g_pass = 0;
int g_fail = 0;
std::string g_log;

void emit(const std::string& line) {
  printf("%s\n", line.c_str());
  consoleUpdate(nullptr);
  g_log += line;
  g_log += "\n";
}

void check(const char* name, bool condition, const std::string& detail) {
  if (condition) {
    ++g_pass;
  } else {
    ++g_fail;
  }
  emit(std::string(condition ? "[ OK ] " : "[FAIL] ") + name + " -> " + detail);
}

bool file_text_is(const std::string& path, const std::string& expected) {
  const std::string actual = delanx::paths::read_file(path).value_or(std::string("<no existe>"));
  if (actual == expected) return true;
  emit("       esperado: " + expected + " / real: " + actual);
  return false;
}

}  // namespace

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;

  consoleInit(nullptr);
  printf("instcheck - verificacion de Delanx (sha256, time, installer, launcher, platform)\n");
  consoleUpdate(nullptr);

  PadState pad;
  padInitializeDefault(&pad);

  delanx::platform::init();

  // ---------------------------------------------------------------- sha256 ---
  const std::string empty_hash =
      delanx::sha256_hex(static_cast<const void*>(""), static_cast<size_t>(0));
  check("sha256(\"\")", empty_hash == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
        empty_hash);

  const std::string abc_hash = delanx::sha256_hex(std::string("abc"));
  check("sha256(\"abc\")", abc_hash == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
        abc_hash);

  // Vectores extra de FIPS 180-4 para cubrir el padding de 2 bloques.
  const std::string long_hash = delanx::sha256_hex(
      std::string("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"));
  check("sha256(448-bit)",
        long_hash == "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1", long_hash);

  const std::string mb_hash = delanx::sha256_hex(std::string(1000000, 'a'));
  check("sha256(1e6 x 'a')",
        mb_hash == "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0", mb_hash);

  // sha256_file sobre un fichero conocido + progreso.
  delanx::paths::ensure_layout();
  const std::string probe = delanx::paths::join(delanx::paths::temp_dir(), "instcheck_abc.bin");
  delanx::paths::write_file(probe, "abc");
  int progress_calls = 0;
  const std::string file_hash = delanx::sha256_file(probe, [&progress_calls](int64_t, int64_t) {
    ++progress_calls;
    return true;
  });
  check("sha256_file(abc)", file_hash == abc_hash, file_hash);
  check("sha256_file llama a progress", progress_calls > 0,
        std::to_string(progress_calls) + " llamadas");
  const std::string missing_hash = delanx::sha256_file("sdmc:/no/existe.bin");
  check("sha256_file inexistente", missing_hash.empty(), "vacio");

  // ------------------------------------------------------------------ time ---
  const std::string iso = delanx::iso8601_utc(1773344823);
  check("iso8601_utc(1773344823)", iso == "2026-03-12T19:47:03Z", iso);
  check("parse_iso8601_utc vuelta", delanx::parse_iso8601_utc(iso) == 1773344823,
        std::to_string(delanx::parse_iso8601_utc(iso)));
  check("parse_iso8601_utc con desfase",
        delanx::parse_iso8601_utc("2026-03-12T21:47:03+02:00") == 1773344823,
        std::to_string(delanx::parse_iso8601_utc("2026-03-12T21:47:03+02:00")));
  check("parse_iso8601_utc con desfase negativo",
        delanx::parse_iso8601_utc("2026-03-24T06:23:45+01:00") == 1774329825,
        std::to_string(delanx::parse_iso8601_utc("2026-03-24T06:23:45+01:00")));
  check("parse_iso8601_utc solo fecha", delanx::parse_iso8601_utc("2026-03-12") == 1773273600,
        std::to_string(delanx::parse_iso8601_utc("2026-03-12")));
  check("parse_iso8601_utc invalido", delanx::parse_iso8601_utc("no-es-fecha") == 0, "0");
  check("parse_iso8601_utc vacio", delanx::parse_iso8601_utc("") == 0, "0");
  check("parse_iso8601_utc fecha imposible", delanx::parse_iso8601_utc("2026-02-30") == 0, "0");
  check("parse_iso8601_utc anio bisiesto", delanx::parse_iso8601_utc("2024-02-29") == 1709164800,
        std::to_string(delanx::parse_iso8601_utc("2024-02-29")));
  emit("       clock_hhmm_local() = " + delanx::clock_hhmm_local());
  check("date_short_local(1773344823)", delanx::date_short_local(1773344823) == "12 mar 2026",
        delanx::date_short_local(1773344823));
  check("now_unix razonable", delanx::now_unix() > 1700000000, std::to_string(delanx::now_unix()));

  // ------------------------------------------------------------ extractor ---
  const std::string staging = delanx::paths::join(delanx::paths::temp_dir(), "instcheck_zips");
  const std::string out_a = delanx::paths::join(delanx::paths::temp_dir(), "instcheck_out_a");
  delanx::paths::remove_dir_recursive(out_a);

  std::string error;
  const bool ok_a = delanx::extract_archive(delanx::paths::join(staging, "pack_root.zip"), out_a, true, &error);
  check("extract_archive(strip_root)", ok_a, error.empty() ? "sin error" : error);
  check("  raiz eliminada", delanx::paths::exists(delanx::paths::join(out_a, "soh.nro")),
        delanx::paths::join(out_a, "soh.nro"));
  check("  subcarpeta conservada", delanx::paths::exists(delanx::paths::join(out_a, "assets/data.txt")),
        delanx::paths::join(out_a, "assets/data.txt"));
  check("  contenido correcto", file_text_is(delanx::paths::join(out_a, "assets/data.txt"), "datos\n"), "datos");

  const std::string out_b = delanx::paths::join(delanx::paths::temp_dir(), "instcheck_out_b");
  delanx::paths::remove_dir_recursive(out_b);
  error.clear();
  const bool ok_b = delanx::extract_archive(delanx::paths::join(staging, "pack_root.zip"), out_b, false, &error);
  check("extract_archive(sin strip_root)", ok_b, error.empty() ? "sin error" : error);
  check("  raiz conservada", delanx::paths::exists(delanx::paths::join(out_b, "pack/soh.nro")),
        delanx::paths::join(out_b, "pack/soh.nro"));

  // Path traversal: evil/../../evil.txt debe salir fuera de dest_dir y rechazarse.
  const std::string out_c = delanx::paths::join(delanx::paths::temp_dir(), "instcheck_out_c");
  delanx::paths::remove_dir_recursive(out_c);
  const std::string escaped = delanx::paths::join(delanx::paths::temp_dir(), "evil.txt");
  delanx::paths::remove_file(escaped);
  error.clear();
  const bool ok_c = delanx::extract_archive(delanx::paths::join(staging, "traversal.zip"), out_c, false, &error);
  check("extract_archive detecta traversal", !ok_c, error.empty() ? "sin mensaje de error" : error);
  check("  no se escapo del destino", !delanx::paths::exists(escaped), escaped);
  check("  el fichero legitimo si se extrajo",
        delanx::paths::exists(delanx::paths::join(out_c, "legit.txt")),
        delanx::paths::join(out_c, "legit.txt"));

  // Zip con varios ficheros raiz: strip_root no debe quitar nada.
  const std::string out_d = delanx::paths::join(delanx::paths::temp_dir(), "instcheck_out_d");
  delanx::paths::remove_dir_recursive(out_d);
  error.clear();
  const bool ok_d = delanx::extract_archive(delanx::paths::join(staging, "multi_root.zip"), out_d, true, &error);
  check("extract_archive(multi-root, strip_root)", ok_d, error.empty() ? "sin error" : error);
  check("  no se perdio nada", delanx::paths::exists(delanx::paths::join(out_d, "a/one.txt")) &&
                                   delanx::paths::exists(delanx::paths::join(out_d, "b/two.txt")),
        "a/one.txt + b/two.txt");

  // 7z (mGBA publica .7z).
  const std::string out_e = delanx::paths::join(delanx::paths::temp_dir(), "instcheck_out_e");
  delanx::paths::remove_dir_recursive(out_e);
  error.clear();
  const bool ok_e = delanx::extract_archive(delanx::paths::join(staging, "pack.7z"), out_e, false, &error);
  check("extract_archive(7z)", ok_e, error.empty() ? "sin error" : error);

  // ------------------------------------------------------- nro lanzable -----
  const std::string nro_dir = delanx::paths::join(delanx::paths::temp_dir(), "instcheck_nro");
  delanx::paths::remove_dir_recursive(nro_dir);
  delanx::paths::make_dirs(delanx::paths::join(nro_dir, "sub"));
  delanx::paths::write_file(delanx::paths::join(nro_dir, "otro.nro"), "x");
  delanx::paths::write_file(delanx::paths::join(nro_dir, "sub/soh.nro"), "x");
  check("find_launchable_nro(preferido en subcarpeta)",
        delanx::find_launchable_nro(nro_dir, "soh.nro") == delanx::paths::join(nro_dir, "sub/soh.nro"),
        delanx::find_launchable_nro(nro_dir, "soh.nro"));
  check("find_launchable_nro(preferido inexistente -> primero)",
        delanx::find_launchable_nro(nro_dir, "nada.nro") == delanx::paths::join(nro_dir, "otro.nro"),
        delanx::find_launchable_nro(nro_dir, "nada.nro"));
  check("find_launchable_nro(vacio)", delanx::find_launchable_nro("", "x.nro").empty(), "vacio");

  // ------------------------------------------------------------- metadata ---
  delanx::AppEntry app;
  app.name = "SoH";
  app.folder_name = "instcheck_app";
  app.sw.target_dir = delanx::paths::join(delanx::paths::temp_dir(), "instcheck_app");
  delanx::paths::remove_dir_recursive(app.sw.target_dir);
  const bool metadata_ok = delanx::write_install_metadata(app, "v9.9.9", "soh.zip", abc_hash);
  check("write_install_metadata", metadata_ok, std::to_string(static_cast<int>(metadata_ok)));
  check("  version.txt", file_text_is(delanx::paths::join(app.sw.target_dir, "version.txt"), "v9.9.9\n"),
        "v9.9.9");
  check("  installed-build.txt",
        file_text_is(delanx::paths::join(app.sw.target_dir, "installed-build.txt"),
                     "soh.zip\n" + abc_hash + "\n"),
        "soh.zip + sha");

  // ------------------------------------------------ ficheros del usuario ----
  const std::string guard_dir = delanx::paths::join(delanx::paths::temp_dir(), "instcheck_guard");
  delanx::paths::remove_dir_recursive(guard_dir);
  delanx::paths::make_dirs(guard_dir);
  delanx::paths::write_file(delanx::paths::join(guard_dir, "oot.otr"), "rom del usuario");
  delanx::paths::write_file(delanx::paths::join(guard_dir, "viejo.nro"), "viejo");
  app.sw.target_dir = guard_dir;
  const bool removed = delanx::uninstall_app(app, false, &error);
  check("uninstall_app conserva .otr", removed && delanx::paths::exists(delanx::paths::join(guard_dir, "oot.otr")),
        error.empty() ? "sin error" : error);
  check("  borro el resto", !delanx::paths::exists(delanx::paths::join(guard_dir, "viejo.nro")),
        "viejo.nro fuera");
  const bool forced = delanx::uninstall_app(app, true, &error);
  check("uninstall_app(force)", forced && !delanx::paths::exists(guard_dir), error.empty() ? "sin error" : error);

  // ---------------------------------------------- espacio y plataforma ------
  check("free_space_bytes > 0", delanx::free_space_bytes() > 0,
        delanx::str::human_size(delanx::free_space_bytes()));
  emit("       boot_origin()          = " + delanx::platform::boot_origin());
  emit("       boot_origin_description = " + delanx::boot_origin_description());
  emit("       firmware               = " + delanx::platform::firmware_version());
  emit("       nickname               = " + delanx::platform::device_nickname());
  emit("       heap                   = " + delanx::platform::heap_usage_human());
  emit("       applet_mode            = " + std::string(delanx::platform::is_applet_mode() ? "si" : "no"));
  emit("       battery                = " + std::to_string(delanx::platform::battery_percent()) + "%");
  emit("       charging               = " + std::string(delanx::platform::charging() ? "si" : "no"));
  emit("       red                    = " + std::string(delanx::platform::network_connected() ? "si" : "no"));
  emit("       can_chain_nro          = " + std::string(delanx::can_chain_nro() ? "si" : "no"));
  const std::vector<std::string> args = delanx::platform::launch_args();
  emit("       launch_args            = " + std::to_string(args.size()) + " args");
  for (const std::string& arg : args) emit("         argv: " + arg);

  error.clear();
  const bool launch_ok = delanx::request_next_nro({probe, {}}, &error);
  check("request_next_nro(fichero no .nro)", !launch_ok, error);

  error.clear();
  delanx::LaunchTarget bad_target;
  check("request_next_nro(sin ruta)", !delanx::request_next_nro(bad_target, &error), error);

  // ------------------------------------------------------------- resumen ----
  const std::string summary = "instcheck: " + std::to_string(g_pass) + " OK, " + std::to_string(g_fail) + " FAIL";
  emit(summary);
  delanx::platform::log("%s", summary.c_str());

  const std::string report_path = delanx::paths::join(delanx::paths::root(), "instcheck-report.txt");
  delanx::paths::write_file(report_path, g_log);
  printf("\nInforme escrito en %s\n", report_path.c_str());
  printf("Pulsa + para salir.\n");
  consoleUpdate(nullptr);

  while (appletMainLoop()) {
    consoleUpdate(nullptr);
    padUpdate(&pad);
    if ((padGetButtonsDown(&pad) & HidNpadButton_Plus) != 0) break;
  }

  delanx::platform::shutdown();
  consoleExit(nullptr);
  return g_fail == 0 ? 0 : 1;
}

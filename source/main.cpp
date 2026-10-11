// Delanx — punto de entrada del NRO.
//
// Modos de arranque soportados:
//   * hbmenu            -> caso base; envSetNextLoad encadena el siguiente NRO al salir.
//   * Sphaira           -> igual que hbmenu.
//   * Forwarder NSP     -> hbloader inyecta /nextNroPath y respeta el encadenado.
//   * Album (applet)    -> memoria reducida: se avisa en la interfaz.
#include <cstdio>

#include <switch.h>

#include "core/paths.hpp"
#include "platform/switch_platform.hpp"
#include "ui/app.hpp"

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;  // los argumentos reales se leen con platform::launch_args()

  // romfs contiene el catalogo verificado y la fuente de la interfaz.
  romfsInit();
  delanx::platform::set_launch_args(argc, argv);

  delanx::ui::App app;
  if (!app.init()) {
    delanx::platform::log("Delanx: fallo al inicializar (SDL/servicios)");
    std::fprintf(stderr, "Delanx: fallo al inicializar\n");
    return 1;
  }

  const int exit_code = app.run();
  app.shutdown();

  // Con envSetNextLoad ya preparado, salir de main deja que el loader cargue el siguiente NRO.
  if (exit_code == 2) {
    delanx::platform::log("Delanx: encadenando el siguiente NRO y saliendo");
  }
  return exit_code;
}

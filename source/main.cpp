// Delanx — Homebrew Port & Decompilation Manager para Nintendo Switch
// Copyright (C) 2026 KidFeroz <https://github.com/KidFeroz>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.
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

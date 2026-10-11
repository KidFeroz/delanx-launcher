// Delanx — encadenado de NROs mediante envSetNextLoad().
// El patron (el que respetan hbmenu y Sphaira) es: escribir el siguiente NRO en los
// buffers del loader y salir del proceso; el loader recoge el testigo y lo carga.
#pragma once

#include <string>
#include <vector>

namespace delanx {

struct LaunchTarget {
  std::string nro_path;             // ruta absoluta "sdmc:/..."
  std::vector<std::string> args;    // argumentos extra (sin incluir la ruta del NRO)
};

// true si el loader que nos lanzo expuso EntryType_NextLoadPath (hbmenu, Sphaira, hbloader).
bool can_chain_nro();

// Prepara el siguiente NRO. Si devuelve true, el bucle principal DEBE terminar y salir de
// main para que el loader cargue el NRO. Deja el motivo en error si devuelve false.
bool request_next_nro(const LaunchTarget& target, std::string* error);

// Texto del modo de arranque detectado, para la UI ("hbmenu", "Sphaira", "forwarder NSP", "Album (applet)").
std::string boot_origin_description();

}  // namespace delanx

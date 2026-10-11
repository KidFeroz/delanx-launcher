// Delanx — construccion de la cadena de argumentos de envSetNextLoad.
//
// Esta separado del lanzador (que depende de libnx) para poder probarlo en el PC: un error
// aqui significa que el loader arranca el siguiente NRO con argumentos corruptos.
#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace delanx {

// Limites reales del ABI de homebrew: Value[0] = 512 B para la ruta del NRO,
// Value[1] = 2048 B para la cadena de argumentos. Se deja margen para el terminador y para
// no cortar el ultimo argumento por la mitad.
inline constexpr size_t kNextLoadPathMaxBytes = 480;
inline constexpr size_t kNextLoadArgvMaxBytes = 1900;

// Un argumento va entre comillas dobles si contiene espacios, comillas o barras invertidas;
// en ese caso se escapan las comillas y las barras. Tanto el parser de libnx como el de
// Sphaira respetan este formato.
std::string quote_launch_arg(const std::string& arg);

// Cadena de argumentos que espera el loader: argv[0] es la ruta del NRO (lo mismo que hace
// Sphaira, "keeps compat with hbloader") y despues los argumentos extra. Si no cabe todo, se
// descartan los ultimos argumentos enteros y `truncated` (si se pide) queda a true.
std::string build_launch_argv(const std::string& nro_path, const std::vector<std::string>& extra_args,
                              bool* truncated = nullptr);

}  // namespace delanx

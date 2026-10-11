// Delanx — eleccion del asset de Switch dentro de una release.
#pragma once

#include <string>
#include <vector>

#include "model/catalog.hpp"

namespace delanx {

struct AssetChoice {
  int index = -1;                // indice dentro de release.assets; -1 si no hay candidato
  bool automatic = false;        // true si hay exactamente un candidato claro
  std::vector<int> candidates;   // todos los candidatos validos, para que el usuario elija
  std::string reason;            // explicacion para la UI (en espanol)
};

// Filtra release.assets con el filtro dado (releaseAssetFilter de la entrada de catalogo o
// biblioteca) + marcador positivo de Switch y elige el candidato mas probable.
AssetChoice select_switch_asset(const Release& release, const std::string& asset_filter);

}  // namespace delanx

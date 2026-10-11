// Delanx — comprobacion del estado de una release contra la API de Quiver.
// Misma semantica que Quiver Launcher: verified / unverified / blocked + checksums SHA-256.
#pragma once

#include <map>
#include <string>
#include <vector>

#include "model/catalog.hpp"

namespace delanx {

struct ReleaseCheckInfo {
  Release::Check state = Release::Check::Unknown;
  std::vector<std::string> reasons;
  std::string verified_version;
  bool rolling = false;
  std::map<std::string, std::string> checksums;  // filename -> hex (sin el prefijo "sha256:")
};

// Consulta el historial de releases de la app y localiza el tag pedido.
// Nunca lanza: un fallo de red se traduce a Unverified con una razon explicativa.
ReleaseCheckInfo fetch_release_check(const std::string& base_url, const std::string& slug,
                                     const std::string& tag, std::string* error);

// Aplica el resultado al release: state, reasons, verified_version, rolling y checksums.
void apply_check(Release* release, const ReleaseCheckInfo& info);

}  // namespace delanx

// Delanx — releases de GitLab (gitlab.com/api/v4).
#pragma once

#include <string>
#include <vector>

#include "model/catalog.hpp"

namespace delanx::gl {

// GET https://gitlab.com/api/v4/projects/{path codificado}/releases?per_page=20.
// project_path es "namespace/proyecto" (se porcentaje-codifica la barra).
// Rellena tags, nombres, fechas, prerelease y assets desde links[] y sources[].
// GitLab no publica digest: los assets quedan con digest vacio.
std::vector<Release> fetch_releases(const std::string& project_path, const std::string& api_token,
                                    std::string* error);

}  // namespace delanx::gl

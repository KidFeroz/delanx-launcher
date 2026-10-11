// Delanx — releases de GitHub (api.github.com).
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "model/catalog.hpp"

namespace delanx::gh {

// Convierte un timestamp ISO 8601 en UTC ("2025-11-29T18:04:05Z", con o sin fraccion de
// segundo y con desfase +HH:MM) a segundos unix. Devuelve 0 si no se puede parsear.
// Parser manual: newlib no trae timegm() y strptime() depende del locale.
int64_t parse_iso8601_utc(const std::string& text);

// GET https://api.github.com/repos/{repository}/releases (30 por pagina, o 100 si hay token).
// Rellena: tag, name, body, published_at, published_unix, prerelease, assets y los
// checksums publicados en release.checksums ("sha256:<hex>" -> "<hex>").
// Devuelve vacio y rellena *error ante fallo de red o respuesta inesperada.
std::vector<Release> fetch_releases(const std::string& repository, const std::string& api_token,
                                    std::string* error);

// GET https://api.github.com/repos/{repository}/releases/tags/{tag}. Necesario para las
// releases *rolling*, donde el digest cambia bajo el mismo tag.
bool fetch_release_by_tag(const std::string& repository, const std::string& tag,
                          const std::string& api_token, Release* out, std::string* error);

}  // namespace delanx::gh

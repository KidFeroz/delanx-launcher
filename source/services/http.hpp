// Delanx — capa HTTP sobre libcurl (backend TLS de libnx: NO necesita CA bundle).
#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace delanx::net {

// Identificacion que exige la API de GitHub (una peticion sin UA recibe 403).
inline constexpr const char* kUserAgent = "Delanx/0.1 (+https://github.com/; Switch homebrew)";

struct Response {
  long status = 0;
  std::string body;
  std::string error;  // vacio si la peticion llego a completarse (aunque el status sea 4xx/5xx)

  bool ok() const { return error.empty() && status >= 200 && status < 300; }
};

// Progreso de descarga: recibe bytes descargados y total (0 si el servidor no lo anuncia).
// Devolver false cancela la descarga y borra el fichero parcial.
using ProgressFn = std::function<bool(int64_t downloaded, int64_t total)>;

// socketInitializeDefault() de libnx + curl_global_init(CURL_GLOBAL_DEFAULT). Idempotente.
void global_init();
// curl_global_cleanup() + socketExit().
void global_shutdown();

// GET a memoria. Nunca lanza: los fallos de red/TLS se devuelven en Response::error
// (texto de curl_easy_strerror) y los codigos HTTP en Response::status.
Response get(const std::string& url, const std::vector<std::string>& headers = {},
             int timeout_seconds = 20);

// POST con cuerpo JSON (anade Content-Type: application/json).
Response post_json(const std::string& url, const std::string& body,
                   const std::vector<std::string>& headers = {}, int timeout_seconds = 20);

// Descarga a dest_path escribiendo primero un ".part" en el mismo directorio y renombrando
// al terminar. Reintenta 2 veces adicionales ante error de red, llamando a progress de forma
// periodica. Si progress devuelve false aborta y borra el parcial.
bool download(const std::string& url, const std::string& dest_path, const ProgressFn& progress,
              std::string* error, const std::vector<std::string>& headers = {});

// Une base + query porcentaje-codificada ("?a=1&b=2"); los pares con valor vacio se omiten.
std::string build_url(const std::string& base,
                      const std::vector<std::pair<std::string, std::string>>& query);

}  // namespace delanx::net

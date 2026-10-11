// Delanx — implementacion de la capa HTTP (libcurl).
//
// Sobre TLS en Switch: switch-curl se compila con --with-libnx
// --with-default-ssl-backend=libnx, es decir el handshake lo hace el servicio `ssl` de
// Horizon con el CertStore del sistema. Por eso aqui NO se configura CA bundle ni se
// inicializa `ssl` a mano: basta socketInitializeDefault() (libnx) para el driver bsd.
// Ver INFORME-QuiverNX-Homebrew-Launcher.md §3.1.

#include "services/http.hpp"

#include <switch.h>

#include <curl/curl.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "core/paths.hpp"
#include "core/strings.hpp"

namespace delanx::net {
namespace {

// Peticiones normales; 5 s de conexion y el resto hasta agotar timeout_seconds.
constexpr long kConnectTimeoutSeconds = 5;

bool g_initialized = false;

// ---------------------------------------------------------------------------
// Callbacks de libcurl
// ---------------------------------------------------------------------------

size_t write_to_string(char* ptr, size_t size, size_t nmemb, void* userdata) {
  auto* out = static_cast<std::string*>(userdata);
  if (out == nullptr) return 0;
  const size_t bytes = size * nmemb;
  out->append(ptr, bytes);
  return bytes;
}

struct DownloadCtx {
  FILE* file = nullptr;
  const ProgressFn* progress = nullptr;
  bool cancelled = false;
};

size_t write_to_file(char* ptr, size_t size, size_t nmemb, void* userdata) {
  auto* ctx = static_cast<DownloadCtx*>(userdata);
  if (ctx == nullptr || ctx->file == nullptr) return 0;
  return std::fwrite(ptr, size, nmemb, ctx->file);
}

int xfer_info(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t /*ultotal*/,
              curl_off_t /*ulnow*/) {
  auto* ctx = static_cast<DownloadCtx*>(clientp);
  if (ctx == nullptr || ctx->progress == nullptr || !*ctx->progress) return 0;
  // total = 0 cuando el servidor no anuncia Content-Length.
  if (!(*ctx->progress)(static_cast<int64_t>(dlnow), static_cast<int64_t>(dltotal))) {
    ctx->cancelled = true;
    return 1;  // distinto de cero = abortar la transferencia
  }
  return 0;
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

curl_slist* make_headers(const std::vector<std::string>& headers) {
  curl_slist* list = nullptr;
  for (const std::string& header : headers) {
    if (header.empty()) continue;
    list = curl_slist_append(list, header.c_str());
  }
  return list;
}

}  // namespace

void global_init() {
  if (g_initialized) return;
  // El driver bsd de libnx. Si ya estaba inicializado por otra capa, el Result no es 0 y
  // no pasa nada: seguimos adelante.
  socketInitializeDefault();
  curl_global_init(CURL_GLOBAL_DEFAULT);
  g_initialized = true;
}

void global_shutdown() {
  if (!g_initialized) return;
  curl_global_cleanup();
  socketExit();
  g_initialized = false;
}

std::string build_url(const std::string& base,
                      const std::vector<std::pair<std::string, std::string>>& query) {
  std::string url = base;
  bool first = true;
  for (const auto& param : query) {
    if (param.first.empty() || param.second.empty()) continue;
    url += first ? '?' : '&';
    first = false;
    url += param.first;
    url += '=';
    url += str::url_encode(param.second);
  }
  return url;
}

Response get(const std::string& url, const std::vector<std::string>& headers,
             int timeout_seconds) {
  Response out;
  CURL* curl = curl_easy_init();
  if (curl == nullptr) {
    out.error = "no se pudo inicializar libcurl";
    return out;
  }

  std::string body;
  curl_slist* list = make_headers(headers);

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_string);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, kUserAgent);
  curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");  // todas las comprimidas soportadas
  curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, kConnectTimeoutSeconds);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, static_cast<long>(timeout_seconds));
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);  // obligatorio en newlib/Switch

  const CURLcode rc = curl_easy_perform(curl);
  long status = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);

  if (rc != CURLE_OK) {
    out.error = curl_easy_strerror(rc);
  } else {
    out.status = status;
    out.body = std::move(body);
  }

  curl_slist_free_all(list);
  curl_easy_cleanup(curl);
  return out;
}

Response post_json(const std::string& url, const std::string& body,
                   const std::vector<std::string>& headers, int timeout_seconds) {
  Response out;
  CURL* curl = curl_easy_init();
  if (curl == nullptr) {
    out.error = "no se pudo inicializar libcurl";
    return out;
  }

  std::vector<std::string> all = headers;
  all.push_back("Content-Type: application/json");
  all.push_back("Accept: application/json");

  std::string response;
  curl_slist* list = make_headers(all);

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_string);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, kUserAgent);
  curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
  curl_easy_setopt(curl, CURLOPT_POST, 1L);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, kConnectTimeoutSeconds);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, static_cast<long>(timeout_seconds));
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

  const CURLcode rc = curl_easy_perform(curl);
  long status = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);

  if (rc != CURLE_OK) {
    out.error = curl_easy_strerror(rc);
  } else {
    out.status = status;
    out.body = std::move(response);
  }

  curl_slist_free_all(list);
  curl_easy_cleanup(curl);
  return out;
}

bool download(const std::string& url, const std::string& dest_path, const ProgressFn& progress,
              std::string* error, const std::vector<std::string>& headers) {
  const auto fail = [error](const std::string& message) {
    if (error != nullptr) *error = message;
    return false;
  };

  if (url.empty()) return fail("URL vacia");

  const std::string final_path = paths::to_sd_path(dest_path);
  const std::string part_path = final_path + ".part";

  // El directorio destino puede no existir todavia (primer arranque).
  const std::string dir = paths::dirname(final_path);
  if (!dir.empty()) paths::make_dirs(dir);
  // Restos de un intento anterior.
  if (paths::exists(part_path)) paths::remove_file(part_path);

  constexpr int kAttempts = 3;  // 1 intento + 2 reintentos en error de red
  for (int attempt = 1; attempt <= kAttempts; ++attempt) {
    CURL* curl = curl_easy_init();
    if (curl == nullptr) return fail("no se pudo inicializar libcurl");

    FILE* file = std::fopen(part_path.c_str(), "wb");
    if (file == nullptr) {
      curl_easy_cleanup(curl);
      return fail("no se pudo abrir " + part_path + " para escritura");
    }

    DownloadCtx ctx;
    ctx.file = file;
    ctx.progress = &progress;

    curl_slist* list = make_headers(headers);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_file);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ctx);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, kUserAgent);
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, xfer_info);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &ctx);
    // Las descargas de releases pueden tardar: sin CURLOPT_TIMEOUT, solo limites de conexion
    // y de velocidad (si el servidor deja de enviar, se corta en vez de colgarse).
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, kConnectTimeoutSeconds);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 64L);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 60L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

    const CURLcode rc = curl_easy_perform(curl);
    long status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);

    curl_slist_free_all(list);
    curl_easy_cleanup(curl);
    std::fclose(file);

    if (ctx.cancelled) {
      paths::remove_file(part_path);
      return fail("descarga cancelada");
    }
    if (rc != CURLE_OK) {
      paths::remove_file(part_path);
      if (attempt < kAttempts) continue;  // error de red: reintentar
      return fail(curl_easy_strerror(rc));
    }
    if (status < 200 || status >= 300) {
      paths::remove_file(part_path);
      return fail(str::format("el servidor respondio HTTP %ld", status));
    }

    // newlib/FAT no sobrescribe de forma fiable con rename: quitamos el destino primero.
    if (paths::exists(final_path)) paths::remove_file(final_path);
    if (std::rename(part_path.c_str(), final_path.c_str()) != 0) {
      paths::remove_file(part_path);
      return fail("no se pudo renombrar " + part_path + " a " + final_path);
    }
    if (error != nullptr) error->clear();
    return true;
  }

  paths::remove_file(part_path);
  return fail("descarga fallida tras varios intentos");
}

}  // namespace delanx::net

// Delanx — implementacion del cliente de releases de GitLab.
//
// Nota de la API v4: la ruta del proyecto va porcentaje-codificada ("grupo%2Fproyecto") en
// una sola peticion, es decir no se pagina: /releases devuelve la lista completa (aqui se
// pide per_page=20). Los assets de un release se publican como "links" (enlaces directos) o
// como "sources" (archivos fuente autogenerados) y ninguno trae checksum.

#include "services/gitlab.hpp"

#include <string>
#include <utility>
#include <vector>

#include "core/json.hpp"
#include "core/strings.hpp"
#include "services/github.hpp"  // parse_iso8601_utc
#include "services/http.hpp"

namespace delanx::gl {
namespace {

constexpr const char* kApiBase = "https://gitlab.com/api/v4";
constexpr int kTimeoutSeconds = 25;

int64_t parse_date(const std::string& text) { return gh::parse_iso8601_utc(text); }

void set_error(std::string* error, const std::string& message) {
  if (error != nullptr) *error = message;
}

std::vector<std::string> base_headers(const std::string& api_token) {
  std::vector<std::string> headers;
  if (!api_token.empty()) headers.push_back("PRIVATE-TOKEN: " + api_token);
  return headers;
}

std::string api_message(const std::string& body) {
  const auto doc = json::Doc::parse(body);
  if (!doc.has_value()) return {};
  const json::Value root = doc->root();
  const std::string message = str::trim(root.string("message"));
  if (!message.empty()) return message;
  return str::trim(root.string("error"));
}

std::string base_name(const std::string& url) {
  const size_t query = url.find_first_of("?#");
  const std::string clean = query == std::string::npos ? url : url.substr(0, query);
  const size_t slash = clean.find_last_of('/');
  return slash == std::string::npos ? clean : clean.substr(slash + 1);
}

// En la API v4 los ficheros van anidados en el objeto `assets` del release:
// assets.links[] (name/direct_asset_url/url) y assets.sources[] (format/url).
void parse_assets(const json::Value& assets_node, Release* release) {
  const json::Value links = assets_node.get("links");
  if (links.is_array()) {
    for (size_t i = 0; i < links.size(); ++i) {
      const json::Value link = links.at(i);
      if (!link.is_object()) continue;
      ReleaseAsset asset;
      asset.name = link.string("name");
      asset.url = link.string("direct_asset_url");
      if (asset.url.empty()) asset.url = link.string("url");
      asset.size = link.integer("size");
      if (asset.name.empty() && !asset.url.empty()) asset.name = base_name(asset.url);
      // GitLab no publica digest: asset.digest y release->checksums quedan vacios.
      if (!asset.name.empty() && !asset.url.empty()) release->assets.push_back(std::move(asset));
    }
  }

  const json::Value sources = assets_node.get("sources");
  if (sources.is_array()) {
    for (size_t i = 0; i < sources.size(); ++i) {
      const json::Value source = sources.at(i);
      if (!source.is_object()) continue;
      ReleaseAsset asset;
      const std::string format = source.string("format");
      asset.url = source.string("url");
      asset.name = format.empty() ? base_name(asset.url)
                                  : release->tag + "." + str::lower(format);
      if (!asset.url.empty()) release->assets.push_back(std::move(asset));
    }
  }
}

}  // namespace

std::vector<Release> fetch_releases(const std::string& project_path, const std::string& api_token,
                                    std::string* error) {
  std::vector<Release> releases;
  if (error != nullptr) error->clear();
  if (project_path.empty()) {
    set_error(error, "proyecto vacio");
    return releases;
  }

  // La barra de "grupo/proyecto" debe ir codificada para que GitLab la trate como una sola
  // ruta (no como /projects/grupo/releases/proyecto).
  const std::string url =
      str::format("%s/projects/%s/releases?per_page=20", kApiBase,
                  str::url_encode(project_path).c_str());
  const net::Response response = net::get(url, base_headers(api_token), kTimeoutSeconds);

  if (!response.error.empty()) {
    set_error(error, "error de red (releases de " + project_path + "): " + response.error);
    return releases;
  }
  if (response.status != 200) {
    const std::string detail = api_message(response.body);
    set_error(error, str::format("GitLab respondio HTTP %ld%s%s", response.status,
                                 detail.empty() ? "" : " - ", detail.c_str()));
    return releases;
  }

  const auto doc = json::Doc::parse(response.body);
  if (!doc.has_value() || !doc->root().is_array()) {
    set_error(error, "respuesta inesperada de la API de GitLab");
    return releases;
  }

  const json::Value root = doc->root();
  releases.reserve(root.size());
  for (size_t i = 0; i < root.size(); ++i) {
    const json::Value node = root.at(i);
    if (!node.is_object()) continue;

    Release release;
    release.tag = node.string("tag_name");
    release.name = node.string("name");
    if (release.name.empty()) release.name = release.tag;
    release.body = node.string("description");
    release.published_at = node.string("released_at");
    if (release.published_at.empty()) release.published_at = node.string("created_at");
    release.published_unix = parse_date(release.published_at);
    // GitLab no tiene bandera de prerelease: marca el release como "upcoming" cuando aun no
    // se ha publicado (el campo es booleano en la API, no cadena).
    release.prerelease = node.boolean("upcoming_release", false);
    parse_assets(node.get("assets"), &release);
    if (!release.tag.empty()) releases.push_back(std::move(release));
  }
  return releases;
}

}  // namespace delanx::gl

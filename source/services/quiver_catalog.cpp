// Delanx — implementacion del cliente de la API de Quiver Launcher.
//
// Orden de parametros de /apps copiado del cliente oficial para que el servidor reciba
// exactamente la misma peticion: limit, search, os, console|maker, projectType, ai, sort
// (solo sin busqueda), cursor.

#include "services/quiver_catalog.hpp"

#include <jansson.h>

#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

#include "core/json.hpp"
#include "core/strings.hpp"
#include "services/http.hpp"

namespace delanx::quiver {
namespace {

constexpr const char* kMakerPrefix = "maker:";
constexpr int kTimeoutSeconds = 20;

std::string normalize_base(const std::string& base_url) {
  if (base_url.empty()) return std::string(kDefaultBaseUrl);
  std::string base = base_url;
  while (!base.empty() && base.back() == '/') base.pop_back();
  return base;
}

// Quiver devuelve la entrada directamente o envuelta en {"entry":{...}} segun el endpoint.
json::Value entry_of(const json::Value& root) {
  const json::Value entry = root.get("entry");
  if (entry.is_object()) return entry;
  return root;
}

// Primera cadena no vacia: libraryArt.capsule -> hero -> logo -> header -> artwork.
std::string first_non_empty(const std::vector<json::Value>& nodes) {
  for (const json::Value& node : nodes) {
    const std::string value = str::trim(node.as_string());
    if (!value.empty()) return value;
  }
  return {};
}

void parse_app(const json::Value& node, CatalogApp* out) {
  if (!node.is_object()) return;

  out->id = node.string("id");
  out->slug = node.string("slug");
  out->name = node.string("name");
  out->description = node.string("description");
  out->project_name = node.string("projectName");
  out->project_type = node.string("projectType");
  out->ai_level = node.string("aiLevel");

  out->consoles = node.string_array("consoles");
  out->tags = node.string_array("tags");
  out->supported_os = node.string_array("supportedOS");

  out->artwork = node.string("artwork");

  const json::Value library_art = node.get("libraryArt");
  out->icon_url = first_non_empty({library_art.get("capsule"), library_art.get("hero"),
                                   library_art.get("logo"), library_art.get("header"),
                                   node.get("artwork")});

  const json::Value launcher = node.get("launcher");
  out->folder_name = launcher.string("folderName");
  out->release_asset_filter = launcher.string("releaseAssetFilter");

  const json::Value project = node.get("project");
  out->repository = project.string("repository");
  out->repository_source = str::lower(str::trim(project.string("provider")));
  // "manual" = el proyecto no tiene repositorio de releases: Delanx lo trata como
  // gestionado a mano (sin descargas ni actualizaciones), igual que Quiver.
  if (out->repository_source == "manual") {
    out->repository.clear();
    out->repository_source.clear();
  }

  const json::Value verified = node.get("verified");
  out->verified_version = verified.string("version");
  out->rolling = verified.boolean("rolling");
}

void set_error(std::string* error, const std::string& message) {
  if (error != nullptr) *error = message;
}

// Traduce un fallo de transporte o de status a *error. Devuelve false si hubo fallo.
bool ensure_ok(const net::Response& response, const std::string& what, const char* service,
               std::string* error) {
  if (!response.error.empty()) {
    set_error(error, "error de red (" + what + "): " + response.error);
    return false;
  }
  if (response.status != 200) {
    set_error(error, str::format("%s respondio HTTP %ld (%s)", service, response.status,
                                 what.c_str()));
    return false;
  }
  return true;
}

}  // namespace

Page fetch_apps(const std::string& base_url, const Query& query, std::string* error) {
  Page page;
  if (error != nullptr) error->clear();

  const std::string base = normalize_base(base_url);
  std::vector<std::pair<std::string, std::string>> params;
  params.emplace_back("limit", str::format("%d", query.limit > 0 ? query.limit : 48));
  params.emplace_back("search", query.search);
  params.emplace_back("os", query.os);
  // Un "console" con prefijo "maker:" es en realidad una busqueda por fabricante.
  if (str::starts_with(query.console, kMakerPrefix)) {
    params.emplace_back("maker", query.console.substr(std::string(kMakerPrefix).size()));
  } else {
    params.emplace_back("console", query.console);
  }
  params.emplace_back("projectType", query.project_type);
  params.emplace_back("ai", query.ai);
  // El cliente oficial solo manda sort cuando no hay busqueda.
  if (query.search.empty()) params.emplace_back("sort", query.sort);
  params.emplace_back("cursor", query.cursor);

  const std::string url = net::build_url(base + "/apps", params);
  const net::Response response = net::get(url, {}, kTimeoutSeconds);
  if (!ensure_ok(response, "listado de apps", "Quiver", error)) return page;

  const auto doc = json::Doc::parse(response.body);
  if (!doc.has_value() || !doc->root().is_object()) {
    set_error(error, "respuesta inesperada del catalogo de Quiver");
    return page;
  }

  const json::Value root = doc->root();
  const json::Value items = root.get("items");
  if (items.is_array()) {
    page.items.reserve(items.size());
    for (size_t i = 0; i < items.size(); ++i) {
      CatalogApp app;
      parse_app(items.at(i), &app);
      if (!app.slug.empty()) page.items.push_back(std::move(app));
    }
  }
  page.next_cursor = root.string("nextCursor");
  // isDone ausente se interpreta como pagina final (evita bucles de paginacion).
  page.is_done = root.get("isDone").valid() ? root.boolean("isDone", true) : true;
  return page;
}

Facets fetch_facets(const std::string& base_url, std::string* error) {
  Facets facets;
  if (error != nullptr) error->clear();

  const std::string base = normalize_base(base_url);
  const net::Response response = net::get(base + "/facets", {}, kTimeoutSeconds);
  if (!ensure_ok(response, "facets", "Quiver", error)) return facets;

  const auto doc = json::Doc::parse(response.body);
  if (!doc.has_value() || !doc->root().is_object()) {
    set_error(error, "respuesta inesperada del catalogo de Quiver");
    return facets;
  }

  const json::Value root = doc->root();
  facets.total = static_cast<int>(root.integer("total"));
  const json::Value consoles = root.get("consoles");
  if (consoles.is_array()) {
    facets.consoles.reserve(consoles.size());
    for (size_t i = 0; i < consoles.size(); ++i) {
      const json::Value node = consoles.at(i);
      if (!node.is_object()) continue;
      Facet facet;
      facet.id = node.string("id");
      facet.name = node.string("name");
      facet.brand = node.string("brand");
      if (!facet.id.empty()) facets.consoles.push_back(std::move(facet));
    }
  }
  return facets;
}

bool fetch_detail(const std::string& base_url, const std::string& slug, CatalogApp* out,
                  std::string* error) {
  if (error != nullptr) error->clear();
  if (out == nullptr) {
    set_error(error, "salida nula");
    return false;
  }

  const std::string url = normalize_base(base_url) + "/apps/" + str::url_encode(slug);
  const net::Response response = net::get(url, {}, kTimeoutSeconds);
  if (!ensure_ok(response, "detalle de " + slug, "Quiver", error)) return false;

  const auto doc = json::Doc::parse(response.body);
  if (!doc.has_value()) {
    set_error(error, "respuesta inesperada del catalogo de Quiver");
    return false;
  }

  const json::Value entry = entry_of(doc->root());
  if (!entry.is_object()) {
    set_error(error, "respuesta inesperada del catalogo de Quiver");
    return false;
  }
  *out = CatalogApp();
  parse_app(entry, out);
  return true;
}

bool report_download_problem(const std::string& base_url, const std::string& slug,
                             const std::string& version, const std::string& file,
                             const std::string& problem) {
  if (slug.empty()) return false;

  json::Builder payload = json::Builder::object();
  payload.set("version", version);
  payload.set("file", file);
  payload.set("problem", problem);

  std::string body;
  json_t* root = payload.release();
  if (root != nullptr) {
    char* dumped = json_dumps(root, JSON_COMPACT);
    if (dumped != nullptr) {
      body.assign(dumped);
      std::free(dumped);
    }
    json_decref(root);
  }
  if (body.empty()) return false;

  const std::string url =
      normalize_base(base_url) + "/apps/" + str::url_encode(slug) + "/download-problem";
  // Un fallo al reportar no debe romper el flujo de instalacion: no hay parametro de error.
  const net::Response response = net::post_json(url, body, {}, kTimeoutSeconds);
  return response.error.empty() && (response.status == 200 || response.status == 202);
}

}  // namespace delanx::quiver

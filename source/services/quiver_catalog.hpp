// Delanx — cliente de la API publica del catalogo de Quiver Launcher.
// Base verificada: https://api.quiverlauncher.com/api/v1
#pragma once

#include <string>
#include <vector>

namespace delanx::quiver {

// Base por defecto de la API publica (sin autenticacion).
inline constexpr const char* kDefaultBaseUrl = "https://api.quiverlauncher.com/api/v1";

struct Facet {
  std::string id;
  std::string name;
  std::string brand;
};

struct Facets {
  int total = 0;
  std::vector<Facet> consoles;
};

// Entrada del catalogo de Quiver, aplanada a lo que Delanx necesita.
struct CatalogApp {
  std::string id, slug, name, description, project_name, project_type, ai_level;
  std::string repository, repository_source, folder_name, release_asset_filter;
  std::string artwork, icon_url, verified_version;
  bool rolling = false;
  std::vector<std::string> consoles, tags, supported_os;
};

struct Page {
  std::vector<CatalogApp> items;
  std::string next_cursor;
  bool is_done = true;
};

struct Query {
  std::string search, os, console, project_type, ai, sort = "added", cursor;
  int limit = 48;
};

// GET /apps con los mismos parametros que el cliente oficial.
Page fetch_apps(const std::string& base_url, const Query& query, std::string* error);

// GET /facets (total y consolas).
Facets fetch_facets(const std::string& base_url, std::string* error);

// GET /apps/{slug}: el detalle completo (entrada + proyecto + juego).
bool fetch_detail(const std::string& base_url, const std::string& slug, CatalogApp* out,
                  std::string* error);

// POST /apps/{slug}/download-problem con {"version","file","problem"}.
// problem es "missing" o "mismatch"; se considera exito 200 y 202.
bool report_download_problem(const std::string& base_url, const std::string& slug,
                             const std::string& version, const std::string& file,
                             const std::string& problem);

}  // namespace delanx::quiver

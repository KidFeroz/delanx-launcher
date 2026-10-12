// Delanx — catalogo propio (romfs:/catalog/*.json) y modelo de releases.
#include "model/catalog.hpp"

#include <cctype>

#include "core/paths.hpp"
#include "core/strings.hpp"

namespace delanx {
namespace {

// Extensiones de escritorio/consola ajena que Quiver tambien excluye.
// (se anaden .vpk/.cia: PS Vita y 3DS, que antes se colaban como candidatos)
const char* const kForeignExtensions[] = {".exe", ".msi", ".appimage", ".apk", ".dmg",
                                          ".deb", ".rpm", ".vpk",     ".cia"};
// Marcadores de otra plataforma. Incluye las formas cortas de Windows ("win-", "_win")
// porque es como se publican muchos releases (p. ej. "App-win-Portable.zip").
const char* const kForeignMarkers[] = {"windows", "win64", "win32", "win-",  "-win", "_win",
                                       "linux",   "macos", "osx",   "android", "-ios", "_ios",
                                       "wiiu",    "wii",   "3ds",   "vita",   "psvita",
                                       "psp",     "ps4",   "xbox"};

template <size_t N>
bool ends_with_any(const std::string& text, const char* const (&suffixes)[N]) {
  for (const char* suffix : suffixes) {
    if (str::ends_with(text, suffix)) return true;
  }
  return false;
}

// Carpeta destino cuando el catalogo no la fija: se prefiere el nombre del repositorio
// (mas estable que el nombre visible) y se sanea a caracteres validos en FAT/exFAT.
std::string sanitize_folder_name(const std::string& text) {
  std::string out;
  out.reserve(text.size());
  for (char c : text) {
    const unsigned char uc = static_cast<unsigned char>(c);
    if (std::isalnum(uc) || c == '-' || c == '_' || c == '.') out.push_back(c);
  }
  return out;
}

std::string repository_tail(const std::string& repository) {
  const size_t slash = repository.find_last_of('/');
  return slash == std::string::npos ? repository : repository.substr(slash + 1);
}

void append_entries(const json::Value& array, std::vector<CatalogEntry>& out) {
  if (!array.is_array()) return;
  const size_t count = array.size();
  out.reserve(out.size() + count);
  for (size_t i = 0; i < count; ++i) {
    const json::Value item = array.at(i);
    if (!item.is_object()) continue;
    out.push_back(CatalogEntry::from_json(item));
  }
}

void append_unavailable(const json::Value& array, std::vector<UnavailableEntry>& out) {
  if (!array.is_array()) return;
  const size_t count = array.size();
  out.reserve(out.size() + count);
  for (size_t i = 0; i < count; ++i) {
    const json::Value item = array.at(i);
    if (!item.is_object()) continue;
    UnavailableEntry entry;
    entry.name = item.string("name");
    entry.repository = str::trim(item.string("repository"));
    entry.reason = item.string("reason");
    entry.last_checked = item.string("lastChecked");
    entry.workaround = item.string("workaround");
    if (entry.name.empty() && entry.repository.empty()) continue;
    out.push_back(entry);
  }
}

// Formato comun a switch-catalog.json y harbourmasters.json:
//   {"schemaVersion":1,"entries":[...],"unavailable":[...]}
// "schemaVersion" se ignora a proposito: el parser es tolerante a versiones futuras.
std::optional<Catalog> parse_catalog_text(const std::string& text) {
  const auto doc = json::Doc::parse(text);
  if (!doc) return std::nullopt;

  const json::Value root = doc->root();
  Catalog catalog;
  if (root.is_array()) {
    append_entries(root, catalog.entries);
    return catalog;
  }
  if (!root.is_object()) return std::nullopt;
  append_entries(root.get("entries"), catalog.entries);
  append_unavailable(root.get("unavailable"), catalog.unavailable);
  return catalog;
}

}  // namespace

// ---------------------------------------------------------------------------
// ReleaseAsset
// ---------------------------------------------------------------------------

bool ReleaseAsset::is_archive() const {
  static const char* const kArchiveExtensions[] = {".zip",  ".7z",     ".rar",
                                                   ".tar.gz", ".tar.xz", ".nro.zip"};
  return ends_with_any(str::lower(name), kArchiveExtensions);
}

bool ReleaseAsset::is_single_nro() const { return str::iends_with(name, ".nro"); }

bool ReleaseAsset::looks_like_foreign_platform() const {
  const std::string name_lower = str::lower(name);
  if (ends_with_any(name_lower, kForeignExtensions)) return true;
  for (const char* marker : kForeignMarkers) {
    if (name_lower.find(marker) != std::string::npos) return true;
  }
  return false;
}

// Marcador POSITIVO de Switch. Es el predicado INVERSO del de Quiver: su
// PlatformAssetMatcher trata "switch" como marcador *no-Windows/macOS/Linux/Android* y por
// eso descarta justamente los assets que a Delanx le interesan. Aqui "switch" (y compania)
// es la senal de que el asset es para Horizon OS.
bool ReleaseAsset::looks_like_switch() const {
  const std::string name_lower = str::lower(name);
  // Un .nro (o un .nro.zip) es Switch por definicion, aunque el nombre no lo diga.
  if (str::ends_with(name_lower, ".nro") || str::ends_with(name_lower, ".nro.zip")) return true;

  static const char* const kSwitchMarkers[] = {"switch", "-nx", "_nx", "nswitch", "horizon"};
  for (const char* marker : kSwitchMarkers) {
    if (name_lower.find(marker) != std::string::npos) {
      // Ojo: "switch-1.0-linux.zip" lleva marcador switch pero es un build ajeno.
      return !looks_like_foreign_platform();
    }
  }
  // Plataforma ajena sin marcador de Switch (o nombre neutro): no se ofrece para instalar.
  return false;
}

// ---------------------------------------------------------------------------
// Release
// ---------------------------------------------------------------------------

const ReleaseAsset* Release::find_asset(const std::string& name) const {
  if (name.empty()) return nullptr;
  for (const ReleaseAsset& asset : assets) {
    if (asset.name == name) return &asset;
  }
  for (const ReleaseAsset& asset : assets) {  // segunda pasada tolerante a mayusculas
    if (str::iequals(asset.name, name)) return &asset;
  }
  return nullptr;
}

std::string Release::checksum_for(const std::string& asset_name) const {
  if (asset_name.empty()) return {};
  const std::string wanted = str::lower(str::trim(asset_name));
  for (const auto& pair : checksums) {
    if (str::lower(pair.first) == wanted) return pair.second;
  }
  return {};
}

// ---------------------------------------------------------------------------
// CatalogEntry
// ---------------------------------------------------------------------------

CatalogEntry CatalogEntry::from_json(const json::Value& node) {
  CatalogEntry entry;
  if (!node.is_object()) return entry;

  entry.name = node.string("name");
  entry.project = node.string("project");
  entry.repository = str::trim(node.string("repository"));

  const std::string source = str::lower(str::trim(node.string("repositorySource")));
  entry.repository_source = (source == "gitlab") ? "gitlab" : "github";

  entry.folder_name = str::trim(node.string("folderName"));
  entry.tags = str::normalize_tags(node.string_array("tags"));
  entry.release_asset_filter = node.string("releaseAssetFilter");
  entry.requirements = node.string("requirements");
  entry.app_icon_url = str::trim(node.string("appIconUrl"));
  // Claves informativas del catalogo maestro: ausentes en switch-catalog.json y en
  // harbourmasters.json, asi que se leen de forma tolerante (vacio si no estan).
  entry.category = str::trim(node.string("category"));
  entry.asset_type = str::trim(node.string("assetType"));
  entry.rom_source_hint = str::trim(node.string("romSourceHint"));
  entry.switch_proof = node.string_array("switchProof");
  entry.verified = node.boolean("verified", false);
  entry.mrship = node.boolean("mrship", node.boolean("harbourMasters", false));
  entry.notes = node.string("notes");

  // El bloque "switch" y sus normalizaciones (to_sd_path, descarte de rutas vacias) son
  // identicos en apps.json y en el catalogo: se reutiliza el parser de AppEntry en lugar de
  // duplicar reglas que podrian divergir.
  entry.sw = AppEntry::from_json(node).sw;
  return entry;
}

AppEntry CatalogEntry::to_app_entry() const {
  AppEntry app;
  app.name = name;
  app.project = project;
  app.repository = repository;
  app.repository_source = repository_source.empty() ? "github" : str::lower(repository_source);
  app.folder_name = folder_name;
  if (app.folder_name.empty()) app.folder_name = sanitize_folder_name(repository_tail(repository));
  if (app.folder_name.empty()) app.folder_name = sanitize_folder_name(name);
  app.app_icon_url = app_icon_url;
  // normalize_tags es idempotente: la entrada de biblioteca siempre sale normalizada aunque
  // el CatalogEntry se haya construido a mano en lugar de con from_json.
  app.tags = str::normalize_tags(tags);
  app.release_asset_filter = release_asset_filter;
  app.requirements = requirements;
  app.mrship = mrship;
  app.verified_entry = verified;
  app.sw = sw;
  app.install_path.clear();  // el catalogo no fija ruta en la SD: manda switch.targetDir
  // Quiver identifica la entrada de catalogo por el repositorio.
  app.catalog_entry_id = repository;

  // Ficha de catalogo: permite pintar nombre/proyecto/tags/icono aunque no haya red.
  CatalogSnapshot snapshot;
  snapshot.name = name;
  snapshot.project = project;
  snapshot.app_icon_url = app_icon_url;
  snapshot.tags = tags;
  if (!snapshot.name.empty() || !snapshot.project.empty() || !snapshot.tags.empty() || !snapshot.app_icon_url.empty()) {
    app.catalog = snapshot;
  }
  return app;
}

std::string CatalogEntry::badge() const {
  if (mrship) return "MrShip";
  if (verified) return "Switch";
  return "Sin verificar";
}

// ---------------------------------------------------------------------------
// Catalog
// ---------------------------------------------------------------------------

std::optional<Catalog> Catalog::load_file(const std::string& path) {
  const auto text = paths::read_file(path);
  if (!text) return std::nullopt;
  return parse_catalog_text(*text);
}

std::optional<Catalog> Catalog::load_romfs(const char* relative_name) {
  if (!relative_name || !*relative_name) return std::nullopt;
  // Sin <switch.h>: basta la ruta literal que expone el VFS de libnx una vez llamado
  // romfsInit() en main. Acepta tanto "catalog/switch-catalog.json" como "romfs:/...".
  std::string path = relative_name;
  if (!str::starts_with(path, "romfs:")) {
    path = (path[0] == '/') ? ("romfs:" + path) : ("romfs:/" + path);
  }
  const auto text = paths::read_file(path);
  if (!text) return std::nullopt;
  return parse_catalog_text(*text);
}

}  // namespace delanx

// Delanx — AppEntry y Library sobre apps.json de Quiver Launcher.
#include "model/app_entry.hpp"

#include <algorithm>
#include <cstdlib>
#include <utility>

#include "core/paths.hpp"
#include "core/strings.hpp"
#include "model/settings.hpp"

namespace delanx {
namespace {

using json::Builder;
using json::Value;

// Builder no expone forma de adoptar su json_t* en un Doc, asi que se serializa a texto y
// se delega en Doc::save_file: mismo formato y misma escritura atomica (tmp + rename) que
// usa el resto del proyecto.
bool save_builder(const std::string& path, const Builder& root) {
  const json_t* raw = root.value().raw();
  if (!raw) return false;
  char* dumped = json_dumps(raw, JSON_INDENT(2) | JSON_PRESERVE_ORDER);
  if (!dumped) return false;
  const std::string text = dumped;
  free(dumped);
  const auto doc = json::Doc::parse(text);
  if (!doc) return false;
  paths::make_dirs(paths::dirname(path));  // la estructura de la SD puede no existir aun
  return doc->save_file(path);
}

// Bloque "switch": extension propia de Delanx que Quiver ignora sin romperse.
SwitchInstall parse_switch_install(const Value& node) {
  SwitchInstall sw;
  if (!node.is_object()) return sw;

  sw.target_dir = str::trim(node.string("targetDir"));
  if (!sw.target_dir.empty()) sw.target_dir = paths::to_sd_path(sw.target_dir);
  // homeDir es opcional (Quiver no lo conoce): ausente = la app vive en target_dir.
  sw.home_dir = str::trim(node.string("homeDir"));
  if (!sw.home_dir.empty()) sw.home_dir = paths::to_sd_path(sw.home_dir);
  sw.nro = str::trim(node.string("nro"));
  sw.nro_path = str::trim(node.string("nroPath"));
  sw.strip_root = node.boolean("stripRoot", false);

  const Value files = node.get("requiredUserFiles");
  if (files.is_array()) {
    for (size_t i = 0; i < files.size(); ++i) {
      const Value item = files.at(i);
      if (!item.is_object()) continue;
      SwitchInstall::RequiredFile file;
      // La ruta se normaliza al consultarla (missing_user_files): asi el apps.json
      // conserva exactamente lo que escribio Quiver o Delanx.
      file.path = str::trim(item.string("path"));
      if (file.path.empty()) continue;
      file.description = item.string("description");
      sw.required_user_files.push_back(file);
    }
  }
  return sw;
}

// Primer .nro lanzable: raiz de la carpeta y despues un unico nivel de subcarpeta.
// Limitar la busqueda a un nivel evita dar por instalada una app que no se puede lanzar.
std::string find_launch_nro(const std::string& dir) {
  if (dir.empty() || !paths::is_dir(dir)) return {};
  std::vector<std::string> subdirs;
  for (const std::string& name : paths::list_dir(dir)) {
    const std::string path = paths::join(dir, name);
    if (paths::is_dir(path)) {
      subdirs.push_back(path);
    } else if (paths::extension(path) == ".nro") {
      return path;
    }
  }
  for (const std::string& subdir : subdirs) {
    for (const std::string& name : paths::list_dir(subdir)) {
      const std::string path = paths::join(subdir, name);
      if (!paths::is_dir(path) && paths::extension(path) == ".nro") return path;
    }
  }
  return {};
}

// Existe el .nro de lanzamiento? Se respeta switch.nro / switch.nroPath y, si no, se busca.
bool has_launch_nro(const SwitchInstall& sw, const std::string& dir) {
  if (dir.empty()) return false;
  if (!paths::is_dir(dir)) {
    if (paths::exists(dir) && paths::extension(dir) == ".nro") return true;
    return false;
  }
  if (!sw.nro.empty()) {
    if (paths::exists(paths::join(dir, sw.nro))) return true;
    const std::string base = paths::basename(sw.nro);
    if (base != sw.nro && paths::exists(paths::join(dir, base))) return true;
    // Busqueda insensible a mayusculas/minusculas dentro de la carpeta
    for (const std::string& name : paths::list_dir(dir)) {
      if (str::iequals(name, sw.nro) || str::iequals(name, base)) return true;
    }
  }
  if (!sw.nro_path.empty()) {
    const std::string sd = paths::to_sd_path(sw.nro_path);
    if (!paths::is_dir(sd) && paths::exists(sd)) return true;
    const std::string base = paths::basename(sw.nro_path);
    for (const std::string& name : paths::list_dir(dir)) {
      if (str::iequals(name, base)) return true;
    }
  }
  return !find_launch_nro(dir).empty();
}

void append_apps(const Value& array, std::vector<AppEntry>& out) {
  if (!array.is_array()) return;
  const size_t count = array.size();
  out.reserve(out.size() + count);
  for (size_t i = 0; i < count; ++i) {
    const Value item = array.at(i);
    if (!item.is_object()) continue;  // entradas corruptas: se descartan, no rompen la carga
    out.push_back(AppEntry::from_json(item));
  }
}

}  // namespace

// ---------------------------------------------------------------------------
// SwitchInstall
// ---------------------------------------------------------------------------

std::string SwitchInstall::effective_target(const std::string& apps_dir,
                                            const std::string& folder_name) const {
  if (!target_dir.empty()) return paths::to_sd_path(target_dir);
  return paths::join(apps_dir, folder_name);
}

// ---------------------------------------------------------------------------
// AppEntry
// ---------------------------------------------------------------------------

AppEntry AppEntry::from_json(const json::Value& node) {
  AppEntry entry;
  if (!node.is_object()) return entry;

  // ---- contrato Quiver ----
  entry.name = node.string("name");
  entry.project = node.string("project");
  entry.custom_display_name = node.string("customDisplayName");
  entry.repository = str::trim(node.string("repository"));  // vacio => manually managed
  entry.folder_name = str::trim(node.string("folderName"));
  entry.install_path = str::trim(node.string("installPath"));
  entry.preferred_version = node.string("preferredVersion");
  entry.skipped_update_version = node.string("skippedUpdateVersion");
  entry.auto_update = node.boolean("autoUpdate", false);
  entry.defer_update_tracking = node.boolean("deferUpdateTracking", false);
  entry.tags = str::normalize_tags(node.string_array("tags"));
  entry.files_to_add = node.string_array("filesToAdd");
  entry.release_asset_filter = node.string("releaseAssetFilter");
  entry.catalog_entry_id = node.string("catalogEntryId");

  // Alias de icono verificados: del mas especifico al mas generico.
  entry.app_icon_url = node.string("appIconUrl");
  if (entry.app_icon_url.empty()) entry.app_icon_url = node.string("gameIconUrl");
  if (entry.app_icon_url.empty()) entry.app_icon_url = node.string("customDefaultIconUrl");

  // Fuente del repositorio: ausente => GitHub (igual que Quiver). Cualquier otro valor se
  // degrada a GitHub dejando el aviso en last_error (estado de ejecucion, no se serializa).
  const std::string source_raw = str::trim(node.string("repositorySource"));
  const std::string source = str::lower(source_raw);
  if (source.empty()) {
    entry.repository_source = "github";
  } else if (source == "github" || source == "gitlab") {
    entry.repository_source = source;
  } else {
    entry.repository_source = "github";
    entry.last_error = "repositorySource desconocido (\"" + source_raw + "\"): se asume github";
  }

  // ---- catalogo y mods ----
  const Value catalog = node.get("catalog");
  if (catalog.is_object()) {
    CatalogSnapshot snapshot;
    snapshot.name = catalog.string("name");
    snapshot.project = catalog.string("project");
    snapshot.app_icon_url = catalog.string("appIconUrl");
    if (snapshot.app_icon_url.empty()) snapshot.app_icon_url = catalog.string("gameIconUrl");
    snapshot.tags = str::normalize_tags(catalog.string_array("tags"));
    if (!snapshot.name.empty() || !snapshot.project.empty() || !snapshot.app_icon_url.empty() ||
        !snapshot.tags.empty()) {
      entry.catalog = snapshot;
    }
  }

  const Value mods = node.get("mods");
  if (mods.is_object()) {
    entry.mods_path = str::trim(mods.string("path"));
    entry.mods_layout = str::trim(mods.string("layout"));
    const Value sources = mods.get("sources");
    if (sources.is_array()) {
      for (size_t i = 0; i < sources.size(); ++i) {
        const Value item = sources.at(i);
        if (!item.is_object()) continue;
        ModSource mod;
        const std::string provider = str::lower(str::trim(item.string("provider")));
        if (!provider.empty()) mod.provider = provider;
        mod.source_url = str::trim(item.string("sourceUrl"));
        if (mod.source_url.empty()) continue;  // sources sin sourceUrl: inservibles
        entry.mods_sources.push_back(mod);
      }
    }
  }

  // ---- extensiones Delanx ----
  entry.sw = parse_switch_install(node.get("switch"));
  entry.requirements = node.string("requirements");
  entry.mrship = node.boolean("mrship", node.boolean("harbourMasters", false));
  entry.verified_entry = node.boolean("verified", false);

  // Entrada sin repositorio = app gestionada a mano: no se descarga ni se actualiza nunca.
  if (entry.manually_managed()) {
    entry.repository_source.clear();
    entry.auto_update = false;
    entry.preferred_version.clear();
    entry.skipped_update_version.clear();
    entry.release_asset_filter.clear();
    entry.defer_update_tracking = false;
  }
  return entry;
}

json::Builder AppEntry::to_json() const {
  Builder root = Builder::object();

  // Claves que Quiver siempre espera.
  root.set("name", name);
  root.set("folderName", folder_name);
  root.set("installPath", install_path);
  root.set("appIconUrl", app_icon_url);

  if (!manually_managed()) {
    root.set("repository", repository);
    if (!repository_source.empty() && !str::iequals(repository_source, "github")) {
      root.set("repositorySource", str::lower(repository_source));
    }
    // Quiver escribe ambas aunque no haya pin: null = sin preferencia.
    if (preferred_version.empty()) {
      root.set_null("preferredVersion");
    } else {
      root.set("preferredVersion", preferred_version);
    }
    if (skipped_update_version.empty()) {
      root.set_null("skippedUpdateVersion");
    } else {
      root.set("skippedUpdateVersion", skipped_update_version);
    }
    if (auto_update) root.set("autoUpdate", true);
    if (defer_update_tracking) root.set("deferUpdateTracking", true);
    if (!release_asset_filter.empty()) root.set("releaseAssetFilter", release_asset_filter);
  }

  if (!project.empty()) root.set("project", project);
  if (!custom_display_name.empty()) root.set("customDisplayName", custom_display_name);
  if (!tags.empty()) root.set_strings("tags", tags);
  if (!catalog_entry_id.empty()) root.set("catalogEntryId", catalog_entry_id);

  if (catalog) {
    Builder snapshot = Builder::object();
    snapshot.set("name", catalog->name);
    if (!catalog->project.empty()) snapshot.set("project", catalog->project);
    if (!catalog->app_icon_url.empty()) snapshot.set("appIconUrl", catalog->app_icon_url);
    if (!catalog->tags.empty()) snapshot.set_strings("tags", catalog->tags);
    root.set_json("catalog", std::move(snapshot));
  }

  if (!files_to_add.empty()) root.set_strings("filesToAdd", files_to_add);

  if (!mods_path.empty() || !mods_layout.empty() || !mods_sources.empty()) {
    Builder mods = Builder::object();
    if (!mods_path.empty()) mods.set("path", mods_path);
    if (!mods_layout.empty()) mods.set("layout", mods_layout);
    if (!mods_sources.empty()) {
      Builder sources = Builder::array();
      for (const ModSource& source : mods_sources) {
        Builder item = Builder::object();
        item.set("provider", source.provider);
        item.set("sourceUrl", source.source_url);
        sources.append_json(std::move(item));
      }
      mods.set_json("sources", std::move(sources));
    }
    root.set_json("mods", std::move(mods));
  }

  // Bloque propio "switch": solo claves con informacion.
  Builder switch_block = Builder::object();
  bool has_switch_data = false;
  if (!sw.target_dir.empty()) {
    switch_block.set("targetDir", sw.target_dir);
    has_switch_data = true;
  }
  // Solo si tiene valor: apps.json sigue siendo identico al de Quiver cuando no se usa.
  if (!sw.home_dir.empty()) {
    switch_block.set("homeDir", sw.home_dir);
    has_switch_data = true;
  }
  if (!sw.nro.empty()) {
    switch_block.set("nro", sw.nro);
    has_switch_data = true;
  }
  if (!sw.nro_path.empty()) {
    switch_block.set("nroPath", sw.nro_path);
    has_switch_data = true;
  }
  if (sw.strip_root) {
    switch_block.set("stripRoot", true);
    has_switch_data = true;
  }
  if (sw.has_user_files()) {
    Builder files = Builder::array();
    for (const SwitchInstall::RequiredFile& file : sw.required_user_files) {
      Builder item = Builder::object();
      item.set("path", file.path);
      if (!file.description.empty()) item.set("description", file.description);
      files.append_json(std::move(item));
    }
    switch_block.set_json("requiredUserFiles", std::move(files));
    has_switch_data = true;
  }
  if (has_switch_data) root.set_json("switch", std::move(switch_block));

  // Metadatos de catalogo: solo si aportan algo (nada de nulls ensuciando el fichero).
  if (!requirements.empty()) root.set("requirements", requirements);
  if (mrship) root.set("mrship", true);
  if (verified_entry) root.set("verified", true);

  return root;
}

std::string AppEntry::identity_key() const {
  if (manually_managed()) return "manual:" + folder_name;
  std::string source = str::lower(str::trim(repository_source));
  if (source.empty()) source = "github";  // ausente = GitHub, igual que Quiver
  return source + ":" + repository + ":" + folder_name;
}

std::string AppEntry::display_name() const {
  if (!custom_display_name.empty()) return custom_display_name;
  if (!name.empty()) return name;
  return folder_name;  // ultimo recurso para que la UI siempre tenga algo que pintar
}

std::string AppEntry::install_dir() const {
  if (!install_path.empty()) return install_path;
  return sw.effective_target(paths::apps_dir(), folder_name);
}

std::string AppEntry::version_file() const { return paths::join(install_dir(), "version.txt"); }

std::string AppEntry::installed_build_file() const {
  return paths::join(install_dir(), "installed-build.txt");
}

std::string AppEntry::icon_cache_path() const {
  return paths::join(paths::artwork_dir(), folder_name + ".png");
}

void AppEntry::refresh_install_state() {
  installed_version.clear();
  installed = false;
  if (install_path.empty() && folder_name.empty() && sw.target_dir.empty() && sw.nro.empty()) return;

  std::vector<std::string> candidates;
  const std::string primary_dir = install_dir();
  if (!primary_dir.empty()) candidates.push_back(primary_dir);
  if (!sw.target_dir.empty()) candidates.push_back(paths::to_sd_path(sw.target_dir));
  if (!sw.home_dir.empty()) candidates.push_back(paths::to_sd_path(sw.home_dir));
  if (!folder_name.empty()) {
    candidates.push_back(paths::to_sd_path("/switch/" + folder_name));
    candidates.push_back(paths::to_sd_path("/switch/" + str::lower(folder_name)));
  }
  if (!sw.target_dir.empty()) {
    const std::string base = paths::basename(sw.target_dir);
    candidates.push_back(paths::to_sd_path("/switch/" + base));
    candidates.push_back(paths::to_sd_path("/switch/" + str::lower(base)));
  }
  if (!project.empty()) {
    candidates.push_back(paths::to_sd_path("/switch/" + project));
    candidates.push_back(paths::to_sd_path("/switch/" + str::lower(project)));
  }
  if (!sw.nro.empty()) {
    candidates.push_back(paths::to_sd_path("/switch/" + sw.nro));
    candidates.push_back(paths::to_sd_path("/switch/" + str::lower(sw.nro)));
  }

  for (const std::string& dir : candidates) {
    if (dir.empty()) continue;
    // Evitar raices compartidas de la SD
    const std::string lower_dir = str::lower(dir);
    if (lower_dir == str::lower(paths::to_sd_path("/")) ||
        lower_dir == str::lower(paths::to_sd_path("/switch")) ||
        lower_dir == str::lower(paths::to_sd_path("/switch/"))) {
      continue;
    }

    const std::string vfile = paths::join(dir, "version.txt");
    const bool has_version_file = paths::exists(vfile);
    if (has_version_file) {
      if (const auto text = paths::read_file(vfile)) {
        installed_version = str::trim(*text);
      }
    }
    if (has_version_file || has_launch_nro(sw, dir)) {
      installed = true;
      install_path = dir;
      return;
    }
  }
}

std::vector<SwitchInstall::RequiredFile> AppEntry::missing_user_files() const {
  std::vector<SwitchInstall::RequiredFile> missing;
  for (const SwitchInstall::RequiredFile& file : sw.required_user_files) {
    const std::string path = paths::to_sd_path(file.path);
    if (path.empty() || paths::exists(path)) continue;
    SwitchInstall::RequiredFile copy = file;
    copy.path = path;  // ruta lista para la SD, para mostrarla o descargarla
    missing.push_back(copy);
  }
  return missing;
}

bool AppEntry::update_available_for(const std::string& latest_tag) const {
  if (manually_managed()) return false;        // sin repositorio no hay nada que actualizar
  if (latest_tag.empty()) return false;
  if (!installed) return false;                // no esta instalada: sera una instalacion
  if (installed_version.empty()) return true;  // instalada pero sin version registrada
  if (installed_version == latest_tag) return false;
  if (!skipped_update_version.empty() && skipped_update_version == latest_tag) return false;
  return true;
}

std::string compose_display_name(const AppEntry& entry, LibraryNameStyle style) {
  // El nombre personalizado manda sobre cualquier estilo.
  if (!entry.custom_display_name.empty()) return entry.custom_display_name;
  if (!entry.custom_display_name.empty()) return entry.custom_display_name;

  const std::string name = entry.name.empty() ? entry.folder_name : entry.name;
  const std::string project = entry.project;
  if (project.empty() || name.empty()) return name.empty() ? project : name;

  // Los separadores son una decision de UI (no afectan a la compatibilidad de datos).
  switch (style) {
    case LibraryNameStyle::NameOnly:
      return name;
    case LibraryNameStyle::NameAndProject:
      return name + " (" + project + ")";
    case LibraryNameStyle::ProjectOnly:
      return project;
    case LibraryNameStyle::NameAndProjectInTitle:
      return name + " - " + project;
    case LibraryNameStyle::ProjectAndName:
      return project + " - " + name;
  }
  return name;
}

// ---------------------------------------------------------------------------
// Library
// ---------------------------------------------------------------------------

std::optional<Library> Library::load(const std::string& path) {
  const auto doc = json::Doc::parse_file(path);
  if (!doc) return std::nullopt;  // fichero ausente o JSON invalido: el llamante decide

  Library library;
  const Value root = doc->root();
  if (root.is_array()) {
    // Formato legado: la raiz ya es la lista de apps.
    append_apps(root, library.apps);
    return library;
  }
  if (!root.is_object()) return std::nullopt;

  append_apps(root.get("apps"), library.apps);
  // Secciones legadas de Quiver (versiones antiguas del fichero).
  append_apps(root.get("standard"), library.apps);
  append_apps(root.get("experimental"), library.apps);
  append_apps(root.get("custom"), library.apps);
  return library;
}

bool Library::save(const std::string& path) const {
  Builder apps = Builder::array();
  for (const AppEntry& app : this->apps) {
    apps.append_json(app.to_json());
  }
  Builder root = Builder::object();
  root.set_json("apps", std::move(apps));
  return save_builder(path, root);
}

AppEntry* Library::find(const std::string& name_or_display) {
  const std::string query = str::trim(name_or_display);
  if (query.empty()) return nullptr;
  for (AppEntry& app : apps) {
    if (str::iequals(app.name, query) || str::iequals(app.custom_display_name, query) ||
        str::iequals(app.display_name(), query) || str::iequals(app.folder_name, query)) {
      return &app;
    }
  }
  return nullptr;
}

AppEntry* Library::find_by_identity(const std::string& key) {
  if (key.empty()) return nullptr;
  for (AppEntry& app : apps) {
    if (app.identity_key() == key) return &app;
  }
  return nullptr;
}

void Library::upsert(const AppEntry& entry) {
  const std::string key = entry.identity_key();
  for (AppEntry& app : apps) {
    if (app.identity_key() == key) {
      app = entry;
      return;
    }
  }
  apps.push_back(entry);
}

bool Library::remove_by_identity(const std::string& key) {
  if (key.empty()) return false;
  for (auto it = apps.begin(); it != apps.end(); ++it) {
    if (it->identity_key() == key) {
      apps.erase(it);
      return true;
    }
  }
  return false;
}

void Library::sort_by_name() {
  // stable_sort: con nombres repetidos se conserva el orden previo del fichero.
  std::stable_sort(apps.begin(), apps.end(), [](const AppEntry& a, const AppEntry& b) {
    return str::lower(a.display_name()) < str::lower(b.display_name());
  });
}

}  // namespace delanx

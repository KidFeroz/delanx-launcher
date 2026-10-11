// Delanx — entrada de biblioteca compatible con apps.json de Quiver Launcher.
#pragma once

#include <optional>
#include <string>
#include <vector>

#include "core/json.hpp"

namespace delanx {

struct ModSource {
  std::string provider = "thunderstore";  // "thunderstore" | "gamebanana"
  std::string source_url;
};

struct CatalogSnapshot {
  std::string name;
  std::string project;
  std::string app_icon_url;
  std::vector<std::string> tags;
};

// Bloque "switch": extension propia de Delanx. Quiver Launcher ignora las claves que
// no conoce, asi que el mismo apps.json sigue siendo valido en escritorio.
struct SwitchInstall {
  // targetDir = raiz de extraccion: donde se descomprime el asset (puede ser "/" si el zip
  // ya trae el prefijo switch/).
  std::string target_dir;  // "sdmc:/switch/soh" — donde se extrae el release
  // homeDir = carpeta final de la app en la SD: donde vive el .nro y sus datos. Normalmente
  // coincide con target_dir; se distinguen cuando el zip trae una estructura propia (por
  // ejemplo targetDir="/" y homeDir="/switch/soh"). Solo es informativo: la extraccion sigue
  // usando target_dir. Se escribe en apps.json solo si tiene valor, para no anadir claves
  // vacias al fichero que comparte con Quiver.
  std::string home_dir;    // "sdmc:/switch/soh" — carpeta final de la app
  std::string nro;         // "soh.nro"      — ejecutable a lanzar
  std::string nro_path;    // "switch/soh/soh.nro" — ruta dentro del asset
  bool strip_root = false; // el zip envuelve todo en una carpeta raiz

  struct RequiredFile {
    std::string path;         // "sdmc:/switch/soh/oot.otr"
    std::string description;  // explicacion para el usuario
  };
  std::vector<RequiredFile> required_user_files;

  bool empty() const { return target_dir.empty() && nro.empty() && nro_path.empty(); }
  bool has_user_files() const { return !required_user_files.empty(); }
  // target_dir, o <apps_dir>/<folder_name> si el catalogo no fija ruta nativa.
  std::string effective_target(const std::string& apps_dir, const std::string& folder_name) const;
};

struct AppEntry {
  // ---- contrato Quiver (apps.json) ----
  std::string name;
  std::string project;
  std::string custom_display_name;
  std::string repository;          // "owner/repo" o "namespace/project"; vacio = manual
  std::string repository_source;   // "github" (por defecto) | "gitlab"
  std::string folder_name;
  std::string install_path;
  std::string app_icon_url;
  std::string preferred_version;
  std::string skipped_update_version;
  bool auto_update = false;
  bool defer_update_tracking = false;
  std::vector<std::string> tags;
  std::vector<std::string> files_to_add;
  std::string release_asset_filter;
  std::string catalog_entry_id;
  std::optional<CatalogSnapshot> catalog;
  std::string mods_path;
  std::string mods_layout;  // "flat" (por defecto) | "folderPerMod"
  std::vector<ModSource> mods_sources;

  // ---- extensiones Delanx ----
  SwitchInstall sw;
  std::string requirements;
  bool harbour_masters = false;
  bool verified_entry = false;

  // ---- estado en tiempo de ejecucion (no se serializa) ----
  bool installed = false;
  std::string installed_version;
  std::string latest_version;
  bool update_available = false;
  std::string last_error;

  bool manually_managed() const { return repository.empty(); }
  std::string identity_key() const;
  std::string display_name() const;
  std::string install_dir() const;
  std::string version_file() const;
  std::string installed_build_file() const;
  std::string icon_cache_path() const;

  // Lee version.txt del disco y comprueba que exista el .nro de lanzamiento.
  void refresh_install_state();
  // Ficheros que el usuario debe aportar y aun no estan en la SD.
  std::vector<SwitchInstall::RequiredFile> missing_user_files() const;

  // true si hay una actualizacion pendiente hacia `latest_tag`. Se compara el tag instalado
  // tal cual (igual que Quiver: no hay comparacion semantica de versiones), se respeta la
  // version que el jugador decidio saltarse y una app manual nunca se actualiza.
  bool update_available_for(const std::string& latest_tag) const;

  static AppEntry from_json(const json::Value& node);
  json::Builder to_json() const;
};

struct Library {
  std::vector<AppEntry> apps;

  static std::optional<Library> load(const std::string& path);
  bool save(const std::string& path) const;

  AppEntry* find(const std::string& name_or_display);
  AppEntry* find_by_identity(const std::string& key);
  void upsert(const AppEntry& entry);
  bool remove_by_identity(const std::string& key);
  void sort_by_name();
};

// Estilo de nombre de la biblioteca, definido en model/settings.hpp. Se declara aqui de
// forma opaca para no acoplar AppEntry a Settings (y evitar un include circular).
enum class LibraryNameStyle : int;

// Nombre visible segun el estilo configurado; respeta siempre customDisplayName.
std::string compose_display_name(const AppEntry& entry, LibraryNameStyle style);

}  // namespace delanx

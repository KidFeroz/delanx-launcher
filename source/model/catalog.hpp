// Delanx — catalogo propio y modelo de releases.
#pragma once

#include <map>
#include <string>
#include <vector>

#include "core/json.hpp"
#include "model/app_entry.hpp"

namespace delanx {

struct ReleaseAsset {
  std::string name;
  std::string url;
  std::string digest;  // "sha256:<hex>" publicado por GitHub, si existe
  int64_t size = 0;

  bool is_archive() const;
  bool is_single_nro() const;
  // Marcador positivo de Switch: nombre con .nro, o "switch"/"nx" y no una plataforma ajena.
  bool looks_like_switch() const;
  // Marcadores de escritorio/consola ajena que Quiver tambien excluye.
  bool looks_like_foreign_platform() const;
};

struct Release {
  enum class Check { Unknown, Verified, Unverified, Blocked };

  std::string tag;
  std::string name;
  std::string body;
  std::string published_at;  // ISO 8601 tal cual lo devuelve la API
  int64_t published_unix = 0;
  bool prerelease = false;
  std::vector<ReleaseAsset> assets;

  // Verificacion estilo Quiver.
  Check check = Check::Unknown;
  std::vector<std::string> check_reasons;
  std::string verified_version;
  bool rolling = false;
  std::map<std::string, std::string> checksums;  // nombre de asset -> hex

  const ReleaseAsset* find_asset(const std::string& name) const;
  std::string checksum_for(const std::string& asset_name) const;
  bool flagged() const { return false; }
};

struct CatalogEntry {
  std::string name;
  std::string project;
  std::string repository;
  std::string repository_source = "github";
  std::string folder_name;
  std::vector<std::string> tags;
  std::string release_asset_filter;
  std::string requirements;
  std::string app_icon_url;
  // Metadatos informativos del catalogo (sobre todo del catalogo maestro del usuario): como se
  // publica el port y de donde sale la ROM. Opcionales: si la clave no esta, quedan vacios y
  // la UI simplemente no los pinta. No afectan a la instalacion.
  std::string category;         // clave "category"
  std::string asset_type;       // clave "assetType" (rom_extract, archive_extract_o2r...)
  std::string rom_source_hint;  // clave "romSourceHint" (de donde sacar la ROM propia)
  std::vector<std::string> switch_proof;
  bool verified = false;
  bool mrship = false;
  std::string notes;
  SwitchInstall sw;

  static CatalogEntry from_json(const json::Value& node);
  // Convierte la entrada de catalogo en una entrada de biblioteca lista para instalar.
  AppEntry to_app_entry() const;
  std::string badge() const;  // etiqueta corta para la UI
};

struct UnavailableEntry {
  std::string name;
  std::string repository;
  std::string reason;
  std::string last_checked;
  std::string workaround;
};

struct Catalog {
  std::vector<CatalogEntry> entries;
  std::vector<UnavailableEntry> unavailable;

  static std::optional<Catalog> load_file(const std::string& path);
  static std::optional<Catalog> load_romfs(const char* relative_name);
};

}  // namespace delanx

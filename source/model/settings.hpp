// Delanx — ajustes. Se conservan los nombres y tipos de Quiver (PascalCase en JSON,
// enums numericos) para que el fichero sea intercambiable con el escritorio.
#pragma once

#include <string>
#include <utility>
#include <vector>

#include "core/json.hpp"

namespace delanx {

enum class ThemeTextMode : int { Automatic = 0, Light = 1, Dark = 2 };
enum class LibraryNameStyle : int {
  NameOnly = 0,
  NameAndProject = 1,
  ProjectOnly = 2,
  NameAndProjectInTitle = 3,
  ProjectAndName = 4,
};
enum class ListScope : int { AllApps = 0, InstalledOnly = 1, HiddenOnly = 2 };

struct Settings {
  // ---- claves compatibles con Quiver ----
  bool FirstStartup = true;
  bool IconFill = false;
  bool UseGridView = true;
  int IconSize = 124;
  int SlotSize = 180;
  int ListRowHeight = 96;
  std::string PrimaryColor = "#18181b";
  std::string SecondaryColor = "#404040";
  ThemeTextMode ThemeMode = ThemeTextMode::Automatic;
  int Platform = 0;
  std::string SortBy = "LastPlayed";
  std::vector<std::string> HiddenApps;
  std::string GitHubApiToken;
  std::string GitLabApiToken;
  LibraryNameStyle NameStyle = LibraryNameStyle::NameAndProject;
  ListScope Scope = ListScope::AllApps;
  bool BackgroundUpdateCheckEnabled = false;
  int BackgroundUpdateCheckIntervalMinutes = 1440;
  bool AutoUpdateNewlyAddedApps = false;
  std::string CustomAppListLocation;
  std::string CatalogAiFilter;
  bool CatalogHideLibraryApps = true;
  bool EnableGamepadInput = true;

  // ---- extensiones Delanx ----
  std::string DelanxCatalogBaseUrl = "https://api.quiverlauncher.com/api/v1";
  bool DelanxHarbourMastersSection = true;
  bool DelanxIncludeUnverified = true;
  bool DelanxUseNativeInstallPaths = true;
  bool DelanxAutoCheckUpdatesOnBoot = false;
  bool DelanxConfirmBlockedReleases = true;
  std::string DelanxLastTab = "library";
  std::string DelanxTheme = "sphaira-dark";
  std::string DelanxLanguage = "es";
  int DelanxGridColumns = 0;  // 0 = automatico segun modo de arranque

  // Claves del fichero que esta version no conoce: se guardan como JSON literal para
  // reescribirlas tal cual y no borrar ajustes de Quiver (u otra version de Delanx).
  std::vector<std::pair<std::string, std::string>> Extra;

  static Settings load(const std::string& path);
  bool save(const std::string& path) const;
  static Settings from_json(const json::Value& node);
  json::Builder to_json() const;

  // Igual que AppSettings.EnsureInitialized(): rellena valores derivados y limita rangos.
  void ensure_initialized();
  bool is_dark() const;
};

}  // namespace delanx

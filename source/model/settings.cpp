// Delanx — Settings sobre settings.json: nombres PascalCase y enums numericos de Quiver
// mas las claves propias Delanx*.
#include "model/settings.hpp"

#include <cstdlib>
#include <cstring>
#include <utility>

#include "core/paths.hpp"

namespace delanx {
namespace {

// Claves desconocidas conservadas como JSON literal (no solo el texto del valor): asi se
// reescriben con su tipo original y no se pierde nada de lo que escriba Quiver u otra
// version de Delanx.
using ExtraPairs = std::vector<std::pair<std::string, std::string>>;

const char* const kKnownKeys[] = {
    "FirstStartup",
    "IconFill",
    "UseGridView",
    "IconSize",
    "SlotSize",
    "ListRowHeight",
    "PrimaryColor",
    "SecondaryColor",
    "ThemeTextMode",
    "Platform",
    "SortBy",
    "HiddenApps",
    "GitHubApiToken",
    "GitLabApiToken",
    "LibraryNameStyle",
    "ListScope",
    "BackgroundUpdateCheckEnabled",
    "BackgroundUpdateCheckIntervalMinutes",
    "AutoUpdateNewlyAddedApps",
    "CustomAppListLocation",
    "CatalogAiFilter",
    "CatalogHideLibraryApps",
    "EnableGamepadInput",
    "DelanxCatalogBaseUrl",
    "DelanxMrShipSection",
    "DelanxHarbourMastersSection",
    "DelanxIncludeUnverified",
    "DelanxUseNativeInstallPaths",
    "DelanxAutoCheckUpdatesOnBoot",
    "DelanxConfirmBlockedReleases",
    "DelanxLastTab",
    "DelanxTheme",
    "DelanxLanguage",
    "DelanxGridColumns",
    "DelanxFavorites",
};

bool is_known_key(const char* key) {
  for (const char* known : kKnownKeys) {
    if (std::strcmp(known, key) == 0) return true;
  }
  return false;
}

void keep_raw(const std::string& key, const json::Value& value, ExtraPairs& extra) {
  if (!value.valid()) return;
  char* dumped = json_dumps(value.raw(), JSON_COMPACT | JSON_ENCODE_ANY);
  if (!dumped) return;
  extra.emplace_back(key, std::string(dumped));
  free(dumped);
}

// Lectores tolerantes: si la clave aparece con un tipo que no podemos representar sin
// cambiarlo (p.ej. PrimaryColor numerico), se conserva tal cual en Extra en lugar de
// descartarla. Quiver la volvera a encontrar identica.
void read_bool(const json::Value& node, const char* key, bool& out, ExtraPairs& extra) {
  const json::Value value = node.get(key);
  if (!value.valid()) return;
  if (value.is_bool()) {
    out = value.as_boolean(out);
  } else {
    keep_raw(key, value, extra);
  }
}

void read_int(const json::Value& node, const char* key, int& out, ExtraPairs& extra) {
  const json::Value value = node.get(key);
  if (!value.valid()) return;
  if (value.is_number()) {
    out = static_cast<int>(value.as_integer(out));
  } else {
    keep_raw(key, value, extra);
  }
}

void read_string(const json::Value& node, const char* key, std::string& out, ExtraPairs& extra) {
  const json::Value value = node.get(key);
  if (!value.valid()) return;
  if (value.is_string()) {
    out = value.as_string(out);
  } else {
    keep_raw(key, value, extra);
  }
}

void read_strings(const json::Value& node, const char* key, std::vector<std::string>& out,
                  ExtraPairs& extra) {
  const json::Value value = node.get(key);
  if (!value.valid()) return;
  if (value.is_array()) {
    out = node.string_array(key);
  } else {
    keep_raw(key, value, extra);
  }
}

// Builder no expone forma de adoptar su json_t* en un Doc, asi que se serializa a texto y
// se delega en Doc::save_file: mismo formato y escritura atomica (tmp + rename).
bool save_builder(const std::string& path, const json::Builder& root) {
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

}  // namespace

Settings Settings::from_json(const json::Value& node) {
  Settings settings;
  if (!node.is_object()) return settings;
  ExtraPairs& extra = settings.Extra;

  // ---- claves compatibles con Quiver ----
  read_bool(node, "FirstStartup", settings.FirstStartup, extra);
  read_bool(node, "IconFill", settings.IconFill, extra);
  read_bool(node, "UseGridView", settings.UseGridView, extra);
  read_int(node, "IconSize", settings.IconSize, extra);
  read_int(node, "SlotSize", settings.SlotSize, extra);
  read_int(node, "ListRowHeight", settings.ListRowHeight, extra);
  read_string(node, "PrimaryColor", settings.PrimaryColor, extra);
  read_string(node, "SecondaryColor", settings.SecondaryColor, extra);
  read_string(node, "SortBy", settings.SortBy, extra);
  read_strings(node, "HiddenApps", settings.HiddenApps, extra);
  read_string(node, "GitHubApiToken", settings.GitHubApiToken, extra);
  read_string(node, "GitLabApiToken", settings.GitLabApiToken, extra);
  read_bool(node, "BackgroundUpdateCheckEnabled", settings.BackgroundUpdateCheckEnabled, extra);
  read_int(node, "BackgroundUpdateCheckIntervalMinutes",
           settings.BackgroundUpdateCheckIntervalMinutes, extra);
  read_bool(node, "AutoUpdateNewlyAddedApps", settings.AutoUpdateNewlyAddedApps, extra);
  read_string(node, "CustomAppListLocation", settings.CustomAppListLocation, extra);
  read_string(node, "CatalogAiFilter", settings.CatalogAiFilter, extra);
  read_bool(node, "CatalogHideLibraryApps", settings.CatalogHideLibraryApps, extra);
  read_bool(node, "EnableGamepadInput", settings.EnableGamepadInput, extra);
  read_int(node, "Platform", settings.Platform, extra);

  // Enums de Quiver: se leen como entero y se sanean si el valor no existe en la version
  // actual (un enum fuera de rango haria que la UI tomase decisiones sin sentido).
  int theme_mode = static_cast<int>(settings.ThemeMode);
  read_int(node, "ThemeTextMode", theme_mode, extra);
  if (theme_mode < static_cast<int>(ThemeTextMode::Automatic) ||
      theme_mode > static_cast<int>(ThemeTextMode::Dark)) {
    theme_mode = static_cast<int>(ThemeTextMode::Automatic);
  }
  settings.ThemeMode = static_cast<ThemeTextMode>(theme_mode);

  int name_style = static_cast<int>(settings.NameStyle);
  read_int(node, "LibraryNameStyle", name_style, extra);
  if (name_style < static_cast<int>(LibraryNameStyle::NameOnly) ||
      name_style > static_cast<int>(LibraryNameStyle::ProjectAndName)) {
    name_style = static_cast<int>(LibraryNameStyle::NameAndProject);
  }
  settings.NameStyle = static_cast<LibraryNameStyle>(name_style);

  int scope = static_cast<int>(settings.Scope);
  read_int(node, "ListScope", scope, extra);
  if (scope < static_cast<int>(ListScope::AllApps) ||
      scope > static_cast<int>(ListScope::HiddenOnly)) {
    scope = static_cast<int>(ListScope::AllApps);
  }
  settings.Scope = static_cast<ListScope>(scope);

  // ---- extensiones Delanx ----
  read_string(node, "DelanxCatalogBaseUrl", settings.DelanxCatalogBaseUrl, extra);
  read_bool(node, "DelanxMrShipSection", settings.DelanxMrShipSection, extra);
  if (node.get("DelanxHarbourMastersSection").valid() && !node.get("DelanxMrShipSection").valid()) {
    read_bool(node, "DelanxHarbourMastersSection", settings.DelanxMrShipSection, extra);
  }
  read_bool(node, "DelanxIncludeUnverified", settings.DelanxIncludeUnverified, extra);
  read_bool(node, "DelanxUseNativeInstallPaths", settings.DelanxUseNativeInstallPaths, extra);
  read_bool(node, "DelanxAutoCheckUpdatesOnBoot", settings.DelanxAutoCheckUpdatesOnBoot, extra);
  read_bool(node, "DelanxConfirmBlockedReleases", settings.DelanxConfirmBlockedReleases, extra);
  read_string(node, "DelanxLastTab", settings.DelanxLastTab, extra);
  read_string(node, "DelanxTheme", settings.DelanxTheme, extra);
  read_string(node, "DelanxLanguage", settings.DelanxLanguage, extra);
  read_int(node, "DelanxGridColumns", settings.DelanxGridColumns, extra);
  read_strings(node, "DelanxFavorites", settings.DelanxFavorites, extra);

  // ---- claves que no conocemos: se conservan para reescribirlas igual ----
  json_t* object = node.raw();
  if (json_is_object(object)) {
    void* iter = json_object_iter(object);
    while (iter) {
      const char* key = json_object_iter_key(iter);
      json_t* value = json_object_iter_value(iter);
      if (key && value && !is_known_key(key)) keep_raw(key, json::Value(value), extra);
      iter = json_object_iter_next(object, iter);
    }
  }
  return settings;
}

json::Builder Settings::to_json() const {
  json::Builder root = json::Builder::object();

  // ---- claves compatibles con Quiver (PascalCase) ----
  root.set("FirstStartup", FirstStartup);
  root.set("IconFill", IconFill);
  root.set("UseGridView", UseGridView);
  root.set("IconSize", static_cast<int64_t>(IconSize));
  root.set("SlotSize", static_cast<int64_t>(SlotSize));
  root.set("ListRowHeight", static_cast<int64_t>(ListRowHeight));
  root.set("PrimaryColor", PrimaryColor);
  root.set("SecondaryColor", SecondaryColor);
  root.set("ThemeTextMode", static_cast<int64_t>(static_cast<int>(ThemeMode)));
  root.set("Platform", static_cast<int64_t>(Platform));
  root.set("SortBy", SortBy);
  root.set_strings("HiddenApps", HiddenApps);
  root.set("GitHubApiToken", GitHubApiToken);
  root.set("GitLabApiToken", GitLabApiToken);
  root.set("LibraryNameStyle", static_cast<int64_t>(static_cast<int>(NameStyle)));
  root.set("ListScope", static_cast<int64_t>(static_cast<int>(Scope)));
  root.set("BackgroundUpdateCheckEnabled", BackgroundUpdateCheckEnabled);
  root.set("BackgroundUpdateCheckIntervalMinutes",
           static_cast<int64_t>(BackgroundUpdateCheckIntervalMinutes));
  root.set("AutoUpdateNewlyAddedApps", AutoUpdateNewlyAddedApps);
  root.set("CustomAppListLocation", CustomAppListLocation);
  root.set("CatalogAiFilter", CatalogAiFilter);
  root.set("CatalogHideLibraryApps", CatalogHideLibraryApps);
  root.set("EnableGamepadInput", EnableGamepadInput);

  // ---- extensiones Delanx ----
  root.set("DelanxCatalogBaseUrl", DelanxCatalogBaseUrl);
  root.set("DelanxMrShipSection", DelanxMrShipSection);
  root.set("DelanxIncludeUnverified", DelanxIncludeUnverified);
  root.set("DelanxUseNativeInstallPaths", DelanxUseNativeInstallPaths);
  root.set("DelanxAutoCheckUpdatesOnBoot", DelanxAutoCheckUpdatesOnBoot);
  root.set("DelanxConfirmBlockedReleases", DelanxConfirmBlockedReleases);
  root.set("DelanxLastTab", DelanxLastTab);
  root.set("DelanxTheme", DelanxTheme);
  root.set("DelanxLanguage", DelanxLanguage);
  root.set("DelanxGridColumns", static_cast<int64_t>(DelanxGridColumns));
  root.set_strings("DelanxFavorites", DelanxFavorites);

  // Claves heredadas de otras versiones, aplicadas al final: si una clave conocida venia
  // con un tipo que no representamos, su valor original gana sobre el nuestro.
  json_t* object = root.value().raw();
  if (object) {
    for (const auto& pair : Extra) {
      const auto doc = json::Doc::parse(pair.second);
      if (!doc) continue;
      json_t* value = doc->root().raw();
      if (!value) continue;
      json_incref(value);
      json_object_set_new(object, pair.first.c_str(), value);
    }
  }
  return root;
}

void Settings::ensure_initialized() {
  // ListRowHeight: 96 por defecto; fuera del rango legible se limita a 72..400.
  if (ListRowHeight <= 0) ListRowHeight = 96;
  if (ListRowHeight < 72) ListRowHeight = 72;
  if (ListRowHeight > 400) ListRowHeight = 400;

  // Comprobar actualizaciones cada menos de 15 minutos no tiene sentido (y castiga la API).
  if (BackgroundUpdateCheckIntervalMinutes < 15) BackgroundUpdateCheckIntervalMinutes = 15;

  // DelanxGridColumns: 0 = automatico segun el modo de arranque.
  if (DelanxGridColumns < 0) DelanxGridColumns = 0;
  if (DelanxGridColumns > 6) DelanxGridColumns = 6;

  if (DelanxLastTab.empty()) DelanxLastTab = "library";
}

bool Settings::is_dark() const {
  // Automatic se resuelve a oscuro: es el tema por defecto de la consola y del menu de Switch.
  return ThemeMode == ThemeTextMode::Dark || ThemeMode == ThemeTextMode::Automatic;
}

Settings Settings::load(const std::string& path) {
  Settings settings;  // valores por defecto si el fichero falta o esta corrupto
  const auto doc = json::Doc::parse_file(path);
  if (doc) {
    const json::Value root = doc->root();
    if (root.is_object()) settings = Settings::from_json(root);
  }
  settings.ensure_initialized();
  return settings;
}

bool Settings::save(const std::string& path) const {
  json::Builder root = to_json();
  return save_builder(path, root);
}

}  // namespace delanx

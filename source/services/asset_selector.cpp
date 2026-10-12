// Delanx — implementacion de la eleccion de asset de Switch.
//
// IMPORTANTE — este predicado es el INVERSO del PlatformAssetMatcher de Quiver Launcher.
// Quiver es un launcher de escritorio: su matcher usa "switch" como marcador de plataforma
// AJENA y descarta deliberadamente cualquier asset cuyo nombre lo contenga (junto con otros
// marcadores como wiiu, ps4, xbox, .apk o .dmg). Delanx corre en Switch, asi que invierte la
// regla: "switch"/"nx" y la extension .nro son los marcadores POSITIVOS, y los marcadores de
// escritorio/otras consolas son los que se descartan. Por eso aqui nunca se reutiliza el
// matcher de Quiver: se descartan .exe/.msi/.appimage/.apk/.dmg/.vpk/.wiiu/.3ds, etc.
//
// Reglas de seleccion (ver docs/ARCHITECTURE.md §4):
//   a) el nombre debe pasar releaseAssetFilter (str::asset_matches_filter);
//   b) no puede parecer de otra plataforma;
//   c) debe ser `looks_like_switch()`, un archivo comprimido o un unico .nro;
//   d) con filtro vacio se exige ademas `looks_like_switch()`.
// Preferencia: primero los .nro sueltos; entre archivos, los que llevan "switch" en el nombre.

#include "services/asset_selector.hpp"

#include <string>
#include <vector>

#include "core/paths.hpp"
#include "core/strings.hpp"

namespace delanx {
namespace {

int score_candidate(const ReleaseAsset& asset) {
  if (asset.is_single_nro()) return 3;
  if (asset.is_archive() && str::icontains(asset.name, "switch")) return 2;
  if (asset.is_archive()) return 1;
  return 0;
}

std::string display_name(const ReleaseAsset& asset) {
  if (!asset.name.empty()) return asset.name;
  const std::string from_url = paths::basename(asset.url);
  return from_url.empty() ? std::string("(sin nombre)") : from_url;
}

// Mensaje cuando la release no publica NINGUN asset que se pueda usar.
std::string describe_missing_asset(const Release& release, const std::string& filter) {
  const std::string version = release.tag.empty() ? std::string("esta version") : release.tag;
  if (!filter.empty()) {
    return str::format(
        "La release %s no publica ningun asset que cumpla el filtro \"%s\": los ports de "
        "MrShip dejaron de incluir builds de Switch a partir de cierta version. "
        "Puedes probar otra version anterior o instalar una build nocturna a mano.",
        version.c_str(), filter.c_str());
  }
  if (release.assets.empty()) {
    return str::format(
        "La release %s no publica ningun asset: no hay nada que descargar. Busca otra version "
        "o comprueba el repositorio del proyecto.",
        version.c_str());
  }
  return str::format(
      "La release %s no incluye ningun fichero para Switch: los ports de MrShip "
      "dejaron de incluir builds de Switch a partir de cierta version. Puedes probar una "
      "version anterior o instalar una build nocturna a mano.",
      version.c_str());
}

}  // namespace

AssetChoice select_switch_asset(const Release& release, const std::string& asset_filter) {
  AssetChoice choice;
  // El filtro viene del catalogo o de la biblioteca (CatalogEntry/AppEntry); el Release no
  // lo transporta, asi que el llamante siempre lo pasa (vacio = sin filtro).
  const std::string filter = str::trim(asset_filter);

  int best = -1;
  int best_score = -1;
  for (size_t i = 0; i < release.assets.size(); ++i) {
    const ReleaseAsset& asset = release.assets[i];
    const std::string name = display_name(asset);

    // (a) filtro del catalogo/biblioteca.
    if (!str::asset_matches_filter(filter, name)) continue;
    // (b) nunca un asset de escritorio u otra consola.
    if (asset.looks_like_foreign_platform()) continue;
    // (c) marcador positivo de Switch, archivo comprimido o .nro unico.
    const bool usable = asset.looks_like_switch() || asset.is_archive() || asset.is_single_nro();
    if (!usable) continue;
    // (d) sin filtro, exigir el marcador positivo.
    if (filter.empty() && !asset.looks_like_switch()) continue;

    choice.candidates.push_back(static_cast<int>(i));
    const int score = score_candidate(asset);
    if (score > best_score) {
      best_score = score;
      best = static_cast<int>(i);
    }
  }

  if (choice.candidates.empty()) {
    choice.reason = describe_missing_asset(release, filter);
    return choice;
  }

  choice.index = best;
  choice.automatic = choice.candidates.size() == 1;
  if (choice.automatic) {
    choice.reason = str::format("Asset de Switch detectado automaticamente: %s",
                                display_name(release.assets[static_cast<size_t>(best)]).c_str());
  } else {
    choice.reason = str::format(
        "Hay %llu assets compatibles con Switch; se preselecciona %s. Puedes elegir otro.",
        static_cast<unsigned long long>(choice.candidates.size()),
        display_name(release.assets[static_cast<size_t>(best)]).c_str());
  }
  return choice;
}

}  // namespace delanx

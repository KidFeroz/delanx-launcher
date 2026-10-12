// Delanx — pruebas de la capa de datos, ejecutables en el PC (MSYS2/ucrt64 + jansson).
//
// Verifican el contrato que hace que la biblioteca de Delanx sea intercambiable con
// Quiver Launcher: apps.json y settings.json deben sobrevivir a un ciclo
// leer -> escribir -> leer sin perder nada, y el catalogo propio debe parsearse entero.
//
// Compilar y ejecutar: tests/host/run.sh
#include <jansson.h>

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "core/json.hpp"
#include "core/paths.hpp"
#include "core/sha256.hpp"
#include "core/strings.hpp"
#include "core/time.hpp"
#include "model/app_entry.hpp"
#include "model/catalog.hpp"
#include "model/settings.hpp"
#include "services/asset_selector.hpp"
#include "services/launch_args.hpp"

namespace {

int g_failures = 0;
int g_checks = 0;

void check(bool condition, const std::string& what) {
  ++g_checks;
  if (!condition) {
    ++g_failures;
    std::printf("  FALLO: %s\n", what.c_str());
  }
}

void check_eq(const std::string& actual, const std::string& expected, const std::string& what) {
  ++g_checks;
  if (actual != expected) {
    ++g_failures;
    std::printf("  FALLO: %s\n    esperado: \"%s\"\n    obtenido: \"%s\"\n", what.c_str(), expected.c_str(),
                actual.c_str());
  }
}

void section(const char* name) { std::printf("\n== %s ==\n", name); }

std::string tmp_dir() { return "./delanx-host-tests"; }

std::string path_in_tmp(const char* name) { return delanx::paths::join(tmp_dir(), name); }

json_t* parse_file(const std::string& path) {
  json_error_t error;
  json_t* root = json_load_file(path.c_str(), JSON_DECODE_ANY, &error);
  return root;
}

bool has_key(json_t* object, const char* key) { return json_object_get(object, key) != nullptr; }

std::string json_string(json_t* object, const char* key) {
  json_t* value = json_object_get(object, key);
  return json_is_string(value) ? std::string(json_string_value(value)) : std::string();
}

// ------------------------------------------------------------------ 1. apps.json gestionada

const char* kManagedApp = R"json({
  "apps": [
    {
      "name": "Ship of Harkinian",
      "project": "Shipwright",
      "customDisplayName": "Ocarina of Time (SoH)",
      "repository": "HarbourMasters/Shipwright",
      "repositorySource": "gitlab",
      "folderName": "OcarinaOfTime-Shipwright",
      "installPath": "sdmc:/switch/soh",
      "appIconUrl": "https://example.invalid/soh.png",
      "preferredVersion": "8.0.6",
      "skippedUpdateVersion": "9.0.0",
      "autoUpdate": true,
      "deferUpdateTracking": true,
      "tags": ["N64", "Decomp", "n64"],
      "filesToAdd": ["oot.otr"],
      "releaseAssetFilter": "SoH-*-Switch.zip",
      "catalogEntryId": "ship-of-harkinian",
      "catalog": {
        "name": "Ship of Harkinian",
        "project": "Shipwright",
        "appIconUrl": "https://example.invalid/soh.png",
        "tags": ["n64"]
      },
      "mods": {
        "path": "mods",
        "layout": "folderPerMod",
        "sources": [{ "provider": "thunderstore", "sourceUrl": "https://thunderstore.io/c/oot/" }]
      },
      "switch": {
        "targetDir": "/switch/soh",
        "nro": "soh.nro",
        "nroPath": "switch/soh/soh.nro",
        "stripRoot": false,
        "requiredUserFiles": [
          { "path": "sdmc:/switch/soh/oot.otr", "description": "Assets generados en PC" }
        ]
      },
      "requirements": "ROM propia de OoT",
      "harbourMasters": true,
      "verified": true
    }
  ]
})json";

void test_managed_roundtrip() {
  section("1. apps.json gestionada: ida y vuelta sin perdida");
  const std::string input = path_in_tmp("managed-in.json");
  const std::string output = path_in_tmp("managed-out.json");
  check(delanx::paths::write_file(input, kManagedApp), "escritura del apps.json de entrada");

  auto library = delanx::Library::load(input);
  check(library.has_value(), "Library::load devuelve una biblioteca");
  if (!library) return;
  check(library->apps.size() == 1, "una entrada");

  const delanx::AppEntry& app = library->apps[0];
  check_eq(app.name, "Ship of Harkinian", "name");
  check_eq(app.project, "Shipwright", "project");
  check_eq(app.custom_display_name, "Ocarina of Time (SoH)", "customDisplayName");
  check_eq(app.repository, "HarbourMasters/Shipwright", "repository");
  check_eq(app.repository_source, "gitlab", "repositorySource");
  check_eq(app.folder_name, "OcarinaOfTime-Shipwright", "folderName");
  check_eq(app.preferred_version, "8.0.6", "preferredVersion");
  check_eq(app.release_asset_filter, "SoH-*-Switch.zip", "releaseAssetFilter");
  check(app.auto_update, "autoUpdate");
  check(app.defer_update_tracking, "deferUpdateTracking");
  check(app.tags.size() == 2, "tags normalizadas (n64 duplicada se elimina)");
  check(app.files_to_add.size() == 1, "filesToAdd");
  check(app.catalog.has_value(), "catalog (snapshot)");
  check(app.mods_sources.size() == 1, "mods.sources");
  check_eq(app.mods_layout, "folderPerMod", "mods.layout");
  check(app.mrship, "mrship");
  check(app.verified_entry, "verified");
  check_eq(app.sw.target_dir, "sdmc:/switch/soh", "switch.targetDir normalizado");
  check_eq(app.sw.nro, "soh.nro", "switch.nro");
  check(app.sw.required_user_files.size() == 1, "requiredUserFiles");
  check_eq(app.identity_key(), "gitlab:HarbourMasters/Shipwright:OcarinaOfTime-Shipwright", "identityKey");

  check(library->save(output), "Library::save");
  json_t* root = parse_file(output);
  check(root != nullptr, "el fichero escrito es JSON valido");
  if (!root) return;
  json_t* apps = json_object_get(root, "apps");
  check(json_is_array(apps) && json_array_size(apps) == 1, "raiz {apps:[...]}");
  json_t* entry = json_array_get(apps, 0);

  // Claves que Quiver siempre escribe.
  for (const char* key : {"name", "folderName", "installPath", "appIconUrl", "repository", "preferredVersion",
                          "skippedUpdateVersion"}) {
    check(has_key(entry, key), std::string("clave obligatoria en la salida: ") + key);
  }
  check_eq(json_string(entry, "repositorySource"), "gitlab", "repositorySource se conserva");
  check(json_is_true(json_object_get(entry, "autoUpdate")), "autoUpdate=true se escribe");
  check(json_object_get(entry, "preferredVersion") && json_is_string(json_object_get(entry, "preferredVersion")),
        "preferredVersion se escribe como cadena");
  json_t* tags = json_object_get(entry, "tags");
  check(json_is_array(tags) && json_array_size(tags) == 2, "tags en la salida");
  json_t* switch_block = json_object_get(entry, "switch");
  check(json_is_object(switch_block), "bloque switch en la salida");
  if (json_is_object(switch_block)) {
    check_eq(json_string(switch_block, "targetDir"), "sdmc:/switch/soh", "switch.targetDir en disco");
  }

  // Segunda vuelta: el fichero escrito se vuelve a leer igual.
  auto again = delanx::Library::load(output);
  check(again && again->apps.size() == 1, "segunda lectura");
  if (again && !again->apps.empty()) {
    check_eq(again->apps[0].identity_key(), app.identity_key(), "identityKey estable entre vueltas");
    check_eq(again->apps[0].release_asset_filter, app.release_asset_filter, "filtro de asset estable");
    check(again->apps[0].sw.required_user_files.size() == 1, "requiredUserFiles estable");
  }
  json_decref(root);
}

// ------------------------------------------------------------------ 2. app manual

void test_manual_app() {
  section("2. app gestionada a mano: se fuerzan los campos que Quiver no admite");
  const char* json_text = R"json({
    "apps": [
      {
        "name": "Mod suelto",
        "folderName": "ManualDrop",
        "appIconUrl": null,
        "preferredVersion": "1.0",
        "autoUpdate": true,
        "releaseAssetFilter": "algo.zip",
        "tags": ["manual"]
      }
    ]
  })json";
  const std::string input = path_in_tmp("manual-in.json");
  const std::string output = path_in_tmp("manual-out.json");
  delanx::paths::write_file(input, json_text);

  auto library = delanx::Library::load(input);
  check(library && library->apps.size() == 1, "se lee la app manual");
  if (!library || library->apps.empty()) return;
  const delanx::AppEntry& app = library->apps[0];
  check(app.manually_managed(), "manually_managed()");
  check(!app.auto_update, "autoUpdate forzado a false");
  check(app.preferred_version.empty(), "preferredVersion vaciado");
  check(app.release_asset_filter.empty(), "releaseAssetFilter vaciado");
  check_eq(app.identity_key(), "manual:ManualDrop", "identityKey de app manual");

  library->save(output);
  json_t* root = parse_file(output);
  if (!root) {
    check(false, "salida de app manual es JSON valido");
    return;
  }
  json_t* entry = json_array_get(json_object_get(root, "apps"), 0);
  check(!has_key(entry, "repository"), "no se escribe repository en una app manual");
  check(!has_key(entry, "preferredVersion"), "no se escribe preferredVersion en una app manual");
  check(!has_key(entry, "autoUpdate"), "no se escribe autoUpdate=false");
  json_decref(root);
}

// ------------------------------------------------------------------ 3. settings.json

void test_settings_roundtrip() {
  section("3. settings.json: claves de Quiver, enums numericos y claves desconocidas");
  const char* json_text = R"json({
    "FirstStartup": false,
    "UseGridView": true,
    "IconSize": 124,
    "ListRowHeight": 120,
    "PrimaryColor": "#18181b",
    "ThemeTextMode": 2,
    "LibraryNameStyle": 4,
    "ListScope": 1,
    "SortBy": "Name",
    "HiddenApps": ["X"],
    "GitHubApiToken": "",
    "IconOpacity": 0.85,
    "AlgunaClaveFutura": { "a": 1, "b": [true, null] },
    "DelanxMrShipSection": false,
    "DelanxGridColumns": 5
  })json";
  const std::string input = path_in_tmp("settings-in.json");
  const std::string output = path_in_tmp("settings-out.json");
  delanx::paths::write_file(input, json_text);

  delanx::Settings settings = delanx::Settings::load(input);
  check(settings.ListRowHeight == 120, "ListRowHeight leido");
  check(static_cast<int>(settings.ThemeMode) == 2, "ThemeTextMode leido como entero");
  check(static_cast<int>(settings.NameStyle) == 4, "LibraryNameStyle leido como entero");
  check(!settings.DelanxMrShipSection, "clave Delanx* leida");
  check(settings.DelanxGridColumns == 5, "DelanxGridColumns leida");
  check(settings.HiddenApps.size() == 1, "HiddenApps");

  settings.save(output);
  json_t* root = parse_file(output);
  if (!root) {
    check(false, "salida de settings es JSON valido");
    return;
  }
  check(json_is_false(json_object_get(root, "FirstStartup")), "FirstStartup se escribe");
  check(json_integer_value(json_object_get(root, "ThemeTextMode")) == 2, "ThemeTextMode sigue siendo 2");
  check(json_integer_value(json_object_get(root, "LibraryNameStyle")) == 4, "LibraryNameStyle sigue siendo 4");
  json_t* unknown_number = json_object_get(root, "IconOpacity");
  check(json_is_number(unknown_number), "clave desconocida numerica conservada (IconOpacity)");
  json_t* unknown_object = json_object_get(root, "AlgunaClaveFutura");
  check(json_is_object(unknown_object) && json_is_integer(json_object_get(unknown_object, "a")),
        "clave desconocida con objeto conservada intacta");
  json_decref(root);
}

// ------------------------------------------------------------------ 4. catalogo propio

void test_local_catalog() {
  section("4. catalogo propio (romfs/catalog)");
  auto catalog = delanx::Catalog::load_file("romfs/catalog/switch-catalog.json");
  check(catalog.has_value(), "se carga switch-catalog.json");
  if (!catalog) return;
  check(catalog->entries.size() >= 40,
        "entradas esperadas >= 40, obtenidas " + std::to_string(catalog->entries.size()));

  int with_nro = 0;
  int harbour = 0;
  int verified = 0;
  int broken = 0;
  for (const auto& entry : catalog->entries) {
    if (entry.name.empty() || entry.repository.empty() || entry.folder_name.empty()) ++broken;
    if (!entry.sw.nro.empty() || !entry.sw.nro_path.empty()) ++with_nro;
    if (entry.mrship) ++harbour;
    if (entry.verified) ++verified;
    const delanx::AppEntry app = entry.to_app_entry();
    if (app.repository.empty() || app.folder_name.empty()) ++broken;
  }
  check(broken == 0, "ninguna entrada con campos basicos vacios");
  check(with_nro >= 40, "entradas con .nro identificado: " + std::to_string(with_nro));
  check(harbour >= 5, "entradas de Harbour Masters: " + std::to_string(harbour));
  check(verified >= 40, "entradas verificadas: " + std::to_string(verified));

  auto harbour_catalog = delanx::Catalog::load_file("romfs/catalog/harbourmasters.json");
  check(harbour_catalog.has_value(), "se carga harbourmasters.json");
  if (harbour_catalog) {
    check(harbour_catalog->entries.size() == 5,
          "5 entradas de Harbour Masters, obtenidas " + std::to_string(harbour_catalog->entries.size()));
    check(harbour_catalog->unavailable.size() == 3,
          "3 proyectos sin build de Switch, obtenidos " + std::to_string(harbour_catalog->unavailable.size()));
    for (const auto& entry : harbour_catalog->unavailable) {
      check(!entry.reason.empty(), "cada proyecto no disponible explica el motivo");
    }
  }
}

// ------------------------------------------------------------------ 5. assets y seleccion

void test_asset_classification() {
  section("5. clasificacion de assets (el inverso del matcher de Quiver)");
  using delanx::ReleaseAsset;

  auto asset = [](const char* name) {
    ReleaseAsset a;
    a.name = name;
    return a;
  };

  check(asset("SoH-MacReady-Golf-Switch.zip").looks_like_switch(), "zip -Switch.zip es de Switch");
  check(asset("SoH-MacReady-Golf-Switch.zip").is_archive(), "zip reconocido");
  check(!asset("SoH-MacReady-Golf-Switch.zip").looks_like_foreign_platform(), "no es de plataforma ajena");

  check(asset("devilutionx-switch.nro").is_single_nro(), ".nro suelto");
  check(asset("devilutionx-switch.nro").looks_like_switch(), ".nro es de Switch");

  check(asset("mGBA-0.10.5-switch.7z").is_archive(), "7z reconocido");
  check(asset("mGBA-0.10.5-switch.7z").looks_like_switch(), "7z de Switch");

  ReleaseAsset windows = asset("QuiverLauncher-win-Portable.zip");
  check(windows.looks_like_foreign_platform(), "zip de Windows marcado como ajeno");
  check(!windows.looks_like_switch(), "zip de Windows no es de Switch");

  check(asset("Starship-Linux-x64.zip").looks_like_foreign_platform(), "zip de Linux marcado como ajeno");
  check(asset("app-release.apk").looks_like_foreign_platform(), "apk marcado como ajeno");
  check(asset("thing.dmg").looks_like_foreign_platform(), "dmg marcado como ajeno");

  section("5b. filtro de asset y coincidencia de patrones");
  check(delanx::str::glob_match("SoH-*-Switch.zip", "SoH-MacReady-Golf-Switch.zip"), "glob con *");
  check(delanx::str::asset_matches_filter("SoH-*-Switch.zip", "SoH-MacReady-Golf-Switch.zip"), "filtro glob");
  check(delanx::str::asset_matches_filter("EXIT1", "MyGame-EXIT1-linux.zip"), "filtro por subcadena");
  check(!delanx::str::asset_matches_filter("EXIT1", "MyGame-EXIT2-linux.zip"), "subcadena que no coincide");
  check(delanx::str::asset_matches_filter("", "lo-que-sea.zip"), "filtro vacio acepta todo");

  section("5c. seleccion del asset a instalar");
  delanx::Release release;
  release.tag = "8.0.6";
  ReleaseAsset a1 = asset("SoH-MacReady-Golf-Windows.zip");
  ReleaseAsset a2 = asset("SoH-MacReady-Golf-Switch.zip");
  ReleaseAsset a3 = asset("SoH-MacReady-Golf-Mac.zip");
  release.assets = {a1, a2, a3};
  delanx::AssetChoice choice = delanx::select_switch_asset(release, "SoH-*-Switch.zip");
  check(choice.index == 1, "se elige el asset de Switch");
  check(choice.automatic, "un unico candidato => automatico");

  delanx::Release only_windows;
  only_windows.assets = {a1, a3};
  delanx::AssetChoice none = delanx::select_switch_asset(only_windows, "");
  check(none.index == -1, "sin asset de Switch no hay candidato");
  check(!none.reason.empty(), "se explica el motivo al usuario");
}

// ------------------------------------------------------------------ 6. sha256 y utilidades

void test_hash_and_utils() {
  section("6. SHA-256 y utilidades");
  check_eq(delanx::sha256_hex(std::string()), "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
           "sha256 de la cadena vacia");
  check_eq(delanx::sha256_hex(std::string("abc")),
           "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", "sha256 de \"abc\"");

  const std::string file = path_in_tmp("hash.txt");
  delanx::paths::write_file(file, "abc");
  check_eq(delanx::sha256_file(file), "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
           "sha256 de fichero");

  section("6b. rutas y cadenas");
  check_eq(delanx::paths::to_sd_path("/switch/soh"), "sdmc:/switch/soh", "ruta absoluta a sdmc:");
  check_eq(delanx::paths::to_sd_path("sdmc:/switch/soh"), "sdmc:/switch/soh", "sdmc: se respeta");
  check_eq(delanx::paths::basename("sdmc:/switch/soh/soh.nro"), "soh.nro", "basename");
  check_eq(delanx::paths::extension("SoH-Switch.ZIP"), ".zip", "extension en minusculas");
  check_eq(delanx::str::human_size(1536), "1.5 KB", "tamano legible");
  check_eq(delanx::str::join({"a", "b"}, ", "), "a, b", "join");
  check(delanx::str::icontains("HarbourMasters/Shipwright", "shipwright"), "icontains");
}

// ------------------------------------------------------------------ 7. actualizaciones

void test_update_logic() {
  section("7. Deteccion de actualizaciones (tag instalado vs tag remoto)");
  delanx::AppEntry app;
  app.name = "Prueba";
  app.folder_name = "Prueba";
  app.repository = "owner/repo";
  app.installed = true;
  app.installed_version = "v2.0.0";

  check(!app.update_available_for("v2.0.0"), "mismo tag: no hay actualizacion");
  check(app.update_available_for("v2.0.1"), "tag distinto: actualizacion pendiente");

  app.skipped_update_version = "v2.0.1";
  check(!app.update_available_for("v2.0.1"), "version que el jugador decidio saltarse: no se avisa");
  check(app.update_available_for("v2.1.0"), "otra version si se avisa");

  app.installed = false;
  check(!app.update_available_for("v3.0.0"), "app no instalada: sera instalacion, no actualizacion");

  app.installed = true;
  app.repository.clear();
  check(!app.update_available_for("v3.0.0"), "app manual (sin repositorio): nunca se actualiza");

  app.repository = "owner/repo";
  app.installed_version.clear();
  check(app.update_available_for("v9.9.9"), "instalada sin version registrada: se avisa");

  check(!app.update_available_for(""), "sin tag remoto no se inventa una actualizacion");
}

// ------------------------------------------------------------------ 8. argumentos del lanzador

void test_launch_argv() {
  section("8. Argumentos del lanzador (ABI de envSetNextLoad)");

  // --- entrecomillado ------------------------------------------------------
  check_eq(delanx::quote_launch_arg("sdmc:/switch/soh/soh.nro"), "sdmc:/switch/soh/soh.nro",
           "sin espacios no se entrecomilla");
  check_eq(delanx::quote_launch_arg("sdmc:/switch/soh/soh.nro"), "sdmc:/switch/soh/soh.nro",
           "la ruta limpia queda intacta byte a byte");
  check_eq(delanx::quote_launch_arg("/switch/my app/app.nro"), "\"/switch/my app/app.nro\"",
           "con espacios se entrecomilla");
  check_eq(delanx::quote_launch_arg("a\tb"), "\"a\tb\"", "el tabulador tambien exige comillas");
  check_eq(delanx::quote_launch_arg("di \"hola\""), "\"di \\\"hola\\\"\"",
           "las comillas internas se escapan");
  check_eq(delanx::quote_launch_arg("c:\\roms\\gba"), "\"c:\\\\roms\\\\gba\"",
           "las barras invertidas se escapan");
  check_eq(delanx::quote_launch_arg("a\\\"b"), "\"a\\\\\\\"b\"",
           "barra invertida y comilla juntas se escapan en orden");

  // --- cadena completa de argv --------------------------------------------
  const std::string nro = "sdmc:/switch/soh/soh.nro";
  check_eq(delanx::build_launch_argv(nro, {}), nro, "sin argumentos extra la cadena es solo la ruta");

  bool truncated = false;
  const std::string two = delanx::build_launch_argv(nro, {"uno", "dos tres"}, &truncated);
  check_eq(two, nro + " uno \"dos tres\"", "cadena con un argumento simple y uno con espacios");
  check(!truncated, "lo que cabe no se marca como recortado");
  check_eq(two.substr(0, two.find(' ')), nro, "argv[0] es la ruta del NRO (igual que Sphaira)");

  bool truncated_empty = true;
  delanx::build_launch_argv(nro, {}, &truncated_empty);
  check(!truncated_empty, "sin argumentos extra no hay recorte");

  // --- limite exacto -------------------------------------------------------
  // out = ruta (len 22) + ' ' + argumento; cabe mientras len(ruta) + 1 + len(arg) <= limite.
  const size_t just_fits = delanx::kNextLoadArgvMaxBytes - nro.size() - 1;
  check_eq(std::to_string(delanx::build_launch_argv(nro, {std::string(just_fits, 'a')}, &truncated).size()),
           std::to_string(delanx::kNextLoadArgvMaxBytes), "el argumento que cabe justo se conserva");
  check(!truncated, "el argumento que cabe justo no marca recorte");
  const std::string one_more = delanx::build_launch_argv(nro, {std::string(just_fits + 1, 'a')}, &truncated);
  check(truncated, "un byte de mas ya marca recorte");
  check_eq(one_more, nro, "el argumento que no cabe se descarta entero");

  // --- 200 argumentos largos ----------------------------------------------
  std::vector<std::string> many;
  many.reserve(200);
  for (int index = 0; index < 200; ++index) {
    many.push_back("argumento numero " + std::to_string(index) + " con espacios y relleno para que no quepa");
  }
  const std::string capped = delanx::build_launch_argv(nro, many, &truncated);
  check(truncated, "200 argumentos largos no caben: truncated es true");
  check(capped.size() <= delanx::kNextLoadArgvMaxBytes,
        "nunca se supera kNextLoadArgvMaxBytes (" + std::to_string(capped.size()) + " <= " +
            std::to_string(delanx::kNextLoadArgvMaxBytes) + ")");
  check_eq(capped.substr(0, capped.find(' ')), nro, "tras recortar, la ruta sigue siendo el primer argumento");
  check(capped.find("argumento numero 0 ") != std::string::npos, "se conservan los primeros argumentos");
  check(capped.find("argumento numero 199 ") == std::string::npos, "se descartan los ultimos enteros");
  check(!capped.empty() && capped.back() != ' ', "no queda un espacio suelto al final");

  // Y una ruta que por si sola no cabe: se recorta al limite, nunca lo supera.
  bool truncated_path = false;
  const std::string huge = delanx::build_launch_argv("/switch/" + std::string(3000, 'x'), {"uno"},
                                                     &truncated_path);
  check(truncated_path, "una ruta gigantesca marca recorte");
  check(huge.size() <= delanx::kNextLoadArgvMaxBytes, "la ruta gigantesca tambien se recorta al limite");
  check(delanx::kNextLoadPathMaxBytes < 512 && delanx::kNextLoadArgvMaxBytes < 2048,
        "los margenes respetan el ABI (512 B de ruta, 2048 B de argv)");
}

// ------------------------------------------------------------------ 9. catalogo maestro

void test_master_catalog_if_present() {
  section("9. Catalogo maestro (si existe)");
  const std::string path = "romfs/catalog/ports-master.json";
  if (!delanx::paths::exists(path)) {
    std::printf("  aviso: %s todavia no existe (lo esta creando otro agente); se omite esta seccion\n",
                path.c_str());
    return;
  }

  auto master = delanx::Catalog::load_file(path);
  check(master.has_value(), "se carga " + path);
  if (!master) return;
  check(!master->entries.empty(),
        "el catalogo maestro tiene entradas: " + std::to_string(master->entries.size()));

  int incomplete = 0;
  int marked_verified = 0;
  for (const auto& entry : master->entries) {
    const std::string where = " (entrada " + (entry.name.empty() ? std::string("<sin nombre>") : entry.name) + ")";
    if (entry.repository.empty()) {
      ++incomplete;
      check(false, "repository no vacio" + where);
    }
    if (entry.folder_name.empty()) {
      ++incomplete;
      check(false, "folderName no vacio" + where);
    }
    if (entry.sw.target_dir.empty()) {
      ++incomplete;
      check(false, "switch.targetDir no vacio" + where);
    }
    if (entry.sw.nro.empty()) {
      ++incomplete;
      check(false, "switch.nro no vacio" + where);
    }
    if (entry.verified) {
      ++marked_verified;
      check(false, "ninguna entrada del catalogo maestro se marca verified" + where);
    }
  }
  check(incomplete == 0, "todas las entradas tienen repository/folderName/targetDir/nro (" +
                             std::to_string(master->entries.size()) + " entradas)");
  check(marked_verified == 0, "ninguna entrada del catalogo maestro viene pre-verificada");
}

}  // namespace

int main() {
  std::printf("Delanx — pruebas de la capa de datos\n");
  delanx::paths::make_dirs(tmp_dir());

  test_managed_roundtrip();
  test_manual_app();
  test_settings_roundtrip();
  test_local_catalog();
  test_asset_classification();
  test_hash_and_utils();
  test_update_logic();
  test_launch_argv();
  test_master_catalog_if_present();

  std::printf("\n----------------------------------------\n");
  std::printf("Comprobaciones: %d   Fallos: %d\n", g_checks, g_failures);
  return g_failures == 0 ? 0 : 1;
}

// Delanx — pruebas del INSTALADOR con releases REALES de Switch.
//
// La capa HTTP se sustituye por un stub que copia un asset ya descargado en
// tests/fixtures/assets/ (capturado de las URLs reales de los releases), de modo que aqui se
// ejercita el camino completo: elegir asset -> descargar -> verificar SHA-256 -> extraer con
// libarchive -> escribir metadatos -> localizar el .nro -> desinstalar.
//
// Los tres assets cubren los tres diseños de paquete que usa el homebrew de Switch:
//   Starship-Barnard-Alfa-Switch.zip          -> .nro en la raiz del zip        (stripRoot=false)
//   sphaira.zip                               -> switch/<app>/<app>.nro          (targetDir "/")
//   Spaghettify-Alfredo-Alfa-1-Switch.zip     -> carpeta raiz unica             (stripRoot=true)
//
// Compilar y ejecutar: tests/host/run.sh
#include <jansson.h>

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "core/json.hpp"
#include "core/paths.hpp"
#include "core/sha256.hpp"
#include "core/strings.hpp"
#include "model/app_entry.hpp"
#include "model/catalog.hpp"
#include "services/artwork.hpp"
#include "services/asset_selector.hpp"
#include "services/github.hpp"
#include "services/http.hpp"
#include "services/installer.hpp"

// ---------------------------------------------------------------------------
// Stub de red: "descarga" copiando el asset real ya presente en fixtures/assets.
// ---------------------------------------------------------------------------
namespace delanx::net {

void global_init() {}
void global_shutdown() {}

std::string build_url(const std::string& base,
                      const std::vector<std::pair<std::string, std::string>>& query) {
  std::string out = base;
  bool first = true;
  for (const auto& [key, value] : query) {
    if (value.empty()) continue;
    out += first ? "?" : "&";
    first = false;
    out += key + "=" + str::url_encode(value);
  }
  return out;
}

Response get(const std::string& url, const std::vector<std::string>& headers, int timeout_seconds) {
  (void)headers;
  (void)timeout_seconds;
  Response response;
  if (url.find("/repos/HarbourMasters/Starship/releases") != std::string::npos) {
    const auto body = paths::read_file("tests/fixtures/github_starship_releases.json");
    if (body) {
      response.status = 200;
      response.body = *body;
      return response;
    }
  }
  response.status = 404;
  response.body = "{\"message\":\"Not Found\"}";
  return response;
}

Response post_json(const std::string& url, const std::string& body,
                   const std::vector<std::string>& headers, int timeout_seconds) {
  (void)url;
  (void)body;
  (void)headers;
  (void)timeout_seconds;
  Response response;
  response.status = 501;
  return response;
}

// Copia el asset local al destino llamando al callback de progreso, como haria libcurl.
bool download(const std::string& url, const std::string& dest_path, const ProgressFn& progress,
              std::string* error, const std::vector<std::string>& headers) {
  (void)headers;
  const std::string name = paths::basename(url);
  const std::string source = "tests/fixtures/assets/" + name;
  if (!paths::exists(source)) {
    if (error != nullptr) *error = "asset de prueba no encontrado: " + source;
    return false;
  }
  paths::make_dirs(paths::dirname(dest_path));
  const uint64_t total = paths::file_size(source);
  if (progress && !progress(0, static_cast<int64_t>(total))) return false;

  FILE* in = fopen(source.c_str(), "rb");
  if (in == nullptr) {
    if (error != nullptr) *error = "no se pudo abrir " + source;
    return false;
  }
  FILE* out = fopen(dest_path.c_str(), "wb");
  if (out == nullptr) {
    fclose(in);
    if (error != nullptr) *error = "no se pudo escribir " + dest_path;
    return false;
  }
  char buffer[64 * 1024];
  size_t read = 0;
  uint64_t written = 0;
  bool cancelled = false;
  while ((read = fread(buffer, 1, sizeof(buffer), in)) > 0) {
    if (fwrite(buffer, 1, read, out) != read) break;
    written += read;
    if (progress && !progress(static_cast<int64_t>(written), static_cast<int64_t>(total))) {
      cancelled = true;
      break;
    }
  }
  fclose(in);
  fclose(out);
  if (cancelled) {
    paths::remove_file(dest_path);
    if (error != nullptr) *error = "descarga cancelada";
    return false;
  }
  return written == total;
}

}  // namespace delanx::net

// ---------------------------------------------------------------------------
// Utilidades del test
// ---------------------------------------------------------------------------
namespace {

using delanx::AppEntry;
using delanx::Catalog;
using delanx::Release;
using delanx::ReleaseAsset;
using delanx::SwitchInstall;

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

// --- repositorio de prueba -------------------------------------------------

constexpr const char* kStarshipSha = "b64238ad35e5401229576baa98dbd8a517228eac996454f9d55d293888ec9fa9";
constexpr const char* kSphairaSha = "3e7ebacd421917d426cc1ab49622ec4a1ea7943fb571d62aea7176089dc1384c";
constexpr const char* kSpaghettiSha = "52dbf4b1d7213d29133d98f92ac86ebcf87854ad3ebd072881b671507592b726";
// mGBA es el unico asset del catalogo empaquetado en .7z: lo extrae libarchive.
constexpr const char* kMgbaSha = "ef7a7f5cea6d6639c8a12ebb282d915e444bf844b658ff5a4dd72886c8cfa016";

std::string work_dir() { return "build/host/install-tests"; }
std::string sd_root() { return work_dir() + "/sd"; }

// Recarga el catalogo real y devuelve la entrada pedida convertida en AppEntry.
bool catalog_app(const std::string& name, AppEntry* out) {
  auto catalog = Catalog::load_file("romfs/catalog/switch-catalog.json");
  if (!catalog) return false;
  for (const auto& entry : catalog->entries) {
    if (entry.name == name) {
      *out = entry.to_app_entry();
      return true;
    }
  }
  return false;
}

ReleaseAsset pick_asset(const AppEntry& app, Release* release) {
  std::string error;
  const std::vector<Release> releases = delanx::gh::fetch_releases(app.repository, "", &error);
  for (size_t index = 0; index < releases.size(); ++index) {
    if (releases[index].tag == "v2.0.0") {
      *release = releases[index];
      break;
    }
  }
  const delanx::AssetChoice choice = delanx::select_switch_asset(*release, app.release_asset_filter);
  if (choice.index < 0) return ReleaseAsset{};
  return release->assets[static_cast<size_t>(choice.index)];
}

delanx::InstallReport install(const AppEntry& app, const Release& release, const ReleaseAsset& asset,
                             const std::string& expected_sha) {
  delanx::InstallRequest request;
  request.app = app;
  request.release = release;
  request.asset = asset;
  request.expected_sha256 = expected_sha;
  request.allow_unverified = expected_sha.empty();
  return delanx::install_release(request, [](int64_t, int64_t) { return true; });
}

// ---------------------------------------------------------------------------
// 1. Starship: .nro en la raiz del zip, instalado en /switch/Starship
// ---------------------------------------------------------------------------
void test_starship_install() {
  section("1. Instalacion real: Starship (Strip-zip con .nro en la raiz)");
  AppEntry app;
  if (!catalog_app("Starship (Star Fox 64)", &app)) {
    check(false, "el catalogo contiene Starship (Star Fox 64)");
    return;
  }
  std::printf("  info  targetDir=%s nro=%s stripRoot=%d filtro=%s\n", app.sw.target_dir.c_str(),
              app.sw.nro.c_str(), app.sw.strip_root ? 1 : 0, app.release_asset_filter.c_str());

  Release release;
  const ReleaseAsset asset = pick_asset(app, &release);
  check(!asset.name.empty(), "se selecciona un asset del release v2.0.0");
  check_eq(asset.name, "Starship-Barnard-Alfa-Switch.zip", "el asset elegido es el de Switch");
  if (asset.name.empty()) return;

  const std::string target = app.sw.effective_target(delanx::paths::apps_dir(), app.folder_name);
  check(delanx::str::ends_with(target, "/switch/Starship"),
        "la app se instala en su ruta nativa (" + target + ")");

  const delanx::InstallReport report = install(app, release, asset, kStarshipSha);
  check(report.status == delanx::InstallStatus::Ok, "instalacion correcta (" + report.message + ")");
  check_eq(report.sha256, kStarshipSha, "el SHA-256 calculado coincide con el real del asset");

  const std::string nro = delanx::paths::join(target, "Starship.nro");
  check(delanx::paths::exists(nro), "queda Starship.nro en " + nro);
  const std::string o2r = delanx::paths::join(target, "starship.o2r");
  check(delanx::paths::exists(o2r), "se extrae tambien starship.o2r");
  check(delanx::paths::is_dir(delanx::paths::join(target, "assets")), "se extraen las subcarpetas");

  // Metadatos compatibles con Quiver.
  const auto version = delanx::paths::read_file(delanx::paths::join(target, "version.txt"));
  check(version && delanx::str::trim(*version) == "v2.0.0", "version.txt contiene el tag instalado");
  const auto build = delanx::paths::read_file(delanx::paths::join(target, "installed-build.txt"));
  check(build.has_value(), "se escribe installed-build.txt");
  if (build) {
    const std::vector<std::string> lines = delanx::str::split(*build, '\n');
    check(!lines.empty() && lines[0] == asset.name, "installed-build.txt guarda el fichero instalado");
    check(lines.size() > 1 && delanx::str::trim(lines[1]) == kStarshipSha,
          "installed-build.txt guarda el SHA-256");
  }

  const std::string launched = delanx::find_launchable_nro(target, app.sw.nro);
  check(!launched.empty(), "find_launchable_nro encuentra el ejecutable");
  check(delanx::str::iends_with(launched, "Starship.nro"), "el ejecutable encontrado es el correcto");
}

// ---------------------------------------------------------------------------
// 2. Sphaira: zip con switch/<app>/<app>.nro en la raiz de la SD
// ---------------------------------------------------------------------------
void test_sphaira_install() {
  section("2. Instalacion real: Sphaira (switch/<app>/<app>.nro)");
  AppEntry app;
  if (!catalog_app("Sphaira", &app)) {
    check(false, "el catalogo contiene Sphaira");
    return;
  }
  std::printf("  info  targetDir=%s nroPath=%s\n", app.sw.target_dir.c_str(), app.sw.nro_path.c_str());
  const bool sd_root_target = app.sw.target_dir == sd_root() || app.sw.target_dir == sd_root() + "/" ||
                              app.sw.target_dir == "sdmc:/";
  check(sd_root_target, "Sphaira se instala en la raiz de la SD (" + app.sw.target_dir + ")");
  check_eq(app.sw.nro_path, "switch/sphaira/sphaira.nro", "nroPath apunta dentro de switch/");

  Release release;
  release.tag = "1.0.8";
  ReleaseAsset asset;
  asset.name = "sphaira.zip";
  asset.url = "https://github.com/NaGaa95/sphaira/releases/download/1.0.8/sphaira.zip";

  const delanx::InstallReport report = install(app, release, asset, kSphairaSha);
  check(report.status == delanx::InstallStatus::Ok, "instalacion correcta (" + report.message + ")");

  const std::string nro = delanx::paths::join(sd_root(), "switch/sphaira/sphaira.nro");
  check(delanx::paths::exists(nro), "el zip se extrae respetando switch/sphaira/ (" + nro + ")");
}

// ---------------------------------------------------------------------------
// 3. SpaghettiKart: carpeta raiz unica -> stripRoot
// ---------------------------------------------------------------------------
void test_spaghetti_strip_root() {
  section("3. Instalacion real: SpaghettiKart (carpeta raiz unica, stripRoot)");
  AppEntry app;
  if (!catalog_app("SpaghettiKart (Mario Kart 64)", &app)) {
    check(false, "el catalogo contiene SpaghettiKart");
    return;
  }
  std::printf("  info  targetDir=%s stripRoot=%d\n", app.sw.target_dir.c_str(),
              app.sw.strip_root ? 1 : 0);
  check(app.sw.strip_root, "el catalogo marca stripRoot para este paquete");

  Release release;
  release.tag = "0.9.9.1";
  ReleaseAsset asset;
  asset.name = "Spaghettify-Alfredo-Alfa-1-Switch.zip";
  asset.url =
      "https://github.com/HarbourMasters/SpaghettiKart/releases/download/0.9.9.1/"
      "Spaghettify-Alfredo-Alfa-1-Switch.zip";

  const delanx::InstallReport report = install(app, release, asset, kSpaghettiSha);
  check(report.status == delanx::InstallStatus::Ok, "instalacion correcta (" + report.message + ")");

  const std::string target = app.sw.effective_target(delanx::paths::apps_dir(), app.folder_name);
  // El zip trae "Spaghettify-Switch/Spaghettify.nro": la carpeta raiz debe desaparecer.
  check(delanx::paths::exists(delanx::paths::join(target, "Spaghettify.nro")),
        "el .nro queda en la raiz del destino (sin Spaghettify-Switch/)");
  check(!delanx::paths::is_dir(delanx::paths::join(target, "Spaghettify-Switch")),
        "la carpeta raiz del zip se elimina");
}

// ---------------------------------------------------------------------------
// 4. Verificacion: checksum incorrecto y checksum ausente
// ---------------------------------------------------------------------------
void test_hash_enforcement() {
  section("4. Verificacion SHA-256");
  AppEntry app;
  if (!catalog_app("Starship (Star Fox 64)", &app)) return;
  Release release;
  const ReleaseAsset asset = pick_asset(app, &release);
  if (asset.name.empty()) return;

  const std::string target = app.sw.effective_target(delanx::paths::apps_dir(), app.folder_name);
  const std::string marker = delanx::paths::join(target, "Starship.nro");
  const std::string wrong = std::string(64, '0');

  // Se borra lo instalado antes para comprobar que un fallo no instala nada.
  delanx::paths::remove_dir_recursive(target);
  delanx::InstallReport report = install(app, release, asset, wrong);
  check(report.status == delanx::InstallStatus::HashMismatch,
        "un checksum que no coincide aborta la instalacion (" + report.message + ")");
  check(!delanx::paths::exists(marker), "tras un fallo de verificacion no queda el .nro");

  // Sin checksum publicado se instala, pero avisando.
  report = install(app, release, asset, "");
  check(report.status == delanx::InstallStatus::Ok, "sin checksum se instala si el usuario acepta");
  check(delanx::str::icontains(report.message, "sin checksum") ||
            delanx::str::icontains(report.message, "no verificado"),
        "se avisa de que no habia checksum (" + report.message + ")");
}

// ---------------------------------------------------------------------------
// 5. Ficheros del usuario: sobreviven a reinstalar y a desinstalar
// ---------------------------------------------------------------------------
void test_user_files_survive() {
  section("5. Los ficheros que aporta el usuario no se tocan");
  AppEntry app;
  if (!catalog_app("Starship (Star Fox 64)", &app)) return;
  Release release;
  const ReleaseAsset asset = pick_asset(app, &release);
  if (asset.name.empty()) return;

  const std::string target = app.sw.effective_target(delanx::paths::apps_dir(), app.folder_name);
  delanx::paths::make_dirs(target);
  const std::string user_file = delanx::paths::join(target, "starship.o2r");
  const std::string user_rom = delanx::paths::join(target, "baserom.z64");
  delanx::paths::write_file(user_file, "DATOS GENERADOS POR EL USUARIO");
  delanx::paths::write_file(user_rom, "ROM DEL USUARIO");

  const delanx::InstallReport report = install(app, release, asset, kStarshipSha);
  check(report.status == delanx::InstallStatus::Ok, "reinstalacion sobre una carpeta existente");
  const auto content = delanx::paths::read_file(user_file);
  check(content && *content == "DATOS GENERADOS POR EL USUARIO", "el .o2r del usuario sigue intacto");
  const auto rom = delanx::paths::read_file(user_rom);
  check(rom && *rom == "ROM DEL USUARIO", "la ROM del usuario sigue intacta");

  // Desinstalar: se van los ficheros de la app, no los del usuario.
  std::string uninstall_message;
  const bool removed = delanx::uninstall_app(app, false, &uninstall_message);
  check(removed, "uninstall_app devuelve true (" + uninstall_message + ")");
  check(delanx::paths::exists(user_file), "el .o2r del usuario sobrevive a la desinstalacion");
  check(delanx::paths::exists(user_rom), "la ROM del usuario sobrevive a la desinstalacion");
  check(!delanx::paths::exists(delanx::paths::join(target, "Starship.nro")),
        "el ejecutable de la app si se borra");
}

// ---------------------------------------------------------------------------
// 6. Extraccion: zip-slip y stripRoot explorito
// ---------------------------------------------------------------------------
void test_extraction_safety() {
  section("6. Extraccion: path traversal y stripRoot");

  const std::string dest = work_dir() + "/extract-dest";
  delanx::paths::remove_dir_recursive(dest);
  delanx::paths::make_dirs(dest);

  // Zip malicioso preparado por tests/host/make_test_archives.py
  const std::string evil = "tests/fixtures/assets/malicious.zip";
  if (delanx::paths::exists(evil)) {
    std::string error;
    const bool ok = delanx::extract_archive(evil, dest, false, &error);
    check(!ok, "un zip con '../' se rechaza");
    check(!delanx::paths::exists(work_dir() + "/escaped.txt"),
          "el fichero malicioso no se escribe fuera del destino");
    check(!delanx::paths::exists("escaped.txt"), "tampoco en el directorio de trabajo");
  } else {
    check(false, "falta tests/fixtures/assets/malicious.zip (ejecuta make_test_archives.py)");
  }

  // Zip con carpeta raiz unica: sin strip_root se conserva, con strip_root se quita.
  const std::string rooted = "tests/fixtures/assets/single-root.zip";
  if (delanx::paths::exists(rooted)) {
    const std::string keep = work_dir() + "/extract-keep";
    const std::string strip = work_dir() + "/extract-strip";
    delanx::paths::remove_dir_recursive(keep);
    delanx::paths::remove_dir_recursive(strip);
    std::string error;
    check(delanx::extract_archive(rooted, keep, false, &error), "extraccion sin strip_root");
    check(delanx::paths::exists(delanx::paths::join(keep, "App/app.nro")),
          "sin strip_root se conserva la carpeta raiz");
    check(delanx::extract_archive(rooted, strip, true, &error), "extraccion con strip_root");
    check(delanx::paths::exists(delanx::paths::join(strip, "app.nro")),
          "con strip_root el .nro queda en la raiz del destino");
  } else {
    check(false, "falta tests/fixtures/assets/single-root.zip (ejecuta make_test_archives.py)");
  }
}

// ---------------------------------------------------------------------------
// 7. Destino compartido: la raiz de la SD no se puede vaciar ni borrar
// ---------------------------------------------------------------------------
void test_shared_target_is_safe() {
  section("7. Destino compartido (raiz de la SD): no se toca lo ajeno");
  AppEntry app;
  if (!catalog_app("Sphaira", &app)) {
    check(false, "el catalogo contiene Sphaira");
    return;
  }
  const bool root_target = app.sw.target_dir == sd_root() || app.sw.target_dir == sd_root() + "/" ||
                           app.sw.target_dir == "sdmc:/";
  check(root_target, "Sphaira se instala en la raiz de la SD, destino compartido (" +
                         app.sw.target_dir + ")");

  // Ficheros ajenos que un homebrew jamas debe tocar: uno en la raiz y otro en una carpeta.
  const std::string foreign_root = delanx::paths::join(sd_root(), "hbmenu.nro");
  const std::string foreign_config = delanx::paths::join(sd_root(), "config_ajena.txt");
  const std::string foreign_dir = delanx::paths::join(sd_root(), "Nintendo");
  const std::string foreign_save = delanx::paths::join(foreign_dir, "partida.sav");
  delanx::paths::write_file(foreign_root, "HOMEBREW MENU AJENO");
  delanx::paths::write_file(foreign_config, "CONFIG DE OTRA APP");
  delanx::paths::make_dirs(foreign_dir);
  delanx::paths::write_file(foreign_save, "PARTIDA DEL USUARIO");

  Release release;
  release.tag = "1.0.8";
  ReleaseAsset asset;
  asset.name = "sphaira.zip";
  asset.url = "https://github.com/NaGaa95/sphaira/releases/download/1.0.8/sphaira.zip";

  const delanx::InstallReport report = install(app, release, asset, kSphairaSha);
  check(report.status == delanx::InstallStatus::Ok, "instalacion en la raiz correcta");
  check(delanx::paths::exists(foreign_root), "hbmenu.nro de otra app sobrevive a la instalacion");
  check(delanx::paths::exists(foreign_config), "un fichero de configuracion ajeno sobrevive");
  check(delanx::paths::exists(foreign_save), "una partida dentro de Nintendo/ sobrevive");

  // Desinstalar: se va lo de Sphaira, no la raiz de la SD.
  std::string message;
  const bool removed = delanx::uninstall_app(app, false, &message);
  check(removed, "uninstall_app devuelve true (" + message + ")");
  check(!delanx::paths::exists(delanx::paths::join(sd_root(), "switch/sphaira/sphaira.nro")),
        "el ejecutable de Sphaira si se elimina");
  check(!delanx::paths::exists(delanx::paths::join(sd_root(), "version.txt")),
        "los metadatos de Delanx tambien se limpian");
  check(delanx::paths::exists(sd_root()), "la raiz de la SD sigue existiendo");
  check(delanx::paths::exists(foreign_root), "hbmenu.nro sigue ahi tras desinstalar");
  check(delanx::paths::exists(foreign_config), "la config ajena sigue ahi tras desinstalar");
  check(delanx::paths::exists(foreign_save), "la partida sigue ahi tras desinstalar");
}

// ---------------------------------------------------------------------------
// 8. Proteccion de los ficheros que declara el catalogo (no solo por extension)
// ---------------------------------------------------------------------------
void test_declared_user_files_protected() {
  section("8. requiredUserFiles del catalogo: se respetan aunque la extension no este en la lista");
  AppEntry app;
  if (!catalog_app("Starship (Star Fox 64)", &app)) return;

  Release release;
  const ReleaseAsset asset = pick_asset(app, &release);
  if (asset.name.empty()) return;

  const std::string target = app.sw.effective_target(delanx::paths::apps_dir(), app.folder_name);
  delanx::paths::make_dirs(target);

  // "gamecontrollerdb.txt" viene DENTRO del zip y su extension (.txt) no esta en la lista de
  // datos de juego: solo puede protegerse porque el catalogo lo declara.
  const std::string declared = delanx::paths::join(target, "gamecontrollerdb.txt");
  delanx::paths::write_file(declared, "MAPPING DEL USUARIO");
  app.sw.required_user_files.push_back(
      SwitchInstall::RequiredFile{declared, "mapping propio del jugador"});

  const delanx::InstallReport report = install(app, release, asset, kStarshipSha);
  check(report.status == delanx::InstallStatus::Ok, "reinstalacion correcta");
  const auto content = delanx::paths::read_file(declared);
  check(content && *content == "MAPPING DEL USUARIO",
        "el fichero declarado en requiredUserFiles no se sobrescribe con el del zip");

  // Y el resto del paquete si se instala.
  check(delanx::paths::exists(delanx::paths::join(target, "Starship.nro")), "el ejecutable se instala");

  // Desinstalar tampoco puede llevarse ese fichero declarado.
  std::string message;
  delanx::uninstall_app(app, false, &message);
  check(delanx::paths::exists(declared), "el fichero declarado sobrevive a la desinstalacion");
}

// ---------------------------------------------------------------------------
// 9. Caratulas: cola de descargas y cache
// ---------------------------------------------------------------------------
void test_artwork_cache() {
  section("9. Caratulas: cola, cache y deduplicacion");
  delanx::clear_artwork_queue();

  AppEntry app;
  app.folder_name = "ArtworkTest";
  app.app_icon_url = "https://example.invalid/artwork-test.png";
  const std::string dest = app.icon_cache_path();
  delanx::paths::remove_file(dest);

  check(delanx::artwork_queue_size() == 0, "la cola empieza vacia");
  delanx::queue_artwork(app);
  check(delanx::artwork_queue_size() == 1, "se encola la caratula");
  delanx::queue_artwork(app);
  check(delanx::artwork_queue_size() == 1, "la misma app no se encola dos veces");

  check(delanx::process_artwork_queue(1) == 1, "se descarga una caratula con presupuesto 1");
  check(delanx::paths::exists(dest), "la caratula queda en Cache/artwork");
  check(delanx::artwork_queue_size() == 0, "la cola queda vacia");

  const uint64_t cached_size = delanx::paths::file_size(dest);
  delanx::queue_artwork(app);
  check(delanx::artwork_queue_size() == 0, "si ya esta en cache no se vuelve a encolar");
  check(delanx::paths::file_size(dest) == cached_size, "el fichero cacheado no se toca");

  AppEntry sin_caratula;
  sin_caratula.folder_name = "SinCaratula";
  delanx::queue_artwork(sin_caratula);
  check(delanx::artwork_queue_size() == 0, "una app sin URL de caratula no se encola");

  AppEntry sin_carpeta;
  sin_carpeta.app_icon_url = "https://example.invalid/artwork-test.png";
  delanx::queue_artwork(sin_carpeta);
  check(delanx::artwork_queue_size() == 0, "una app sin folderName no se encola");
}

// ---------------------------------------------------------------------------
// 10. Asset .7z real: mGBA. Es el unico formato del catalogo que no es zip y solo
//     libarchive puede extraerlo, asi que se prueba con el asset de verdad.
// ---------------------------------------------------------------------------
void test_seven_zip_install() {
  section("10. Asset .7z real (mGBA)");
  const std::string fixture = "tests/fixtures/assets/mGBA-0.10.5-switch.7z";
  if (!delanx::paths::exists(fixture)) {
    check(false, "falta " + fixture + " (ejecuta tests/host/fetch_fixtures.py)");
    return;
  }

  // Entrada sintetica: el catalogo aun no tiene los datos de layout verificados.
  AppEntry app;
  app.name = "mGBA (Game Boy Advance)";
  app.folder_name = "GBA-mGBA";
  app.sw.target_dir = "/switch/mgba";
  app.sw.strip_root = false;

  Release release;
  release.tag = "0.10.5";
  ReleaseAsset asset;
  asset.name = "mGBA-0.10.5-switch.7z";
  asset.url = "https://github.com/mgba-emu/mgba/releases/download/0.10.5/mGBA-0.10.5-switch.7z";

  const delanx::InstallReport report = install(app, release, asset, kMgbaSha);
  check(report.status == delanx::InstallStatus::Ok, "instalacion del .7z correcta (" + report.message + ")");
  check_eq(report.sha256, kMgbaSha, "el SHA-256 del .7z coincide con el real del asset");

  const std::string target = app.sw.effective_target(delanx::paths::apps_dir(), app.folder_name);
  std::printf("  info  destino=%s\n", target.c_str());
  std::printf("  info  listado de %s:\n", target.c_str());
  for (const std::string& name : delanx::paths::list_dir(target)) {
    const std::string full = delanx::paths::join(target, name);
    if (delanx::paths::is_dir(full)) {
      std::printf("  info    %s/\n", name.c_str());
      for (const std::string& inner : delanx::paths::list_dir(full)) {
        const std::string inner_full = delanx::paths::join(full, inner);
        std::printf("  info      %s/%s%s (%llu B)\n", name.c_str(), inner.c_str(),
                    delanx::paths::is_dir(inner_full) ? "/" : "",
                    static_cast<unsigned long long>(delanx::paths::file_size(inner_full)));
      }
    } else {
      std::printf("  info    %s (%llu B)\n", name.c_str(),
                  static_cast<unsigned long long>(delanx::paths::file_size(full)));
    }
  }

  const std::string nro = delanx::find_launchable_nro(target, app.sw.nro);
  std::printf("  info  find_launchable_nro -> %s\n", nro.c_str());
  check(!nro.empty(), "se encuentra un .nro dentro del .7z extraido");
  check(delanx::paths::exists(nro), "el .nro devuelto existe en disco");
  check(delanx::str::iends_with(nro, ".nro"), "la ruta devuelta termina en .nro");
  // Nombre verificado del ejecutable (no vale "hay algun .nro").
  check_eq(delanx::paths::basename(nro), "mgba.nro", "el .nro del .7z se llama exactamente mgba.nro");
  check(delanx::str::ends_with(nro, "/mgba.nro"), "la ruta del .nro termina en /mgba.nro");

  // El .7z envuelve todo en una unica carpeta "mGBA-0.10.5-switch/": eso es justo lo que
  // declara el catalogo como stripRoot, asi que se comprueba tambien por separado.
  const std::string stripped = work_dir() + "/mgba-strip";
  delanx::paths::remove_dir_recursive(stripped);
  std::string extract_error;
  check(delanx::extract_archive(fixture, stripped, true, &extract_error),
        "el mismo .7z se extrae con stripRoot (" + extract_error + ")");
  check(delanx::paths::exists(delanx::paths::join(stripped, "mgba.nro")),
        "con stripRoot el .nro queda directamente en la raiz del destino");
  check(!delanx::paths::is_dir(delanx::paths::join(stripped, "mGBA-0.10.5-switch")),
        "se elimina la carpeta raiz mGBA-0.10.5-switch/ del .7z");

  // Y la entrada del catalogo ya verificada describe exactamente ese layout.
  AppEntry catalog_entry;
  if (catalog_app("mGBA (Game Boy Advance)", &catalog_entry)) {
    check(catalog_entry.verified_entry, "la entrada de mGBA en el catalogo esta verificada");
    check_eq(catalog_entry.sw.nro, "mgba.nro", "el catalogo declara nro=mgba.nro");
    check_eq(catalog_entry.sw.nro_path, "switch/mgba/mgba.nro",
             "el catalogo declara nroPath=switch/mgba/mgba.nro");
    check(catalog_entry.sw.strip_root, "el catalogo declara stripRoot=true para el .7z");
  } else {
    check(false, "el catalogo contiene mGBA (Game Boy Advance)");
  }
}

}  // namespace

int main() {
  std::printf("Delanx — pruebas del instalador con releases reales de Switch\n");

  // Raiz de datos y prefijo de SD locales para no tocar nada del sistema.
  delanx::paths::set_root_override(work_dir() + "/Delanx");
  delanx::paths::set_sd_prefix_override(sd_root());
  delanx::paths::make_dirs(work_dir());
  delanx::paths::make_dirs(sd_root());
  delanx::paths::make_dirs(delanx::paths::temp_dir());

  test_starship_install();
  test_sphaira_install();
  test_spaghetti_strip_root();
  test_hash_enforcement();
  test_user_files_survive();
  test_extraction_safety();
  test_shared_target_is_safe();
  test_declared_user_files_protected();
  test_artwork_cache();
  test_seven_zip_install();

  std::printf("\n----------------------------------------\n");
  std::printf("Comprobaciones: %d   Fallos: %d\n", g_checks, g_failures);
  return g_failures == 0 ? 0 : 1;
}

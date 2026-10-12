// Delanx — pruebas de la capa de red sobre RESPUESTAS REALES de las APIs.
//
// La capa HTTP (libcurl + libnx) no se puede ejecutar en el PC, asi que este test la
// sustituye por un stub que sirve fixtures capturadas literalmente de:
//   * api.github.com  -> releases de HarbourMasters/Starship (incluye asset de Switch)
//   * gitlab.com      -> releases de gitlab-org/cli
//   * api.quiverlauncher.com -> /apps?console=switch, /facets, /apps/{slug}/release-history,
//                               /release-status
// y ejercita los parsers REALES (github.cpp, gitlab.cpp, quiver_catalog.cpp,
// release_check.cpp, asset_selector.cpp) sobre esos datos.
//
// Compilar y ejecutar: tests/host/run.sh
#include <jansson.h>

#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

#include "core/json.hpp"
#include "core/paths.hpp"
#include "core/strings.hpp"
#include "model/app_entry.hpp"
#include "model/catalog.hpp"
#include "services/asset_selector.hpp"
#include "services/github.hpp"
#include "services/gitlab.hpp"
#include "services/http.hpp"
#include "services/quiver_catalog.hpp"
#include "services/release_check.hpp"

// ---------------------------------------------------------------------------
// Stub de la capa HTTP: sirve fixtures locales segun la URL pedida.
// ---------------------------------------------------------------------------
namespace delanx::net {
namespace {

const char* kFixtureDir = "tests/fixtures";

std::string fixture_for(const std::string& url) {
  if (url.find("/repos/HarbourMasters/Starship/releases") != std::string::npos) {
    return "github_starship_releases.json";
  }
  if (url.find("api.github.com") != std::string::npos) return {};  // 404 para el resto
  if (url.find("gitlab.com/api/v4/projects") != std::string::npos) return "gitlab_cli_releases.json";
  if (url.find("release-history") != std::string::npos) return "quiver_history_mgba.json";
  if (url.find("/release-status") != std::string::npos) return "quiver_release_status.json";
  if (url.find("/facets") != std::string::npos) return "quiver_facets.json";
  if (url.find("/apps") != std::string::npos) return "quiver_apps_switch.json";
  return {};
}

}  // namespace

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
  // Caso real: GitHub sin token responde 403 cuando se agota el limite de peticiones.
  if (url.find("/repos/rate/limited/releases") != std::string::npos) {
    const auto limited = paths::read_file("tests/fixtures/github_ratelimit_error.json");
    response.status = 403;
    response.body = limited ? *limited : "{\"message\":\"API rate limit exceeded\"}";
    return response;
  }
  const std::string name = fixture_for(url);
  if (name.empty()) {
    response.status = 404;
    response.body = "{\"message\":\"Not Found\"}";
    return response;
  }
  const std::string path = std::string(kFixtureDir) + "/" + name;
  const auto body = paths::read_file(path);
  if (!body) {
    response.status = 0;
    response.error = "fixture no encontrada: " + path;
    return response;
  }
  response.status = 200;
  response.body = *body;
  return response;
}

Response post_json(const std::string& url, const std::string& body,
                   const std::vector<std::string>& headers, int timeout_seconds) {
  (void)url;
  (void)body;
  (void)headers;
  (void)timeout_seconds;
  Response response;
  response.status = 501;  // el test no necesita POST
  return response;
}

bool download(const std::string& url, const std::string& dest_path, const ProgressFn& progress,
              std::string* error, const std::vector<std::string>& headers) {
  (void)url;
  (void)dest_path;
  (void)progress;
  (void)headers;
  if (error) *error = "download no implementado en el stub";
  return false;
}

}  // namespace delanx::net

// ---------------------------------------------------------------------------
// Utilidades del test
// ---------------------------------------------------------------------------
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

const delanx::Release* find_release(const std::vector<delanx::Release>& releases, const std::string& tag) {
  for (const auto& release : releases) {
    if (release.tag == tag) return &release;
  }
  return nullptr;
}

bool has_asset_named(const delanx::Release& release, const std::string& name) {
  for (const auto& asset : release.assets) {
    if (asset.name == name) return true;
  }
  return false;
}

// ---------------------------------------------------------------------------
// 1. GitHub: parser sobre la respuesta real de HarbourMasters/Starship
// ---------------------------------------------------------------------------
void test_github_releases() {
  section("1. GitHub: releases reales de HarbourMasters/Starship");
  std::string error;
  const std::vector<delanx::Release> releases = delanx::gh::fetch_releases("HarbourMasters/Starship", "", &error);
  check(error.empty(), "sin error de parseo: " + error);
  check(releases.size() >= 2, "se leen varias releases (" + std::to_string(releases.size()) + ")");
  if (releases.empty()) return;

  const delanx::Release* barnard = find_release(releases, "v2.0.0");
  check(barnard != nullptr, "existe la release v2.0.0 (Barnard Alfa)");
  if (barnard != nullptr) {
    check(!barnard->name.empty(), "la release trae nombre");
    check(barnard->published_unix > 1700000000, "published_at convertido a epoch UTC (" +
                                                   std::to_string(barnard->published_unix) + ")");
    check(barnard->assets.size() >= 3, "la release lista varios assets (" +
                                          std::to_string(barnard->assets.size()) + ")");
    check(has_asset_named(*barnard, "Starship-Barnard-Alfa-Switch.zip"),
          "esta el asset de Switch del catalogo");

    // El asset de Switch debe clasificarse como tal y elegirse con el filtro del catalogo.
    delanx::AssetChoice choice = delanx::select_switch_asset(*barnard, "Starship-*-Switch.zip");
    check(choice.index >= 0, "el selector encuentra candidato");
    if (choice.index >= 0) {
      check_eq(barnard->assets[static_cast<size_t>(choice.index)].name, "Starship-Barnard-Alfa-Switch.zip",
               "el asset elegido es el build de Switch");
      check(choice.automatic, "un unico candidato => seleccion automatica");
    }

    // Sin filtro tambien debe reconocerlo por el marcador "switch" del nombre.
    const delanx::AssetChoice unfliltered = delanx::select_switch_asset(*barnard, "");
    check(unfliltered.index >= 0, "sin filtro, el marcador del nombre basta");
    if (unfliltered.index >= 0) {
      check(delanx::str::icontains(barnard->assets[static_cast<size_t>(unfliltered.index)].name, "switch"),
            "el elegido sin filtro lleva 'switch' en el nombre");
    }

    // GitHub solo publica `digest` en assets subidos recientemente. Si viene, debe tener el
    // formato sha256:<hex>; si no viene, Delanx debe quedarse SIN checksum (no inventarlo).
    int with_digest = 0;
    for (const auto& asset : barnard->assets) {
      check(asset.url.rfind("https://", 0) == 0, "el asset trae URL absoluta (" + asset.name + ")");
      if (!asset.digest.empty()) {
        ++with_digest;
        check(delanx::str::starts_with(asset.digest, "sha256:"),
              "el digest usa el prefijo sha256: (" + asset.name + ")");
      }
    }
    std::printf("  info  assets con digest publicado por GitHub: %d de %zu\n", with_digest,
                barnard->assets.size());
    if (with_digest == 0) {
      check(barnard->checksums.empty(), "sin digest publicado no se inventan checksums");
      check(barnard->checksum_for(barnard->assets.front().name).empty(),
            "checksum_for devuelve vacio cuando GitHub no publica digest");
    }
  }

  // Caso negativo REAL: la v1.0.0 del mismo repo no publicaba build de Switch.
  const delanx::Release* old = find_release(releases, "v1.0.0");
  if (old != nullptr) {
    const delanx::AssetChoice none = delanx::select_switch_asset(*old, "Starship-*-Switch.zip");
    check(none.index == -1, "v1.0.0 no ofrece asset de Switch con el filtro del catalogo");
    check(!none.reason.empty(), "se explica el motivo cuando no hay candidato");
  } else {
    check(false, "no se encontro la release v1.0.0 en la fixture");
  }

  // Un repo inexistente debe devolver vector vacio y error, sin lanzar nada.
  std::string missing_error;
  const std::vector<delanx::Release> missing = delanx::gh::fetch_releases("no/existe", "", &missing_error);
  check(missing.empty(), "repo inexistente: sin releases");
  check(!missing_error.empty(), "repo inexistente: se informa del error");
}

// ---------------------------------------------------------------------------
// 1b. Limite de peticiones de GitHub (caso real sin token)
// ---------------------------------------------------------------------------
void test_github_rate_limit() {
  section("1b. GitHub con el limite de peticiones agotado");
  std::string error;
  const std::vector<delanx::Release> releases = delanx::gh::fetch_releases("rate/limited", "", &error);
  check(releases.empty(), "sin token y con el limite agotado no se devuelven releases");
  check(!error.empty(), "se informa del fallo en lugar de fallar en silencio");
  check(delanx::str::icontains(error, "403"), "el mensaje incluye el codigo HTTP (" + error + ")");
  check(delanx::str::icontains(error, "rate limit"),
        "el mensaje conserva el motivo que da GitHub (" + error + ")");

  // Con token configurado el mensaje debe seguir siendo claro; la app ademas sugiere
  // configurarlo (ver App::install_app).
  std::string token_error;
  const std::vector<delanx::Release> with_token =
      delanx::gh::fetch_releases("rate/limited", "ghp_token_de_prueba", &token_error);
  check(with_token.empty(), "con token pero limite agotado: tampoco hay releases");
  check(!token_error.empty(), "con token tambien se informa del error");
}

// ---------------------------------------------------------------------------
// 2. GitLab: parser sobre la respuesta real de gitlab-org/cli
// ---------------------------------------------------------------------------
void test_gitlab_releases() {
  section("2. GitLab: releases reales de gitlab-org/cli");
  std::string error;
  const std::vector<delanx::Release> releases = delanx::gl::fetch_releases("gitlab-org/cli", "", &error);
  check(error.empty(), "sin error de parseo: " + error);
  check(!releases.empty(), "se leen releases (" + std::to_string(releases.size()) + ")");
  if (releases.empty()) return;

  int with_assets = 0;
  int with_url = 0;
  for (const auto& release : releases) {
    check(!release.tag.empty(), "cada release trae tag");
    if (!release.assets.empty()) ++with_assets;
    for (const auto& asset : release.assets) {
      if (asset.url.rfind("http", 0) == 0) ++with_url;
    }
  }
  check(with_assets > 0, "al menos una release trae assets (" + std::to_string(with_assets) + ")");
  check(with_url > 0, "los assets traen URL (" + std::to_string(with_url) + ")");

  // Estructura real de la API v4: cada release anida los ficheros en `assets`
  // (36 links + 4 sources autogeneradas = count 40). Antes este parser los buscaba en la
  // raiz del release y devolvia cero assets en silencio.
  const delanx::Release& newest = releases.front();
  check(newest.assets.size() >= 36, "los assets se leen de assets.links/assets.sources (" +
                                        std::to_string(newest.assets.size()) + ")");
  int sources = 0;
  int links = 0;
  for (const auto& asset : newest.assets) {
    if (delanx::str::icontains(asset.name, newest.tag + ".")) {
      ++sources;
    } else {
      ++links;
    }
  }
  check(sources >= 4, "se generan las fuentes autogeneradas (" + std::to_string(sources) + ")");
  check(links >= 36, "se leen los enlaces del release (" + std::to_string(links) + ")");

  // GitLab no publica digest: Delanx debe asumirlo vacio (y no inventar verificacion).
  for (const auto& release : releases) {
    for (const auto& asset : release.assets) {
      check(asset.digest.empty(), "GitLab no aporta digest (" + asset.name + ")");
    }
  }
}

// ---------------------------------------------------------------------------
// 3. Catalogo de Quiver
// ---------------------------------------------------------------------------
void test_quiver_catalog() {
  section("3. Quiver: catalogo real filtrado por consola Switch");
  const std::string base = "https://api.quiverlauncher.com/api/v1";

  std::string error;
  const delanx::quiver::Facets facets = delanx::quiver::fetch_facets(base, &error);
  check(error.empty(), "facets sin error: " + error);
  check(facets.total > 0, "facets trae total (" + std::to_string(facets.total) + ")");
  bool has_switch = false;
  bool has_nintendo = false;
  for (const auto& console : facets.consoles) {
    if (console.id == "switch") has_switch = true;
    if (console.brand == "Nintendo") has_nintendo = true;
  }
  check(has_switch, "el facet de consolas incluye 'switch'");
  check(has_nintendo, "las consolas se agrupan por marca Nintendo");

  delanx::quiver::Query query;
  query.console = "switch";
  query.limit = 5;
  std::string apps_error;
  const delanx::quiver::Page page = delanx::quiver::fetch_apps(base, query, &apps_error);
  check(apps_error.empty(), "apps sin error: " + apps_error);
  check(!page.items.empty(), "se leen entradas (" + std::to_string(page.items.size()) + ")");
  if (page.items.empty()) return;

  int with_repo = 0;
  for (const auto& item : page.items) {
    check(!item.slug.empty(), "cada entrada trae slug");
    check(!item.name.empty(), "cada entrada trae nombre");
    check(!item.project_type.empty(), "cada entrada trae projectType (" + item.name + ")");
    if (!item.repository.empty()) {
      ++with_repo;
      check(item.repository_source == "github" || item.repository_source == "gitlab",
            "repositorySource valido para " + item.name + " (" + item.repository_source + ")");
    } else {
      // Entradas "manuales": el catalogo de Quiver tambien lista ports sin repositorio
      // publico; Delanx debe aceptarlas sin inventarse una fuente de descarga.
      check(item.repository_source.empty(),
            "sin repositorio publico => sin repositorySource (" + item.name + ")");
    }
    bool mentions_switch = false;
    for (const auto& console : item.consoles) {
      if (console == "switch") mentions_switch = true;
    }
    check(mentions_switch, "la entrada viene etiquetada como Switch (" + item.name + ")");
  }
  std::printf("  info  entradas con repositorio instalable: %d de %zu\n", with_repo, page.items.size());

  const delanx::quiver::CatalogApp& first = page.items.front();
  std::printf("  info  primera entrada: %s (%s/%s, tipo %s)\n", first.name.c_str(),
              first.repository_source.c_str(), first.repository.c_str(), first.project_type.c_str());
}

// ---------------------------------------------------------------------------
// 4. Verificacion de releases (historial de Quiver)
// ---------------------------------------------------------------------------
void test_release_check() {
  section("4. Verificacion: historial real de Quiver para mGBA");
  const std::string base = "https://api.quiverlauncher.com/api/v1";

  // Se toma el tag real de la propia fixture para no depender de datos escritos a mano.
  auto doc = delanx::json::Doc::parse_file("tests/fixtures/quiver_history_mgba.json");
  check(doc.has_value(), "la fixture del historial es JSON valido");
  if (!doc) return;
  const delanx::json::Value items = doc->root().get("items");
  check(items.is_array() && items.size() > 0, "el historial trae releases");
  if (!items.is_array() || items.size() == 0) return;

  std::string tag;
  std::string state;
  for (size_t index = 0; index < items.size(); ++index) {
    const delanx::json::Value item = items.at(index);
    if (!item.string("version").empty()) {
      tag = item.string("version");
      state = item.string("state");
      break;
    }
  }
  check(!tag.empty(), "se obtuvo un tag real del historial (" + tag + ")");

  std::string error;
  const delanx::ReleaseCheckInfo info = delanx::fetch_release_check(base, "mgba", tag, &error);
  check(error.empty(), "sin error de verificacion: " + error);

  if (state == "verified") {
    check(info.state == delanx::Release::Check::Verified, "el estado 'verified' se mapea a Verified");
  } else if (state == "blocked") {
    check(info.state == delanx::Release::Check::Blocked, "el estado 'blocked' se mapea a Blocked");
  } else {
    check(info.state == delanx::Release::Check::Unverified,
          "un estado distinto se mapea a Unverified (estado real '" + state + "')");
  }
  check(!info.reasons.empty(), "siempre hay una razon legible para la interfaz");

  // El historial real trae assets: si alguno publica checksum, debe quedar indexado por nombre.
  int checksums = 0;
  for (size_t index = 0; index < items.size(); ++index) {
    const delanx::json::Value assets = items.at(index).get("assets");
    for (size_t a = 0; a < assets.size(); ++a) {
      const delanx::json::Value asset = assets.at(a);
      const std::string checksum = asset.string("checksum");
      if (!checksum.empty()) {
        const std::string filename = asset.string("filename");
        if (!filename.empty() && info.checksums.count(filename) != 0) {
          ++checksums;
          check(delanx::str::glob_match("[0-9a-f]*", info.checksums.at(filename)) ||
                    info.checksums.at(filename).size() == 64,
                "checksum guardado como hex sin prefijo para " + filename);
        }
      }
    }
  }
  std::printf("  info  checksums indexados desde el historial: %d\n", checksums);

  // Un tag que Quiver no ha visto debe quedar como Unverified con motivo.
  std::string unknown_error;
  const delanx::ReleaseCheckInfo unknown =
      delanx::fetch_release_check(base, "mgba", "0.0.0-no-existe", &unknown_error);
  check(unknown.state == delanx::Release::Check::Unverified, "tag desconocido => Unverified");
  check(!unknown.reasons.empty(), "tag desconocido => motivo informado");

  // apply_check debe trasladar el resultado al release.
  delanx::Release release;
  release.tag = tag;
  delanx::apply_check(&release, info);
  check(release.check == info.state, "apply_check copia el estado");
  check(release.rolling == info.rolling, "apply_check copia 'rolling'");
}

// ---------------------------------------------------------------------------
// 5. Coherencia del catalogo propio con el selector de assets
// ---------------------------------------------------------------------------
void test_catalog_consistency() {
  section("5. Coherencia del catalogo propio (romfs/catalog)");
  auto catalog = delanx::Catalog::load_file("romfs/catalog/switch-catalog.json");
  check(catalog.has_value(), "se carga el catalogo");
  if (!catalog) return;

  int with_filter = 0;
  int nro_paths = 0;
  for (const auto& entry : catalog->entries) {
    if (!entry.release_asset_filter.empty()) {
      ++with_filter;
      check(entry.release_asset_filter.find('*') != std::string::npos ||
                entry.release_asset_filter.find('?') != std::string::npos ||
                entry.release_asset_filter.size() > 2,
            "filtro de asset con forma valida (" + entry.name + ": " + entry.release_asset_filter + ")");
    }
    if (!entry.sw.nro_path.empty()) {
      ++nro_paths;
      check(delanx::str::iends_with(entry.sw.nro_path, ".nro"),
            "nroPath apunta a un .nro (" + entry.name + ": " + entry.sw.nro_path + ")");
    }
    if (!entry.sw.nro.empty()) {
      check(delanx::str::iends_with(entry.sw.nro, ".nro"),
            "switch.nro es un fichero .nro (" + entry.name + ")");
    }
  }
  check(with_filter >= 40, "casi todas las entradas fijan filtro de asset (" + std::to_string(with_filter) + ")");
  check(nro_paths >= 40, "casi todas identifican la ruta del .nro (" + std::to_string(nro_paths) + ")");

  // Las entradas de MrShip deben seguir presentes y ser consistentes.
  int harbour = 0;
  for (const auto& entry : catalog->entries) {
    if (!entry.mrship) continue;
    ++harbour;
    check(delanx::str::starts_with(entry.repository, "HarbourMasters/"),
          "el repo de Harbour Masters es de la organizacion (" + entry.repository + ")");
    check(!entry.requirements.empty(), "explica que debe aportar el usuario (" + entry.name + ")");
  }
  check(harbour == 6, "6 entradas de Harbour Masters (" + std::to_string(harbour) + ")");
}

}  // namespace

int main() {
  std::printf("Delanx — pruebas de la capa de red sobre respuestas reales de las APIs\n");
  test_github_releases();
  test_github_rate_limit();
  test_gitlab_releases();
  test_quiver_catalog();
  test_release_check();
  test_catalog_consistency();

  std::printf("\n----------------------------------------\n");
  std::printf("Comprobaciones: %d   Fallos: %d\n", g_checks, g_failures);
  return g_failures == 0 ? 0 : 1;
}

// Delanx — implementacion de la comprobacion de releases contra Quiver.
//
// Flujo:
//   1. GET {base}/apps/{slug}/release-history?limit=100 y se pagina con cursor (max. 3
//      paginas) buscando la release cuyo version (o el tag del upstreamUrl) coincide con tag.
//   2. Si la release esta en el historial, su `state` manda y sus assets dan los checksums.
//   3. Si no aparece, se mira la version verificada de la app: si coincide con el tag es
//      Verified; si no, Unverified con la razon de que Quiver aun no la ha visto.
//   4. Cualquier fallo de red deja la release como Unverified con una razon explicativa.

#include "services/release_check.hpp"

#include <string>
#include <utility>

#include "core/json.hpp"
#include "core/strings.hpp"
#include "services/http.hpp"
#include "services/quiver_catalog.hpp"

namespace delanx {
namespace {

constexpr int kPageLimit = 100;
constexpr int kMaxPages = 3;  // tope de paginacion: 300 releases como maximo
constexpr int kTimeoutSeconds = 20;

constexpr const char* kReasonNotSeen = "Quiver no ha visto esta release todavia.";
constexpr const char* kReasonNetwork =
    "No se pudo contactar con quiverlauncher.com para comprobar esta release.";
constexpr const char* kReasonVerified =
    "Quiver ha verificado esta release y publica su checksum SHA-256.";
constexpr const char* kReasonUnverified =
    "Quiver no ha verificado esta release: se instalara bajo tu responsabilidad.";

std::string normalize_base(const std::string& base_url) {
  if (base_url.empty()) return std::string(quiver::kDefaultBaseUrl);
  std::string base = base_url;
  while (!base.empty() && base.back() == '/') base.pop_back();
  return base;
}

// "sha256:<hex>" -> "<hex>" en minusculas; una cadena sin prefijo se acepta igual.
std::string normalize_checksum(const std::string& raw) {
  std::string value = str::trim(raw);
  const std::string prefix = "sha256:";
  if (str::starts_with(str::lower(value), prefix)) value = value.substr(prefix.size());
  return str::lower(str::trim(value));
}

// Ultimo segmento de la URL del tag en GitHub/GitLab (".../releases/tag/v1.2" -> "v1.2").
std::string tag_from_upstream(const std::string& upstream_url) {
  const std::string url = str::trim(upstream_url);
  if (url.empty()) return {};
  const size_t slash = url.find_last_of('/');
  std::string tag = slash == std::string::npos ? url : url.substr(slash + 1);
  while (!tag.empty() && tag.back() == '/') tag.pop_back();
  return tag;
}

bool version_matches(const json::Value& node, const std::string& tag) {
  if (str::iequals(str::trim(node.string("version")), tag)) return true;
  const std::string upstream_tag = tag_from_upstream(node.string("upstreamUrl"));
  return !upstream_tag.empty() && str::iequals(upstream_tag, tag);
}

Release::Check parse_state(const std::string& raw) {
  const std::string state = str::lower(str::trim(raw));
  if (state == "verified") return Release::Check::Verified;
  if (state == "blocked") return Release::Check::Blocked;
  // "unverified" y cualquier otro valor publicado se tratan como no verificado.
  return Release::Check::Unverified;
}

// Rellena info a partir del nodo de una release del historial.
void fill_from_history(const json::Value& node, ReleaseCheckInfo* info) {
  info->state = parse_state(node.string("state"));
  info->reasons = node.string_array("reasons");
  info->rolling = node.boolean("rolling");

  const json::Value assets = node.get("assets");
  if (!assets.is_array()) return;
  for (size_t i = 0; i < assets.size(); ++i) {
    const json::Value asset = assets.at(i);
    if (!asset.is_object()) continue;
    const std::string filename = asset.string("filename");
    const std::string hex = normalize_checksum(asset.string("checksum"));
    if (!filename.empty() && !hex.empty()) info->checksums[filename] = hex;
  }
}

// Quiver deja "reasons" vacio en las releases verificadas: la UI necesita algo que mostrar.
void fill_default_reason(ReleaseCheckInfo* info) {
  if (!info->reasons.empty()) return;
  switch (info->state) {
    case Release::Check::Verified:
      info->reasons.emplace_back(kReasonVerified);
      break;
    case Release::Check::Blocked:
      info->reasons.emplace_back(
          "Quiver ha bloqueado esta release: no la instales salvo que sepas lo que haces.");
      break;
    case Release::Check::Unverified:
      info->reasons.emplace_back(kReasonUnverified);
      break;
    case Release::Check::Unknown:
      info->reasons.emplace_back(kReasonNotSeen);
      break;
  }
}

void add_reason(ReleaseCheckInfo* info, const char* reason) {
  for (const std::string& existing : info->reasons) {
    if (existing == reason) return;
  }
  info->reasons.emplace_back(reason);
}

// Ultimo recurso: comprobar la version verificada de la app.
ReleaseCheckInfo fallback_to_verified_version(const std::string& base, const std::string& slug,
                                              const std::string& tag) {
  ReleaseCheckInfo info;
  info.state = Release::Check::Unverified;
  add_reason(&info, kReasonNotSeen);

  quiver::CatalogApp app;
  std::string detail_error;
  if (!quiver::fetch_detail(base, slug, &app, &detail_error)) {
    if (!detail_error.empty()) add_reason(&info, detail_error.c_str());
    return info;
  }
  if (!app.verified_version.empty() && str::iequals(str::trim(app.verified_version), tag)) {
    // La app publica esta misma version como verificada: es la referencia de Quiver.
    info.state = Release::Check::Verified;
    info.verified_version = app.verified_version;
    info.rolling = app.rolling;
    info.reasons.clear();  // sobraba el "no la ha visto todavia"
    fill_default_reason(&info);
  }
  return info;
}

}  // namespace

ReleaseCheckInfo fetch_release_check(const std::string& base_url, const std::string& slug,
                                     const std::string& tag, std::string* error) {
  if (error != nullptr) error->clear();

  const std::string base = normalize_base(base_url);
  ReleaseCheckInfo info;
  if (slug.empty() || tag.empty()) {
    info.state = Release::Check::Unverified;
    add_reason(&info, kReasonNotSeen);
    return info;
  }

  // ---- 1. Buscar la release en el historial (con paginacion acotada). ----
  std::string cursor;
  for (int page_index = 0; page_index < kMaxPages; ++page_index) {
    std::vector<std::pair<std::string, std::string>> params;
    params.emplace_back("limit", str::format("%d", kPageLimit));
    params.emplace_back("cursor", cursor);
    const std::string url =
        net::build_url(base + "/apps/" + str::url_encode(slug) + "/release-history", params);

    const net::Response response = net::get(url, {}, kTimeoutSeconds);
    if (!response.error.empty() || response.status != 200) {
      // Error de red (o del servidor): no se puede afirmar nada sobre la release.
      info.state = Release::Check::Unverified;
      add_reason(&info, kReasonNetwork);
      if (error != nullptr) {
        *error = response.error.empty()
                     ? str::format("Quiver respondio HTTP %ld", response.status)
                     : response.error;
      }
      return info;
    }

    const auto doc = json::Doc::parse(response.body);
    if (!doc.has_value() || !doc->root().is_object()) {
      info.state = Release::Check::Unverified;
      add_reason(&info, kReasonNetwork);
      if (error != nullptr) *error = "respuesta inesperada del historial de releases";
      return info;
    }

    const json::Value root = doc->root();
    const json::Value items = root.get("items");
    if (items.is_array()) {
      for (size_t i = 0; i < items.size(); ++i) {
        const json::Value node = items.at(i);
        if (!node.is_object() || !version_matches(node, tag)) continue;
        fill_from_history(node, &info);
        if (info.state == Release::Check::Verified) {
          // La version verificada de la app es la referencia mas fiable del tag consultado.
          info.verified_version = tag;
        }
        fill_default_reason(&info);
        return info;
      }
    }

    const std::string next = str::trim(root.string("nextCursor"));
    const bool is_done = root.get("isDone").valid() ? root.boolean("isDone", true) : true;
    if (is_done || next.empty()) break;
    cursor = next;
  }

  // ---- 2. No esta en el historial: mirar la version verificada de la app. ----
  return fallback_to_verified_version(base, slug, tag);
}

void apply_check(Release* release, const ReleaseCheckInfo& info) {
  if (release == nullptr) return;
  release->check = info.state;
  release->check_reasons = info.reasons;
  release->verified_version = info.verified_version;
  release->rolling = info.rolling;
  release->checksums = info.checksums;
}

}  // namespace delanx

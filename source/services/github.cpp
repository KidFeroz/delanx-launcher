// Delanx — implementacion del cliente de releases de GitHub.

#include "services/github.hpp"

#include <cstdlib>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

#include "core/json.hpp"
#include "core/strings.hpp"
#include "services/http.hpp"

namespace delanx::gh {
namespace {

constexpr const char* kApiBase = "https://api.github.com";
constexpr int kTimeoutSeconds = 25;

int parse_digits(const std::string& s, size_t pos, size_t count, int fallback) {
  if (pos + count > s.size()) return fallback;
  int value = 0;
  for (size_t i = 0; i < count; ++i) {
    const char c = s[pos + i];
    if (c < '0' || c > '9') return fallback;
    value = value * 10 + (c - '0');
  }
  return value;
}

// Dias desde 1970-01-01 (algoritmo days_from_civil de Howard Hinnant).
int64_t days_from_civil(int64_t year, unsigned month, unsigned day) {
  year -= month <= 2 ? 1 : 0;
  const int64_t era = (year >= 0 ? year : year - 399) / 400;
  const unsigned yoe = static_cast<unsigned>(year - era * 400);
  const unsigned doy = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + static_cast<int64_t>(doe) - 719468;
}

std::string strip_sha256_prefix(const std::string& digest) {
  std::string value = str::trim(digest);
  const std::string prefix = "sha256:";
  if (str::starts_with(str::lower(value), prefix)) value = value.substr(prefix.size());
  return str::lower(str::trim(value));
}

std::vector<std::string> base_headers(const std::string& api_token) {
  std::vector<std::string> headers;
  headers.push_back("Accept: application/vnd.github+json");
  headers.push_back("X-GitHub-Api-Version: 2022-11-28");
  if (!api_token.empty()) headers.push_back("Authorization: Bearer " + api_token);
  return headers;
}

void set_error(std::string* error, const std::string& message) {
  if (error != nullptr) *error = message;
}

// Mensaje de la API de GitHub ({"message":"Not Found"}), si lo hay.
std::string api_message(const std::string& body) {
  const auto doc = json::Doc::parse(body);
  if (!doc.has_value()) return {};
  return str::trim(doc->root().string("message"));
}

bool check_status(const net::Response& response, const std::string& what, std::string* error) {
  if (!response.error.empty()) {
    set_error(error, "error de red (" + what + "): " + response.error);
    return false;
  }
  if (response.status != 200) {
    const std::string detail = api_message(response.body);
    set_error(error, str::format("GitHub respondio HTTP %ld%s%s", response.status,
                                 detail.empty() ? "" : " - ", detail.c_str()));
    return false;
  }
  return true;
}

// Ultimo segmento de una URL (fallback cuando el asset no trae "name").
std::string base_name(const std::string& url) {
  const size_t query = url.find_first_of("?#");
  const std::string clean = query == std::string::npos ? url : url.substr(0, query);
  const size_t slash = clean.find_last_of('/');
  return slash == std::string::npos ? clean : clean.substr(slash + 1);
}

void parse_assets(const json::Value& node, Release* release) {
  const json::Value assets = node.get("assets");
  if (!assets.is_array()) return;
  for (size_t i = 0; i < assets.size(); ++i) {
    const json::Value item = assets.at(i);
    if (!item.is_object()) continue;
    ReleaseAsset asset;
    asset.name = item.string("name");
    asset.url = item.string("browser_download_url");
    asset.size = item.integer("size");
    const std::string digest = item.string("digest");
    if (!digest.empty()) {
      asset.digest = digest;  // se guarda tal cual ("sha256:<hex>")
      const std::string hex = strip_sha256_prefix(digest);
      if (!hex.empty() && !asset.name.empty()) release->checksums[asset.name] = hex;
    }
    if (asset.name.empty() && !asset.url.empty()) asset.name = base_name(asset.url);
    if (!asset.name.empty() && !asset.url.empty()) release->assets.push_back(std::move(asset));
  }
}

bool parse_release(const json::Value& node, Release* out, std::string* error) {
  if (!node.is_object()) {
    set_error(error, "respuesta inesperada de la API de GitHub");
    return false;
  }
  out->tag = node.string("tag_name");
  if (out->tag.empty()) out->tag = node.string("name");
  out->name = node.string("name");
  out->body = node.string("body");
  out->published_at = node.string("published_at");
  if (out->published_at.empty()) out->published_at = node.string("created_at");
  out->published_unix = parse_iso8601_utc(out->published_at);
  out->prerelease = node.boolean("prerelease");
  parse_assets(node, out);
  return true;
}

}  // namespace

int64_t parse_iso8601_utc(const std::string& text) {
  const std::string s = str::trim(text);
  if (s.size() < 19) return 0;
  // "YYYY-MM-DDTHH:MM:SS" — se acepta 'T' o espacio como separador.
  if (s[4] != '-' || s[7] != '-' || (s[10] != 'T' && s[10] != 't' && s[10] != ' ') ||
      s[13] != ':' || s[16] != ':') {
    return 0;
  }
  const int year = parse_digits(s, 0, 4, -1);
  const int month = parse_digits(s, 5, 2, -1);
  const int day = parse_digits(s, 8, 2, -1);
  const int hour = parse_digits(s, 11, 2, -1);
  const int minute = parse_digits(s, 14, 2, -1);
  const int second = parse_digits(s, 17, 2, -1);
  if (year < 0 || month < 1 || month > 12 || day < 1 || day > 31 || hour < 0 || hour > 23 ||
      minute < 0 || minute > 59 || second < 0 || second > 60) {
    return 0;
  }

  // Desfase horario opcional: "Z", "+HH:MM", "-HH:MM" o "+HHMM".
  int offset_minutes = 0;
  size_t i = 19;
  if (i < s.size() && s[i] == '.') {
    ++i;
    while (i < s.size() && s[i] >= '0' && s[i] <= '9') ++i;
  }
  if (i < s.size()) {
    const char sign = s[i];
    if (sign == '+' || sign == '-') {
      const int oh = parse_digits(s, i + 1, 2, -1);
      // RFC 3339 admite "+HH:MM" y la variante compacta "+HHMM": se salta los ':'.
      size_t minute_pos = i + 3;
      if (minute_pos < s.size() && s[minute_pos] == ':') ++minute_pos;
      const int om = parse_digits(s, minute_pos, 2, -1);
      if (oh < 0 || om < 0) return 0;
      offset_minutes = oh * 60 + om;
      if (sign == '-') offset_minutes = -offset_minutes;
    }
    // 'Z' o cualquier otro sufijo: se ignora.
  }

  const int64_t days = days_from_civil(year, static_cast<unsigned>(month),
                                       static_cast<unsigned>(day));
  const int64_t seconds = days * 86400 + hour * 3600 + minute * 60 + second;
  return seconds - static_cast<int64_t>(offset_minutes) * 60;
}

std::vector<Release> fetch_releases(const std::string& repository, const std::string& api_token,
                                    std::string* error) {
  std::vector<Release> releases;
  if (error != nullptr) error->clear();
  if (repository.empty()) {
    set_error(error, "repositorio vacio");
    return releases;
  }

  const int per_page = api_token.empty() ? 30 : 100;
  const std::string url =
      str::format("%s/repos/%s/releases?per_page=%d", kApiBase, repository.c_str(), per_page);
  const net::Response response = net::get(url, base_headers(api_token), kTimeoutSeconds);
  if (!check_status(response, "releases de " + repository, error)) return releases;

  const auto doc = json::Doc::parse(response.body);
  if (!doc.has_value() || !doc->root().is_array()) {
    set_error(error, "respuesta inesperada de la API de GitHub");
    return releases;
  }

  const json::Value root = doc->root();
  releases.reserve(root.size());
  for (size_t i = 0; i < root.size(); ++i) {
    Release release;
    if (parse_release(root.at(i), &release, error)) releases.push_back(std::move(release));
  }
  return releases;
}

bool fetch_release_by_tag(const std::string& repository, const std::string& tag,
                          const std::string& api_token, Release* out, std::string* error) {
  if (error != nullptr) error->clear();
  if (out == nullptr) {
    set_error(error, "salida nula");
    return false;
  }
  if (repository.empty() || tag.empty()) {
    set_error(error, "repositorio o tag vacio");
    return false;
  }

  const std::string url = str::format("%s/repos/%s/releases/tags/%s", kApiBase,
                                      repository.c_str(), str::url_encode(tag).c_str());
  const net::Response response = net::get(url, base_headers(api_token), kTimeoutSeconds);
  if (!check_status(response, "release " + tag + " de " + repository, error)) return false;

  const auto doc = json::Doc::parse(response.body);
  if (!doc.has_value()) {
    set_error(error, "respuesta inesperada de la API de GitHub");
    return false;
  }
  return parse_release(doc->root(), out, error);
}

}  // namespace delanx::gh

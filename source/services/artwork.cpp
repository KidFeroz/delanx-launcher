#include "services/artwork.hpp"

#include <deque>
#include <set>
#include <string>

#include "core/paths.hpp"
#include "services/http.hpp"

namespace delanx {
namespace {

struct Pending {
  std::string url;
  std::string dest;
};

std::deque<Pending>& queue() {
  static std::deque<Pending> instance;
  return instance;
}

// Carpetas ya vistas en esta sesion: evita reintentar la misma caratula en bucle.
std::set<std::string>& attempted() {
  static std::set<std::string> instance;
  return instance;
}

bool download_artwork(const std::string& url, const std::string& dest, std::string* error) {
  if (url.empty()) {
    if (error != nullptr) *error = "la entrada no publica caratula";
    return false;
  }
  if (paths::exists(dest)) return true;
  paths::make_dirs(paths::artwork_dir());
  std::string download_error;
  if (!net::download(url, dest, net::ProgressFn(), &download_error)) {
    if (error != nullptr) *error = download_error;
    return false;
  }
  return true;
}

}  // namespace

void queue_artwork(const AppEntry& app) {
  if (app.app_icon_url.empty() || app.folder_name.empty()) return;
  const std::string dest = app.icon_cache_path();
  if (paths::exists(dest)) return;
  if (attempted().count(app.folder_name) != 0) return;
  attempted().insert(app.folder_name);
  queue().push_back(Pending{app.app_icon_url, dest});
}

int artwork_queue_size() { return static_cast<int>(queue().size()); }

void clear_artwork_queue() { queue().clear(); }

bool ensure_artwork(const AppEntry& app, std::string* error) {
  return download_artwork(app.app_icon_url, app.icon_cache_path(), error);
}

int process_artwork_queue(int budget) {
  int downloaded = 0;
  while (downloaded < budget && !queue().empty()) {
    const Pending item = queue().front();
    queue().pop_front();
    std::string error;
    if (download_artwork(item.url, item.dest, &error)) {
      ++downloaded;
    } else {
      // El fallo no interrumpe el resto de la cola.
      continue;
    }
  }
  return downloaded;
}

}  // namespace delanx

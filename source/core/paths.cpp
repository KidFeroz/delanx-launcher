#include "core/paths.hpp"

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>

#include "core/strings.hpp"

namespace delanx::paths {
namespace {

constexpr const char* kRoot = "sdmc:/switch/Delanx";

// Raiz alternativa usada solo por las pruebas de PC (vacia = raiz real de la SD).
std::string& root_override() {
  static std::string value;
  return value;
}

// Prefijo que sustituye a "sdmc:" solo en las pruebas de PC.
std::string& sd_prefix_override() {
  static std::string value;
  return value;
}

bool make_dirs_impl(const std::string& path) {
  if (path.empty()) return false;
  if (is_dir(path)) return true;
  struct stat st;
  if (stat(path.c_str(), &st) == 0) return S_ISDIR(st.st_mode);
  const std::string parent = dirname(path);
  if (!parent.empty() && parent != path) {
    if (!make_dirs_impl(parent)) return false;
  }
  if (mkdir(path.c_str(), 0777) == 0) return true;
  return is_dir(path);
}

}  // namespace

std::string root() {
  const std::string& override_value = root_override();
  return override_value.empty() ? std::string(kRoot) : override_value;
}

void set_root_override(const std::string& root) { root_override() = root; }

std::string library_file() { return join(root(), "apps.json"); }
std::string settings_file() { return join(root(), "settings.json"); }
std::string apps_dir() { return join(root(), "Apps"); }
std::string cache_dir() { return join(root(), "Cache"); }
std::string temp_dir() { return join(root(), "Temp"); }
std::string artwork_dir() { return join(cache_dir(), "artwork"); }
std::string log_file() { return join(root(), "delanx.log"); }

bool ensure_layout() {
  return make_dirs(root()) && make_dirs(apps_dir()) && make_dirs(cache_dir()) &&
         make_dirs(temp_dir()) && make_dirs(artwork_dir());
}

std::string join(const std::string& a, const std::string& b) {
  if (a.empty()) return b;
  if (b.empty()) return a;
  if (a.back() == '/' || a.back() == ':') return a + b;
  return a + "/" + b;
}

std::string basename(const std::string& path) {
  const size_t pos = path.find_last_of('/');
  return pos == std::string::npos ? path : path.substr(pos + 1);
}

std::string dirname(const std::string& path) {
  const size_t pos = path.find_last_of('/');
  if (pos == std::string::npos) return {};
  if (pos == 0) return "/";
  return path.substr(0, pos);
}

std::string extension(const std::string& path) {
  const std::string name = basename(path);
  const size_t pos = name.find_last_of('.');
  if (pos == std::string::npos) return {};
  return str::lower(name.substr(pos));
}

std::string to_sd_path(const std::string& path) {
  if (path.empty()) return path;
  const std::string& prefix = sd_prefix_override();
  // Idempotencia: si ya se resolvio antes con el mismo prefijo (o ya viene con "sdmc:"),
  // no se vuelve a prefijar. En la consola no hay prefijo y "sdmc:" ya es idempotente.
  if (!prefix.empty() && str::starts_with(path, prefix)) return path;
  std::string result;
  if (str::istarts_with(path, "sdmc:")) {
    result = path;
  } else if (path[0] == '/') {
    result = "sdmc:" + path;
  } else {
    result = std::string("sdmc:/") + path;
  }
  if (!prefix.empty() && str::starts_with(result, "sdmc:")) {
    result = prefix + result.substr(5);
  }
  return result;
}

void set_sd_prefix_override(const std::string& prefix) { sd_prefix_override() = prefix; }

bool exists(const std::string& path) {
  struct stat st;
  return stat(path.c_str(), &st) == 0;
}

bool is_dir(const std::string& path) {
  struct stat st;
  if (stat(path.c_str(), &st) != 0) return false;
  return S_ISDIR(st.st_mode);
}

bool make_dirs(const std::string& path) { return make_dirs_impl(path); }

std::optional<std::string> read_file(const std::string& path) {
  FILE* file = fopen(path.c_str(), "rb");
  if (!file) return std::nullopt;
  std::string data;
  char buffer[8192];
  size_t read = 0;
  while ((read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
    data.append(buffer, read);
  }
  fclose(file);
  return data;
}

bool write_file(const std::string& path, const std::string& data) {
  FILE* file = fopen(path.c_str(), "wb");
  if (!file) return false;
  const size_t written = data.empty() ? 0 : fwrite(data.data(), 1, data.size(), file);
  const bool ok = (written == data.size());
  fclose(file);
  return ok;
}

bool write_file_atomic(const std::string& path, const std::string& data) {
  const std::string temp = path + ".tmp";
  if (!write_file(temp, data)) return false;
  remove(path.c_str());  // newlib no sobrescribe con rename en todos los casos
  if (rename(temp.c_str(), path.c_str()) == 0) return true;
  remove(temp.c_str());
  return false;
}

uint64_t file_size(const std::string& path) {
  struct stat st;
  if (stat(path.c_str(), &st) != 0) return 0;
  return static_cast<uint64_t>(st.st_size);
}

bool remove_file(const std::string& path) { return remove(path.c_str()) == 0; }

bool remove_dir_recursive(const std::string& path) {
  if (!is_dir(path)) return remove_file(path);
  DIR* dir = opendir(path.c_str());
  if (!dir) return false;
  bool ok = true;
  while (struct dirent* entry = readdir(dir)) {
    const std::string name = entry->d_name;
    if (name == "." || name == "..") continue;
    const std::string child = join(path, name);
    if (is_dir(child)) {
      if (!remove_dir_recursive(child)) ok = false;
    } else if (!remove_file(child)) {
      ok = false;
    }
  }
  closedir(dir);
  if (rmdir(path.c_str()) != 0) ok = false;
  return ok;
}

bool copy_file(const std::string& from, const std::string& to) {
  FILE* in = fopen(from.c_str(), "rb");
  if (!in) return false;
  FILE* out = fopen(to.c_str(), "wb");
  if (!out) {
    fclose(in);
    return false;
  }
  char buffer[16384];
  size_t read = 0;
  bool ok = true;
  while ((read = fread(buffer, 1, sizeof(buffer), in)) > 0) {
    if (fwrite(buffer, 1, read, out) != read) {
      ok = false;
      break;
    }
  }
  fclose(in);
  fclose(out);
  return ok;
}

std::vector<std::string> list_dir(const std::string& path) {
  std::vector<std::string> out;
  DIR* dir = opendir(path.c_str());
  if (!dir) return out;
  while (struct dirent* entry = readdir(dir)) {
    const std::string name = entry->d_name;
    if (name == "." || name == "..") continue;
    out.push_back(name);
  }
  closedir(dir);
  return out;
}

}  // namespace delanx::paths

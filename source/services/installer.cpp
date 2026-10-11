// Delanx — instalacion en la SD: descarga (curl), verificacion SHA-256 propia,
// extraccion con libarchive y metadatos al estilo Quiver.
#include "services/installer.hpp"

#include <archive.h>
#include <archive_entry.h>
#if defined(__SWITCH__)
#include <sys/statvfs.h>  // free_space_bytes(); en las pruebas de PC lo suple host_compat.h
#endif

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "core/paths.hpp"
#include "core/sha256.hpp"
#include "core/strings.hpp"
#include "services/http.hpp"

namespace delanx {
namespace {

// Extensiones que consideramos aportadas por el usuario (ROMs, datos de juego,
// mods). Nunca se sobrescriben ni se borran sin permiso explicito.
// Nota: la especificacion listaba ".sfc" dos veces; se deja una sola.
const char* const kUserExtensions[] = {
    ".otr", ".o2r", ".z64", ".n64", ".v64", ".iso", ".gba", ".nes", ".sfc", ".wad",
    ".pk3", ".pk4", ".pak", ".lmp", ".mpq", ".zip", ".rom", ".bin", ".md",
};

bool is_user_file(const std::string& name) {
  const std::string ext = paths::extension(name);
  // Un fichero SIN extension (p. ej. "baserom") no es nuestro: nunca se borra ni se pisa.
  if (ext.empty()) return true;
  for (const char* candidate : kUserExtensions) {
    if (ext == candidate) return true;
  }
  return false;
}

// Margen del 10% sobre el tamano del asset para el staging temporal.
uint64_t space_needed(uint64_t asset_bytes) { return asset_bytes + asset_bytes / 10u + (1024u * 1024u); }

// --- normalizacion de rutas ------------------------------------------------

struct NormalizedPath {
  bool valid = false;
  bool absolute = false;
  std::vector<std::string> components;
};

// Normaliza separadores "/" y "\", quita el prefijo de unidad, resuelve "." y ".."
// y marca como invalido (posible path traversal) cualquier intento de salir de la raiz.
NormalizedPath normalize_archive_path(const std::string& raw) {
  NormalizedPath out;

  std::string s;
  s.reserve(raw.size());
  for (char ch : raw) s.push_back(ch == '\\' ? '/' : ch);

  if (!s.empty() && (s[0] == '/' || s[0] == '\\')) {
    out.absolute = true;
    // Se quita la barra inicial SIEMPRE: asi "/C:/evil/x" queda como "C:/evil/x" y la
    // comprobacion de unidad de abajo puede rechazarlo (antes quedaba como "C:/evil/x"
    // dentro del destino, creando arboles basura).
    s = s.substr(1);
  }
  if (s.size() >= 2 && s[1] == ':' && ((s[0] >= 'A' && s[0] <= 'Z') || (s[0] >= 'a' && s[0] <= 'z'))) {
    out.valid = false;
    return out;
  }

  size_t start = 0;
  bool escaped = false;
  while (start <= s.size()) {
    const size_t slash = s.find('/', start);
    const size_t end = (slash == std::string::npos) ? s.size() : slash;
    const std::string part = s.substr(start, end - start);
    if (!part.empty() && part != ".") {
      if (part == "..") {
        if (out.components.empty()) {
          escaped = true;  // sale de dest_dir: entrada rechazada
          break;
        }
        out.components.pop_back();
      } else {
        out.components.push_back(part);
      }
    }
    if (slash == std::string::npos) break;
    start = slash + 1;
  }

  out.valid = !escaped;
  return out;
}

std::string join_components(const std::string& base, const std::vector<std::string>& parts) {
  std::string out = base;
  for (const std::string& part : parts) {
    if (!out.empty() && out.back() != '/') out.push_back('/');
    out += part;
  }
  return out;
}

bool sd_has_space(uint64_t needed) {
  const uint64_t free_bytes = free_space_bytes();
  return free_bytes == 0 || free_bytes >= needed;
}

// --- ficheros y carpetas ---------------------------------------------------

void ensure_parent_dirs(const std::string& file_path) {
  const std::string parent = paths::dirname(file_path);
  if (!parent.empty() && !paths::is_dir(parent)) paths::make_dirs(parent);
}

std::vector<std::string> list_user_files(const std::string& dir) {
  std::vector<std::string> found;
  if (!paths::is_dir(dir)) return found;
  for (const std::string& name : paths::list_dir(dir)) {
    const std::string full = paths::join(dir, name);
    if (paths::is_dir(full)) continue;
    if (is_user_file(name)) found.push_back(full);
  }
  return found;
}

// Borra todo lo que hay en dir menos user_files (nunca sobrescribe datos del usuario).
// Devuelve los directorios vaciados para intentar podarlos despues.
void purge_dir_before_extract(const std::string& dir, const std::vector<std::string>& user_files,
                              std::vector<std::string>* emptied_dirs) {
  if (!paths::is_dir(dir)) return;
  for (const std::string& name : paths::list_dir(dir)) {
    const std::string full = paths::join(dir, name);
    if (paths::is_dir(full)) {
      if (emptied_dirs != nullptr) emptied_dirs->push_back(full);
      continue;
    }
    bool keep = false;
    for (const std::string& keep_path : user_files) {
      if (keep_path == full) {
        keep = true;
        break;
      }
    }
    if (!keep) paths::remove_file(full);
  }
}

bool dir_is_empty(const std::string& dir) {
  if (!paths::is_dir(dir)) return false;
  return paths::list_dir(dir).empty();
}

// Un destino "compartido" aloja muchas cosas ajenas (la raiz de la SD o /switch): vaciarlo o
// borrarlo recursivamente destruiria instalaciones que no son nuestras (Atmosphere, hbmenu,
// payloads, partidas...). En esos destinos solo se toca lo que Delanx registro como suyo.
bool is_shared_target(const std::string& dir) {
  std::string value = dir;
  while (!value.empty() && value.back() == '/') value.pop_back();
  std::string root = paths::to_sd_path("/");
  while (!root.empty() && root.back() == '/') root.pop_back();
  return value == root || value == paths::join(root, "switch");
}

// Manifiesto de lo instalado por Delanx: permite desinstalar de forma precisa.
std::string manifest_path(const AppEntry& app) {
  return paths::join(paths::join(paths::cache_dir(), "installed"), app.folder_name + ".manifest");
}

bool write_manifest(const AppEntry& app, const std::vector<std::string>& files) {
  const std::string path = manifest_path(app);
  paths::make_dirs(paths::dirname(path));
  std::string text;
  for (const std::string& file : files) {
    text += file;
    text += '\n';
  }
  return paths::write_file_atomic(path, text);
}

std::vector<std::string> read_manifest(const AppEntry& app) {
  std::vector<std::string> files;
  const auto text = paths::read_file(manifest_path(app));
  if (!text) return files;
  for (const std::string& line : str::split(*text, '\n')) {
    const std::string trimmed = str::trim(line);
    if (!trimmed.empty()) files.push_back(trimmed);
  }
  return files;
}

// true si `path` es una ruta protegida o cae dentro de una (fichero o carpeta del usuario).
bool is_protected(const std::string& path, const std::vector<std::string>& protected_paths) {
  for (const std::string& candidate : protected_paths) {
    if (candidate.empty()) continue;
    if (path == candidate) return true;
    const std::string prefix = candidate.back() == '/' ? candidate : candidate + "/";
    if (path.size() > prefix.size() && path.compare(0, prefix.size(), prefix) == 0) return true;
  }
  return false;
}

// Rutas que el catalogo declara como aportadas por el usuario (ficheros o carpetas).
std::vector<std::string> declared_user_paths(const AppEntry& app) {
  std::vector<std::string> result;
  for (const auto& required : app.sw.required_user_files) {
    const std::string path = paths::to_sd_path(required.path);
    if (!path.empty()) result.push_back(path);
  }
  return result;
}

// Poda las carpetas que hayan quedado vacias entre `dir` (excluido) y cada ruta indicada.
void prune_empty_parents(const std::string& dir, const std::vector<std::string>& files) {
  std::string base = dir;
  while (!base.empty() && base.back() == '/') base.pop_back();
  std::vector<std::string> dirs;
  for (const std::string& file : files) {
    std::string parent = paths::dirname(file);
    while (parent.size() > base.size() && parent.compare(0, base.size(), base) == 0) {
      dirs.push_back(parent);
      const std::string next = paths::dirname(parent);
      if (next == parent) break;
      parent = next;
    }
  }
  for (auto it = dirs.rbegin(); it != dirs.rend(); ++it) {
    if (paths::is_dir(*it) && dir_is_empty(*it)) paths::remove_dir_recursive(*it);
  }
}

std::string create_staging_dir(const std::string& asset_name) {
  paths::make_dirs(paths::temp_dir());
  std::string safe = asset_name.empty() ? std::string("asset") : asset_name;
  for (char& ch : safe) {
    if (ch == '/' || ch == '\\' || ch == ':') ch = '_';
  }
  return paths::join(paths::temp_dir(), safe);
}

// --- extraccion ------------------------------------------------------------

bool looks_like_archive(const std::string& name) {
  const std::string ext = paths::extension(name);
  return ext == ".zip" || ext == ".7z" || ext == ".tar" || ext == ".gz" || ext == ".xz" || ext == ".zst" ||
         ext == ".tgz" || ext == ".bz2" || ext == ".lz4";
}

std::string show_name(const std::string& value) { return value.empty() ? std::string("(sin nombre)") : value; }

}  // namespace

const char* install_status_text(InstallStatus status) {
  switch (status) {
    case InstallStatus::Ok:
      return "Instalado";
    case InstallStatus::Cancelled:
      return "Cancelado por el usuario";
    case InstallStatus::DownloadFailed:
      return "Fallo la descarga";
    case InstallStatus::HashMismatch:
      return "El checksum no coincide";
    case InstallStatus::ExtractFailed:
      return "Fallo la extraccion";
    case InstallStatus::MissingAsset:
      return "El release no tiene un asset compatible";
    case InstallStatus::NotEnoughSpace:
      return "No hay espacio en la SD";
    case InstallStatus::UnsupportedAsset:
      return "Formato de asset no soportado";
    case InstallStatus::Unknown:
      break;
  }
  return "Error desconocido";
}

uint64_t free_space_bytes() {
  struct statvfs info;
  memset(&info, 0, sizeof(info));
  if (statvfs("sdmc:/", &info) != 0) return 0;
  if (info.f_frsize == 0) return 0;  // dato no fiable: se trata como "desconocido"
  return static_cast<uint64_t>(info.f_bavail) * static_cast<uint64_t>(info.f_frsize);
}

bool extract_archive(const std::string& archive_path, const std::string& dest_dir, bool strip_root,
                     std::string* error, const std::vector<std::string>& protected_paths,
                     std::vector<std::string>* written) {
  if (!paths::exists(archive_path)) {
    if (error != nullptr) *error = "No existe el archivo " + archive_path;
    return false;
  }
  if (dest_dir.empty()) {
    if (error != nullptr) *error = "Destino de extraccion vacio";
    return false;
  }
  if (!paths::make_dirs(dest_dir) && !paths::is_dir(dest_dir)) {
    if (error != nullptr) *error = "No se pudo crear " + dest_dir;
    return false;
  }

  struct archive* reader = archive_read_new();
  if (reader == nullptr) {
    if (error != nullptr) *error = "libarchive no pudo inicializarse";
    return false;
  }
  archive_read_support_filter_all(reader);
  archive_read_support_format_all(reader);

  if (archive_read_open_filename(reader, archive_path.c_str(), 128 * 1024) != ARCHIVE_OK) {
    if (error != nullptr) {
      const char* detail = archive_error_string(reader);
      *error = std::string("No se pudo abrir el archivo: ") + (detail != nullptr ? detail : "formato desconocido");
    }
    archive_read_free(reader);
    return false;
  }

  struct archive_entry* entry = nullptr;
  bool ok = true;
  std::string failure;
  size_t extracted = 0;
  size_t skipped = 0;
  size_t preserved = 0;  // ficheros del usuario que se han respetado
  std::string root_component;
  bool single_root = strip_root;

  // Pasada 1: decidir que componente raiz hay que quitar.
  while (archive_read_next_header(reader, &entry) == ARCHIVE_OK) {
    const char* raw_name = archive_entry_pathname(entry);
    if (raw_name == nullptr) continue;
    const char* hardlink = archive_entry_hardlink(entry);
    if (hardlink != nullptr && hardlink[0] != '\0') continue;  // entrada de enlace: no decide la raiz

    const NormalizedPath path = normalize_archive_path(raw_name);
    if (!path.valid || path.components.empty()) continue;
    if (single_root && root_component.empty()) root_component = path.components[0];
    if (path.components[0] != root_component) single_root = false;
  }

  archive_read_close(reader);
  archive_read_free(reader);

  // Pasada 2: extraer.
  reader = archive_read_new();
  if (reader == nullptr) {
    if (error != nullptr) *error = "libarchive no pudo inicializarse";
    return false;
  }
  archive_read_support_filter_all(reader);
  archive_read_support_format_all(reader);
  if (archive_read_open_filename(reader, archive_path.c_str(), 128 * 1024) != ARCHIVE_OK) {
    if (error != nullptr) *error = "No se pudo reabrir el archivo";
    archive_read_free(reader);
    return false;
  }

  std::vector<char> buffer(256 * 1024);

  while (archive_read_next_header(reader, &entry) == ARCHIVE_OK) {
    const char* raw_name = archive_entry_pathname(entry);
    if (raw_name == nullptr) {
      archive_read_data_skip(reader);
      continue;
    }

    NormalizedPath path = normalize_archive_path(raw_name);
    if (!path.valid) {
      // Path traversal: se rechaza la entrada y se avisa.
      ok = false;
      if (failure.empty()) failure = std::string("entrada rechazada por path traversal: ") + raw_name;
      ++skipped;
      archive_read_data_skip(reader);
      continue;
    }

    bool strip_this = single_root;
    if (strip_this) {
      if (!path.components.empty() && path.components[0] == root_component) path.components.erase(path.components.begin());
      if (path.components.empty()) continue;  // era el propio directorio raiz: ya no hace falta
    }
    if (path.components.empty()) {
      archive_read_data_skip(reader);
      continue;
    }

    const std::string out_path = join_components(dest_dir, path.components);
    const mode_t type = archive_entry_filetype(entry);

    if (type == AE_IFDIR) {
      paths::make_dirs(out_path);
      continue;
    }
    // Los enlaces simbolicos quedan fuera: un .zip preparado puede escribir asi
    // fuera de la carpeta de destino y en FAT ademas no aportan nada util.
    if (type == AE_IFLNK || type == AE_IFSOCK || type == AE_IFIFO || type == AE_IFBLK || type == AE_IFCHR) {
      ++skipped;
      archive_read_data_skip(reader);
      continue;
    }

    // El destino ya esta comprobado: normalizado + componentes bajo dest_dir.
    if (out_path.size() < dest_dir.size() || out_path.compare(0, dest_dir.size(), dest_dir) != 0) {
      ++skipped;
      archive_read_data_skip(reader);
      continue;
    }

    // Nunca se sobrescribe un fichero que aporta el usuario: ni los que ya estan en el
    // destino con extension de datos de juego, ni los que el catalogo declara como suyos
    // (requiredUserFiles), ni nada que caiga dentro de una carpeta declarada por el catalogo.
    if ((paths::exists(out_path) && is_user_file(out_path)) || is_protected(out_path, protected_paths)) {
      ++preserved;
      archive_read_data_skip(reader);
      continue;
    }

    ensure_parent_dirs(out_path);
    FILE* out = fopen(out_path.c_str(), "wb");
    if (out == nullptr) {
      if (failure.empty()) failure = "no se pudo escribir " + paths::basename(out_path);
      ok = false;
      ++skipped;
      archive_read_data_skip(reader);
      continue;
    }

    la_ssize_t read_bytes = 0;
    while ((read_bytes = archive_read_data(reader, buffer.data(), buffer.size())) > 0) {
      if (fwrite(buffer.data(), 1, static_cast<size_t>(read_bytes), out) != static_cast<size_t>(read_bytes)) {
        if (failure.empty()) failure = "escritura incompleta en " + paths::basename(out_path);
        ok = false;
        break;
      }
    }
    fclose(out);
    if (read_bytes < 0) {
      if (failure.empty()) {
        const char* detail = archive_error_string(reader);
        failure = std::string("error de datos en ") + paths::basename(out_path) + ": " +
                  (detail != nullptr ? detail : "desconocido");
      }
      ok = false;
      paths::remove_file(out_path);
      continue;
    }
    ++extracted;
    if (written != nullptr) written->push_back(out_path);
  }

  archive_read_close(reader);
  archive_read_free(reader);

  if (error != nullptr) {
    if (!failure.empty()) {
      *error = failure;
    } else if (skipped > 0 || preserved > 0) {
      *error = "extraidos " + std::to_string(static_cast<long long>(extracted)) + " ficheros, omitidos " +
               std::to_string(static_cast<long long>(skipped));
      if (preserved > 0) {
        *error += ", conservados " + std::to_string(static_cast<long long>(preserved)) +
                  " ficheros tuyos";
      }
    } else {
      error->clear();
    }
  }
  return ok;
}

std::string find_launchable_nro(const std::string& target_dir, const std::string& preferred_nro) {
  if (target_dir.empty()) return std::string();
  const std::string base = paths::to_sd_path(target_dir);

  // 1) El .nro declarado por la entrada (por nombre, ruta relativa o ruta completa).
  const std::string preferred = str::trim(preferred_nro);
  if (!preferred.empty()) {
    std::vector<std::string> candidates;
    candidates.push_back(paths::join(base, paths::basename(preferred)));
    candidates.push_back(paths::join(base, preferred));
    // Ruta ya absoluta ("/switch/x/y.nro" o "sdmc:/switch/x/y.nro"): tal cual.
    const bool absolute = preferred[0] == '/' || preferred.find(':') != std::string::npos;
    if (absolute) candidates.push_back(paths::to_sd_path(preferred));
    for (const std::string& candidate : candidates) {
      if (paths::exists(candidate) && !paths::is_dir(candidate) && paths::extension(candidate) == ".nro") {
        return candidate;
      }
    }
  }

  // 2) Primer .nro directamente en el directorio. Se prefiere uno con el mismo
  //    nombre que la carpeta (fast path de Sphaira) y si no, el primero que aparezca.
  const std::string folder = paths::basename(base);
  std::string first;
  std::vector<std::string> inner_candidates;
  for (const std::string& name : paths::list_dir(base)) {
    const std::string full = paths::join(base, name);
    if (paths::is_dir(full)) {
      inner_candidates.push_back(full);
      continue;
    }
    if (paths::extension(name) != ".nro") continue;
    if (str::iequals(name, folder + ".nro")) return full;  // fast path de Sphaira
    if (first.empty()) first = full;
  }
  if (!first.empty()) return first;

  // 3) Un solo nivel de subcarpeta (Sphaira no busca mas profundo).
  for (const std::string& sub : inner_candidates) {
    const std::string fast = paths::join(sub, paths::basename(sub) + ".nro");
    if (paths::exists(fast) && !paths::is_dir(fast)) return fast;
    for (const std::string& inner : paths::list_dir(sub)) {
      const std::string full = paths::join(sub, inner);
      if (!paths::is_dir(full) && paths::extension(inner) == ".nro") return full;
    }
  }

  return std::string();
}

// Prefijo comun (por componentes) de todas las entradas de un archivo. Devuelve vacio si la
// primera componente ya difiere. Sirve para saber si un paquete trae el layout de la SD
// ("switch/<app>/...") antes de extraerlo.
std::vector<std::string> archive_root_components(const std::string& archive_path) {
  std::vector<std::string> prefix;
  bool first = true;

  struct archive* reader = archive_read_new();
  if (reader == nullptr) return prefix;
  archive_read_support_filter_all(reader);
  archive_read_support_format_all(reader);
  if (archive_read_open_filename(reader, archive_path.c_str(), 64 * 1024) != ARCHIVE_OK) {
    archive_read_free(reader);
    return prefix;
  }

  struct archive_entry* entry = nullptr;
  while (archive_read_next_header(reader, &entry) == ARCHIVE_OK) {
    const char* raw_name = archive_entry_pathname(entry);
    if (raw_name == nullptr) continue;
    const NormalizedPath path = normalize_archive_path(raw_name);
    if (!path.valid || path.components.empty()) continue;

    if (first) {
      prefix = path.components;
      first = false;
      continue;
    }
    size_t common = 0;
    while (common < prefix.size() && common < path.components.size() &&
           prefix[common] == path.components[common]) {
      ++common;
    }
    prefix.resize(common);
    if (prefix.empty()) break;
  }

  archive_read_close(reader);
  archive_read_free(reader);
  if (prefix.size() > 4) prefix.resize(4);  // no interesan prefijos absurdamente largos
  return prefix;
}

bool write_install_metadata(const AppEntry& app, const std::string& version, const std::string& asset_name,
                            const std::string& sha256) {
  return write_install_metadata_in(app.install_dir(), version, asset_name, sha256);
}

bool write_install_metadata_in(const std::string& dir, const std::string& version,
                               const std::string& asset_name, const std::string& sha256) {
  if (dir.empty()) return false;
  if (!paths::make_dirs(dir) && !paths::is_dir(dir)) return false;

  const std::string version_text = version.empty() ? std::string() : version + "\n";
  const bool version_ok = paths::write_file_atomic(paths::join(dir, "version.txt"), version_text);

  std::string build = asset_name + "\n" + sha256 + "\n";
  const bool build_ok = paths::write_file_atomic(paths::join(dir, "installed-build.txt"), build);

  return version_ok && build_ok;
}

bool uninstall_app(const AppEntry& app, bool force_user_files, std::string* error) {
  const std::string dir = app.install_dir();
  if (dir.empty()) {
    if (error != nullptr) *error = "La entrada no tiene carpeta de instalacion";
    return false;
  }
  if (!paths::exists(dir)) {
    if (error != nullptr) error->clear();  // ya no esta: no es un error
    return true;
  }
  if (!paths::is_dir(dir)) {
    if (error != nullptr) *error = dir + " no es un directorio";
    return false;
  }

  if (force_user_files) {
    if (!paths::remove_dir_recursive(dir)) {
      if (error != nullptr) *error = "No se pudo borrar " + dir;
      return false;
    }
    return true;
  }

  // 1) Si hay manifiesto, se borra exactamente lo que instalo Delanx: ni un fichero de mas.
  const std::vector<std::string> manifest = read_manifest(app);
  if (!manifest.empty()) {
    size_t removed = 0;
    for (const std::string& path : manifest) {
      // Un fichero con extension de datos de juego jamas se borra, aunque este en el manifiesto.
      if (is_user_file(paths::basename(path))) continue;
      if (paths::remove_file(path)) ++removed;
    }
    prune_empty_parents(dir, manifest);
    paths::remove_file(app.version_file());
    paths::remove_file(app.installed_build_file());
    paths::remove_file(manifest_path(app));
    if (!is_shared_target(dir) && dir_is_empty(dir)) paths::remove_dir_recursive(dir);
    if (error != nullptr) {
      *error = "Se eliminaron " + std::to_string(static_cast<long long>(removed)) +
               " ficheros de la app";
    }
    return true;
  }

  // 2) Sin manifiesto (instalacion manual o anterior a esta version): nada de borrados
  //    recursivos en carpetas compartidas, que se llevarian por delante otras apps.
  if (is_shared_target(dir)) {
    if (error != nullptr) {
      *error = "No se borra " + dir + ": es una carpeta compartida con otras apps. Elimina a mano " +
               "solo los ficheros de esta app.";
    }
    return true;
  }

  std::vector<std::string> preserved;
  for (const std::string& name : paths::list_dir(dir)) {
    const std::string full = paths::join(dir, name);
    if (paths::is_dir(full)) continue;
    if (is_user_file(name)) {
      preserved.push_back(full);
    } else {
      paths::remove_file(full);
    }
  }

  if (!preserved.empty()) {
    if (error != nullptr) {
      *error = "Se conservaron " + std::to_string(static_cast<long long>(preserved.size())) +
               " ficheros aportados por el usuario en " + dir;
    }
    return true;
  }

  if (!paths::remove_dir_recursive(dir)) {
    if (error != nullptr) *error = "No se pudo borrar " + dir;
    return false;
  }
  return true;
}

InstallReport install_release(const InstallRequest& request,
                             const std::function<bool(int64_t, int64_t)>& progress) {
  InstallReport report;
  report.version = request.release.tag;
  report.bytes = 0;

  const std::string target_raw =
      request.app.sw.effective_target(paths::apps_dir(), request.app.folder_name);
  if (target_raw.empty()) {
    report.status = InstallStatus::Unknown;
    report.message = "La entrada no define una carpeta de destino";
    return report;
  }
  const std::string target_dir = paths::to_sd_path(target_raw);
  report.target_dir = target_dir;

  if (request.asset.url.empty()) {
    report.status = InstallStatus::MissingAsset;
    report.message = "El release " + show_name(request.release.tag) + " no publica ningun asset descargable";
    return report;
  }

  // Que ficheros ha aportado ya el usuario en el destino: no se tocaran. A la lista por
  // extension se suman los que declara el catalogo (requiredUserFiles), que es el criterio
  // fiable, y los que Delanx registro como suyos en la instalacion anterior.
  std::vector<std::string> user_files = list_user_files(target_dir);
  for (const std::string& declared : declared_user_paths(request.app)) {
    if (std::find(user_files.begin(), user_files.end(), declared) == user_files.end()) {
      user_files.push_back(declared);
    }
  }
  const std::vector<std::string> previous_manifest = read_manifest(request.app);

  // 1) Espacio libre con margen.
  const uint64_t declared = request.asset.size > 0 ? static_cast<uint64_t>(request.asset.size) : 0;
  if (declared > 0 && !sd_has_space(space_needed(declared))) {
    report.status = InstallStatus::NotEnoughSpace;
    report.message = "No hay espacio en la SD: se necesitan unos " + str::human_size(space_needed(declared)) +
                     " y quedan " + str::human_size(free_space_bytes()) +
                     (user_files.empty() ? "" : " (se conservaran los ficheros del usuario)");
    return report;
  }

  // 2) Descarga al staging temporal.
  std::string asset_name = paths::basename(request.asset.url);
  const size_t query = asset_name.find_first_of("?#");
  if (query != std::string::npos) asset_name = asset_name.substr(0, query);
  if (asset_name.empty()) asset_name = paths::basename(request.asset.name);
  if (asset_name.empty()) {
    report.status = InstallStatus::MissingAsset;
    report.message = "El asset no tiene nombre de fichero utilizable";
    return report;
  }

  const std::string staged = create_staging_dir(asset_name);
  paths::remove_file(staged);  // por si quedo una descarga anterior a medias

  std::string download_error;
  bool cancelled = false;
  const bool downloaded = net::download(
      request.asset.url, staged,
      [&progress, &cancelled](int64_t done, int64_t total) -> bool {
        if (!progress) return true;
        const bool keep_going = progress(done, total);
        if (!keep_going) cancelled = true;
        return keep_going;
      },
      &download_error, {});

  if (!downloaded) {
    paths::remove_file(staged);
    report.status = cancelled ? InstallStatus::Cancelled : InstallStatus::DownloadFailed;
    report.message = cancelled ? std::string("Instalacion cancelada por el usuario")
                               : (download_error.empty() ? std::string("No se pudo descargar ") + asset_name
                                                         : download_error);
    return report;
  }
  report.bytes = paths::file_size(staged);
  if (declared == 0) {
    // Tamano desconocido de antemano: se comprueba el espacio ahora que lo sabemos.
    if (!sd_has_space(space_needed(report.bytes))) {
      paths::remove_file(staged);
      report.status = InstallStatus::NotEnoughSpace;
      report.message = "No hay espacio en la SD para extraer " + str::human_size(report.bytes);
      return report;
    }
  }

  // 3) Verificacion SHA-256 (nunca se escribe nada si no cuadra).
  const std::string actual = sha256_file(staged, progress);
  if (actual.empty()) {
    paths::remove_file(staged);
    report.status = InstallStatus::DownloadFailed;
    report.message = "No se pudo leer " + asset_name + " para verificar el checksum";
    return report;
  }
  report.sha256 = actual;

  if (!request.expected_sha256.empty()) {
    if (str::lower(str::trim(request.expected_sha256)) != actual) {
      paths::remove_file(staged);
      report.status = InstallStatus::HashMismatch;
      report.message = "El SHA-256 de " + asset_name + " no coincide con el esperado. Descarga descartada.";
      return report;
    }
  }

  // 4) Extraccion (o copia directa si el asset ya es un .nro).
  const bool single_nro = paths::extension(asset_name) == ".nro" || request.asset.is_single_nro();
  if (!single_nro && !looks_like_archive(asset_name) && !request.asset.is_archive()) {
    paths::remove_file(staged);
    report.status = InstallStatus::UnsupportedAsset;
    report.message = "El formato de " + asset_name + " no es un archivo comprimido reconocido (ni un .nro)";
    return report;
  }

  // Adaptacion de layout para entradas NO verificadas. El catalogo maestro solo dice donde
  // VIVE la app, no donde hay que extraer: muchos paquetes ya traen "switch/<app>/...". Si
  // se extrajeran en su carpeta final, los ficheros acabarian en
  // /switch/<app>/switch/<app>/... y el .nro no se encontraria. Las entradas verificadas
  // tienen targetDir comprobado contra el contenido real del asset, asi que no se tocan.
  std::string extract_dir = target_dir;
  std::string search_dir = target_dir;
  std::vector<std::string> layout_prefix;
  bool layout_adapted = false;
  if (!request.app.verified_entry && !single_nro) {
    layout_prefix = archive_root_components(staged);
    if (!layout_prefix.empty() && str::iequals(layout_prefix[0], "switch")) {
      extract_dir = paths::to_sd_path("/");
      search_dir = paths::join(extract_dir, str::join(layout_prefix, "/"));
      layout_adapted = true;
      platform::log("installer: el paquete trae el layout de la SD (%s); se extrae en la raiz",
                    str::join(layout_prefix, "/").c_str());
    }
  }

  std::string extract_error;
  std::vector<std::string> emptied_dirs;
  if (single_nro) {
    if (!paths::make_dirs(target_dir) && !paths::is_dir(target_dir)) {
      paths::remove_file(staged);
      report.status = InstallStatus::ExtractFailed;
      report.message = "No se pudo crear la carpeta " + target_dir;
      return report;
    }
    // Si la entrada declara un nombre de ejecutable distinto del nombre del asset (p. ej.
    // DevilutionX exige "devilutionx.nro" aunque el asset se llame "devilutionx-switch.nro"),
    // manda el catalogo: es el nombre con el que la app y su documentacion esperan el fichero.
    const std::string final_name =
        request.app.sw.nro.empty() ? asset_name : paths::basename(request.app.sw.nro);
    const std::string final_path = paths::join(target_dir, final_name);
    if (!paths::copy_file(staged, final_path)) {
      paths::remove_file(staged);
      report.status = InstallStatus::ExtractFailed;
      report.message = "No se pudo escribir " + paths::basename(final_path) + " en " + target_dir;
      return report;
    }
    write_manifest(request.app, {final_path});
  } else {
    std::vector<std::string> written;
    if (is_shared_target(extract_dir)) {
      // Destino compartido (raiz de la SD o /switch): jamas se vacia. Solo se retira lo que
      // Delanx instalo la ultima vez, segun su manifiesto.
      for (const std::string& path : previous_manifest) {
        if (!is_user_file(paths::basename(path))) paths::remove_file(path);
      }
    } else {
      // Reinstalacion normal: se vacia la carpeta de la app menos los ficheros del usuario.
      purge_dir_before_extract(extract_dir, user_files, &emptied_dirs);
    }
    if (!extract_archive(staged, extract_dir, request.app.sw.strip_root, &extract_error, user_files,
                         &written)) {
      paths::remove_file(staged);
      report.status = InstallStatus::ExtractFailed;
      report.message = "No se pudo extraer " + asset_name + " en " + extract_dir +
                       (extract_error.empty() ? std::string() : ": " + extract_error);
      return report;
    }
    write_manifest(request.app, written);
  }

  // 5) Localizar el .nro lanzable. Si el layout se adapto, el ejecutable esta en la carpeta
  // que describe el propio paquete (search_dir), no en la carpeta declarada por el catalogo.
  const std::string preferred =
      !request.app.sw.nro.empty() ? request.app.sw.nro : request.app.sw.nro_path;
  report.nro_path = find_launchable_nro(search_dir, preferred);
  if (report.nro_path.empty() && search_dir != target_dir) {
    // Por si el paquete mezcla layouts: se reintenta en la carpeta declarada por el catalogo.
    report.nro_path = find_launchable_nro(target_dir, preferred);
  }

  // 6) Metadatos estilo Quiver. Si el layout se adapto, van junto al ejecutable real.
  const std::string metadata_dir =
      (layout_adapted && !search_dir.empty() && paths::is_dir(search_dir)) ? search_dir : target_dir;
  const bool metadata_ok =
      write_install_metadata_in(metadata_dir, request.release.tag, asset_name, actual);
  report.target_dir = metadata_dir;

  // 7) Limpieza del temporal (salvo keep_archive).
  if (request.keep_archive) {
    paths::make_dirs(paths::cache_dir());
    paths::copy_file(staged, paths::join(paths::cache_dir(), asset_name));
  }
  paths::remove_file(staged);

  // Podar carpetas que la version anterior dejo y que han quedado vacias.
  for (auto it = emptied_dirs.rbegin(); it != emptied_dirs.rend(); ++it) {
    if (dir_is_empty(*it)) paths::remove_dir_recursive(*it);
  }

  std::string message = "Instalado " + request.app.display_name() + " " + show_name(request.release.tag);
  if (!report.nro_path.empty()) {
    message += " en " + report.target_dir;
  } else {
    message += ". No se encontro ningun .nro que lanzar en " + report.target_dir;
  }
  if (layout_adapted) {
    message += ". El paquete ya traia el layout de la SD (switch/...): se ha extraido respetando esa "
               "estructura";
  }
  if (request.expected_sha256.empty()) {
    message += " (sin checksum publicado: contenido no verificado)";
  } else {
    message += " (SHA-256 verificado)";
  }
  if (!metadata_ok) message += ". No se pudieron escribir los metadatos";
  report.message = message;

  report.status = InstallStatus::Ok;
  return report;
}

}  // namespace delanx

// Delanx — instalacion de releases en la SD: descarga, verificacion SHA-256,
// extraccion con libarchive y metadatos compatibles con Quiver Launcher.
#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include "model/app_entry.hpp"
#include "model/catalog.hpp"

namespace delanx {

enum class InstallStatus {
  Ok,
  Cancelled,
  DownloadFailed,
  HashMismatch,
  ExtractFailed,
  MissingAsset,
  NotEnoughSpace,
  UnsupportedAsset,
  Unknown,
};

// Texto corto y en espanol del estado, listo para la interfaz.
const char* install_status_text(InstallStatus status);

struct InstallRequest {
  AppEntry app;
  Release release;
  ReleaseAsset asset;
  std::string expected_sha256;    // hex sin prefijo; vacio = sin verificacion posible
  // Solo se rechaza la instalacion cuando HAY checksum esperado y no coincide. Si el
  // catalogo no publica ninguno, se instala igual y el mensaje lo advierte. Este flag
  // queda como marca explicita del consentimiento del usuario (ya no bloquea nada).
  bool allow_unverified = false;
  bool keep_archive = false;      // conservar el archivo descargado en Cache/
};

struct InstallReport {
  InstallStatus status = InstallStatus::Unknown;
  std::string message;     // texto listo para mostrar al usuario (espanol)
  std::string version;     // tag instalado
  std::string sha256;      // hash real del asset instalado
  std::string target_dir;  // donde quedo instalado
  std::string nro_path;    // ruta al .nro lanzable (vacia si no se encontro)
  uint64_t bytes = 0;
};

// Descarga + verifica + extrae + escribe metadatos.
// progress devuelve false para cancelar; se llama con (bytes_descargados, total).
InstallReport install_release(const InstallRequest& request,
                             const std::function<bool(int64_t, int64_t)>& progress);

// Extrae un archivo local a dest_dir con libarchive (zip/tar/7z/zstd...).
// Detecta y elimina una carpeta raiz unica si strip_root.
// `protected_paths` son ficheros que aporta el usuario y no deben sobrescribirse; cualquier
// entrada que caiga dentro de una de sus carpetas tambien se omite.
// `written` (opcional) recibe la lista de ficheros escritos: permite desinstalar despues de
// forma precisa, sin borrar nada que no haya puesto Delanx.
bool extract_archive(const std::string& archive_path, const std::string& dest_dir, bool strip_root,
                     std::string* error, const std::vector<std::string>& protected_paths = {},
                     std::vector<std::string>* written = nullptr);

// Escribe version.txt e installed-build.txt (formato Quiver: "{filename}\n{sha256}\n").
bool write_install_metadata(const AppEntry& app, const std::string& version, const std::string& asset_name,
                            const std::string& sha256);

// Borra la carpeta de la app. Nunca borra ficheros aportados por el usuario salvo force_user_files.
bool uninstall_app(const AppEntry& app, bool force_user_files, std::string* error);

// Espacio libre en la SD en bytes (statvfs sobre "sdmc:/").
uint64_t free_space_bytes();

// Busca el .nro a lanzar: primero preferred_nro dentro del target, si no el primer .nro
// del directorio y despues en un solo nivel de subcarpeta (Sphaira solo escanea un nivel).
std::string find_launchable_nro(const std::string& target_dir, const std::string& preferred_nro);

}  // namespace delanx

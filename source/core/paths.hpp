// Delanx — rutas y ficheros sobre la SD (newlib POSIX; sin std::filesystem).
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace delanx::paths {

// Raiz de datos de Delanx en la SD.
std::string root();          // sdmc:/switch/Delanx
std::string library_file();  // sdmc:/switch/Delanx/apps.json
std::string settings_file(); // sdmc:/switch/Delanx/settings.json
std::string apps_dir();      // sdmc:/switch/Delanx/Apps
std::string cache_dir();     // sdmc:/switch/Delanx/Cache
std::string temp_dir();      // sdmc:/switch/Delanx/Temp
std::string artwork_dir();   // sdmc:/switch/Delanx/Cache/artwork
std::string log_file();      // sdmc:/switch/Delanx/delanx.log

// Crea la estructura de carpetas necesaria. Devuelve false si alguna falla.
bool ensure_layout();

std::string join(const std::string& a, const std::string& b);
std::string basename(const std::string& path);
std::string dirname(const std::string& path);
std::string extension(const std::string& path);  // en minusculas, con punto (".nro")

// Acepta "/switch/soh", "sdmc:/switch/soh" o "switch/soh" y devuelve "sdmc:/switch/soh".
std::string to_sd_path(const std::string& path);

bool exists(const std::string& path);
bool is_dir(const std::string& path);
bool make_dirs(const std::string& path);

std::optional<std::string> read_file(const std::string& path);
bool write_file(const std::string& path, const std::string& data);
// Escribe en .tmp y renombra: nunca deja el fichero real a medias.
bool write_file_atomic(const std::string& path, const std::string& data);
uint64_t file_size(const std::string& path);
bool remove_file(const std::string& path);
bool remove_dir_recursive(const std::string& path);
bool copy_file(const std::string& from, const std::string& to);

std::vector<std::string> list_dir(const std::string& path);  // nombres, sin "." ni ".."

// Solo para las pruebas en el PC: sustituye la raiz de datos (por defecto "sdmc:/switch/Delanx")
// por una carpeta local. En la consola nunca se llama.
void set_root_override(const std::string& root);
// Solo para las pruebas en el PC: sustituye el prefijo "sdmc:" por una carpeta local, para
// poder instalar en un directorio de trabajo en vez de en una SD real.
void set_sd_prefix_override(const std::string& prefix);

}  // namespace delanx::paths

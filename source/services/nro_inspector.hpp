// Delanx — inspeccion de ficheros NRO, extraccion de NACP/ASET, y separacion Homebrew/Decomps.
#pragma once

#include <string>
#include <vector>

namespace delanx {

struct HomebrewAppInfo {
  std::string name;          // Nombre para mostrar (desde NACP o nombre de carpeta)
  std::string author;        // Autor extraido de NACP
  std::string version;       // Version extraida de NACP (p. ej. "1.2.3")
  std::string nro_path;      // Ruta completa al .nro (sdmc:/switch/...)
  std::string folder_path;   // Carpeta contenedora (sdmc:/switch/...)
  std::string icon_path;     // Ruta a la caratula extraida o icono local
  std::string github_repo;   // Repositorio de GitHub si es un homebrew conocido
  bool is_decomp = false;    // Indica si el NRO corresponde a un port/decompilado
};

// Determina si una aplicacion es una utilidad general de Homebrew
// (p. ej. hb-appstore, Goldleaf, Moonlight, DBI, etc.) y no un Decompilado/Port.
bool is_homebrew_app(const std::string& name_or_folder, const std::string& repo = "");

// Determina si una carpeta, ejecutable o nombre corresponde a un decompilado/port
// (para mantenerlo estrictamente separado del catalogo de homebrew general).
bool is_decomp_app(const std::string& name_or_folder, const std::string& nro_file = "");

// Inspecciona un fichero .nro extrayendo cabeceras NRO0/ASET, NACP e icono incrustado.
HomebrewAppInfo inspect_nro(const std::string& nro_path, const std::string& folder_name = "");

// Escanea el directorio /switch en busca de aplicaciones Homebrew puras
// (excluyendo cualquier decompilado/port reconocido).
std::vector<HomebrewAppInfo> scan_switch_homebrew();

}  // namespace delanx

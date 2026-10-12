// Delanx — detector difuso y organizador de carpetas de decomps en la tarjeta SD.
#pragma once

#include <string>
#include <vector>

#include "model/app_entry.hpp"
#include "model/catalog.hpp"

namespace delanx {

struct DecompMatch {
  std::string folder_path;       // Ruta real detectada (p. ej. sdmc:/switch/MacReady-Golf-Switch)
  std::string nro_path;          // Ruta al ejecutable .nro encontrado dentro
  std::string nro_name;          // Nombre del .nro (p. ej. soh.nro)
  std::string canonical_folder;  // Carpeta estandar/canonica sugerida (p. ej. soh)
  std::string detected_version;  // Version leida si esta disponible
  bool is_custom_name = false;   // Indica si el nombre de carpeta no es el estandar
};

// Escanea /switch de forma difusa (nombre de carpeta + ficheros .nro + ficheros .otr/.o2r/assets)
// y auto-adopta o actualiza las rutas instaladas de los decomps en la biblioteca.
// Devuelve el numero de decomps adoptados o actualizados.
int scan_and_adopt_decomps(Library& library, const std::vector<Catalog>& catalogs);

// Comprueba si una app instalada reside en una carpeta con nombre no estandar
// (p. ej. "MacReady-Golf-Switch" frente a "soh" o "2Ship-Keiichi-Alfa-NX" frente a "2ship").
bool has_non_standard_folder(const AppEntry& app, std::string* out_suggested_folder = nullptr);

// Renombra y normaliza la carpeta del decomp a la ruta estandar deseada previa confirmacion.
bool organize_decomp_folder(AppEntry& app, const std::string& new_folder_name, std::string* error);

}  // namespace delanx

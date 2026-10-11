// Delanx — descarga y cache de caratulas (Cache/artwork/<folderName>.png).
//
// La interfaz no bloquea: las caratulas se encolan y se descargan de una en una entre
// fotogramas. Si una descarga falla, no se reintenta en la misma sesion.
#pragma once

#include <string>

#include "model/app_entry.hpp"

namespace delanx {

// Encola la caratula de una app si no esta ya en cache.
void queue_artwork(const AppEntry& app);
// Descarga como mucho `budget` caratulas pendientes. Devuelve cuantas bajo.
int process_artwork_queue(int budget);
int artwork_queue_size();
void clear_artwork_queue();

// Descarga sincrona (la usa la ficha de detalle al abrirse).
bool ensure_artwork(const AppEntry& app, std::string* error);

}  // namespace delanx

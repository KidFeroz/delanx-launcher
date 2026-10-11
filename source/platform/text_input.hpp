// Delanx — teclado en pantalla de Horizon (swkbd) para buscar y editar ajustes.
#pragma once

#include <string>

namespace delanx::platform {

// Abre el teclado del sistema. Devuelve false si el usuario cancela.
bool text_input(const std::string& header, const std::string& initial, std::string* out,
                size_t max_length = 128, bool password = false);

// Aviso modal simple del sistema (util para errores graves antes de tener UI propia).
void system_message(const std::string& text);

}  // namespace delanx::platform

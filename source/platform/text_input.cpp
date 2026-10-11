// Delanx — implementacion concreta sobre swkbd / applet de libnx.
#include "platform/text_input.hpp"

#include <switch.h>

#include <cstdio>
#include <cstring>
#include <vector>

namespace delanx::platform {

bool text_input(const std::string& header, const std::string& initial, std::string* out, size_t max_length,
                bool password) {
  if (!out) return false;
  if (max_length == 0) max_length = 1;
  if (max_length > 1024) max_length = 1024;

  SwkbdConfig config;
  if (R_FAILED(swkbdCreate(&config, 0))) return false;

  swkbdConfigMakePresetDefault(&config);
  swkbdConfigSetHeaderText(&config, header.c_str());
  swkbdConfigSetGuideText(&config, "Delanx");
  swkbdConfigSetInitialText(&config, initial.c_str());
  swkbdConfigSetStringLenMax(&config, static_cast<u32>(max_length));
  if (password) {
    swkbdConfigSetPasswordFlag(&config, true);
  }

  std::vector<char> buffer(max_length + 1, '\0');
  const Result rc = swkbdShow(&config, buffer.data(), buffer.size());
  swkbdClose(&config);
  if (R_FAILED(rc)) return false;

  *out = std::string(buffer.data());
  return true;
}

void system_message(const std::string& text) {
  // Aviso nativo: se usa solo cuando aun no hay ventana SDL disponible.
  // Nota: libnx no expone un API publica de notificaciones de applet en esta version,
  // asi que se deja constancia en stderr y en el log de Delanx.
  std::fprintf(stderr, "Delanx: %s\n", text.c_str());
}

}  // namespace delanx::platform

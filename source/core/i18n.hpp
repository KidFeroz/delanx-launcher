// Delanx — internacionalizacion (i18n): espanol e ingles.
#pragma once

#include <string>

namespace delanx::i18n {

enum class Language {
  Spanish,
  English,
};

// Configura el idioma actual ("es", "en" o por enum).
void set_language(Language lang);
void set_language_code(const std::string& code);
Language current_language();
std::string current_language_code();

// Obtiene la traduccion para la clave indicada. Si no existe, devuelve la clave.
const char* tr(const char* key);

// Atajo global para facilitar su uso en la interfaz.
inline const char* t(const char* key) { return tr(key); }

}  // namespace delanx::i18n

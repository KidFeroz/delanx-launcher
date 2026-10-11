// Delanx — utilidades de cadena y coincidencia de nombres de asset.
// Sin excepciones ni RTTI (el toolchain de Switch se compila con -fno-exceptions -fno-rtti).
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace delanx::str {

std::string trim(const std::string& s);
std::string lower(const std::string& s);
std::string upper(const std::string& s);

bool iequals(const std::string& a, const std::string& b);
bool icontains(const std::string& haystack, const std::string& needle);
bool starts_with(const std::string& s, const std::string& prefix);
bool istarts_with(const std::string& s, const std::string& prefix);  // sin distinguir mayusculas
bool ends_with(const std::string& s, const std::string& suffix);
bool iends_with(const std::string& s, const std::string& suffix);

// Coincidencia de patron estilo shell, sin distinguir mayusculas. Soporta '*' y '?'.
bool glob_match(const std::string& pattern, const std::string& text);

// Regla de Quiver (RepositorySourceHelper.NormalizeReleaseAssetFilter + AssetNameMatchesFilter):
// si el filtro contiene '*' o '?' se trata como patron completo; si no, como subcadena.
bool asset_matches_filter(const std::string& filter, const std::string& asset_name);

std::vector<std::string> split(const std::string& s, char sep);
std::string join(const std::vector<std::string>& parts, const std::string& sep);
std::string replace_all(std::string s, const std::string& from, const std::string& to);
std::string format(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

// Porcentaje-codifica todo lo que no sea unreserved, para rutas de consulta HTTP.
std::string url_encode(const std::string& s);

// "1,4 MB" a partir de bytes.
std::string human_size(uint64_t bytes);

// Parte editable de una lista de tags, normalizada en minusculas y sin duplicados.
std::vector<std::string> normalize_tags(const std::vector<std::string>& tags);

// Divide texto largo en lineas de como mucho max_chars caracteres, respetando palabras.
std::vector<std::string> wrap_text(const std::string& text, size_t max_chars);

// Formatos de fecha relativos para la UI ("hace 3 d", "12 mar 2026").
std::string relative_date(int64_t unix_seconds);

}  // namespace delanx::str

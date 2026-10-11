// Delanx — tiempo y fechas sin dependencias externas.
// now_unix() no depende de la zona horaria del sistema; solo las funciones
// *_local consultan la configuracion regional de Horizon.
#pragma once

#include <cstdint>
#include <string>

namespace delanx {

// Segundos desde la epoca UTC (time() de newlib).
int64_t now_unix();

// "2026-03-12T21:47:03Z". Devuelve "" si la fecha es invalida.
std::string iso8601_utc(int64_t unix_seconds);

// Acepta "2026-03-12T21:47:03Z", "2026-03-12T21:47:03+00:00", "2026-03-12 21:47:03"
// y solo fecha "2026-03-12". Devuelve 0 si el texto esta vacio o es invalido.
int64_t parse_iso8601_utc(const std::string& text);

// Hora local en formato "21:47" (barra superior de la interfaz).
std::string clock_hhmm_local();

// Fecha local corta en espanol: "12 mar 2026".
std::string date_short_local(int64_t unix_seconds);

}  // namespace delanx

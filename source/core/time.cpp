// Delanx — implementacion de tiempo/fechas sobre newlib.
// Nota de plataforma: newlib para aarch64-none-elf NO exporta timegm() (verificado
// con el enlazado real), asi que la conversion civil -> epoch se hace a mano.
#include "core/time.hpp"

#include <cstdio>
#include <cstring>
#include <ctime>

namespace delanx {
namespace {

const char* const kMonthsEs[12] = {"ene", "feb", "mar", "abr", "may", "jun",
                                   "jul", "ago", "sep", "oct", "nov", "dic"};

bool is_leap(int year) { return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0); }

int days_in_month(int year, int month) {
  static const int kDays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (month == 2 && is_leap(year)) return 29;
  if (month < 1 || month > 12) return 31;
  return kDays[month - 1];
}

// Divisor que redondea hacia menos infinito (el cociente de C++ trunca hacia cero).
int64_t floor_div(int64_t a, int64_t b) {
  const int64_t q = a / b;
  return ((a % b != 0) && ((a < 0) != (b < 0))) ? q - 1 : q;
}

// Dias desde 1970-01-01 para una fecha del calendario proleptico gregoriano
// (algoritmo days-from-civil de Howard Hinnant).
int64_t days_from_civil(int64_t year, unsigned month, unsigned day) {
  year -= (month <= 2) ? 1 : 0;
  const int64_t era = floor_div(year, 400);
  const unsigned yoe = static_cast<unsigned>(year - era * 400);              // [0, 399]
  const unsigned doy = (153u * (month > 2 ? month - 3 : month + 9) + 2u) / 5u + day - 1u;  // [0, 365]
  const unsigned doe = yoe * 365u + yoe / 4u - yoe / 100u + doy;             // [0, 146096]
  return era * 146097 + static_cast<int64_t>(doe) - 719468;
}

// Rellena un struct tm en UTC a partir de segundos epoch.
struct tm utc_tm(int64_t unix_seconds) {
  struct tm tmv;
  memset(&tmv, 0, sizeof(tmv));
  const time_t t = static_cast<time_t>(unix_seconds);
  if (gmtime_r(&t, &tmv) == nullptr) memset(&tmv, 0, sizeof(tmv));
  return tmv;
}

// Escribe un numero de 0 a 99 con dos digitos. Se construye a mano en vez de con
// snprintf porque el rango de los campos de struct tm no esta acotado para el
// analizador de formatos de GCC (-Wformat-truncation).
void append_two_digits(std::string& out, int value) {
  if (value < 0) value = 0;
  if (value > 99) value = 99;
  out.push_back(static_cast<char>('0' + (value / 10) % 10));
  out.push_back(static_cast<char>('0' + value % 10));
}

void append_four_digits(std::string& out, int value) {
  if (value < 0) value = 0;
  if (value > 9999) value = 9999;
  out.push_back(static_cast<char>('0' + (value / 1000) % 10));
  out.push_back(static_cast<char>('0' + (value / 100) % 10));
  out.push_back(static_cast<char>('0' + (value / 10) % 10));
  out.push_back(static_cast<char>('0' + value % 10));
}

}  // namespace

int64_t now_unix() { return static_cast<int64_t>(time(nullptr)); }

std::string iso8601_utc(int64_t unix_seconds) {
  const struct tm tmv = utc_tm(unix_seconds);
  std::string out;
  out.reserve(21);
  append_four_digits(out, tmv.tm_year + 1900);
  out.push_back('-');
  append_two_digits(out, tmv.tm_mon + 1);
  out.push_back('-');
  append_two_digits(out, tmv.tm_mday);
  out.push_back('T');
  append_two_digits(out, tmv.tm_hour);
  out.push_back(':');
  append_two_digits(out, tmv.tm_min);
  out.push_back(':');
  append_two_digits(out, tmv.tm_sec);
  out.push_back('Z');
  return out;
}

int64_t parse_iso8601_utc(const std::string& text) {
  if (text.size() < 10) return 0;

  int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;
  if (sscanf(text.c_str(), "%4d-%2d-%2d", &year, &month, &day) != 3) return 0;
  if (year < 1970 || month < 1 || month > 12 || day < 1 || day > days_in_month(year, month)) return 0;

  // Parte de hora opcional: "T" o espacio.
  if (text.size() >= 19 && (text[10] == 'T' || text[10] == 't' || text[10] == ' ')) {
    if (sscanf(text.c_str() + 11, "%2d:%2d:%2d", &hour, &minute, &second) != 3) return 0;
    if (hour > 23 || minute > 59 || second > 60) return 0;  // 60 por segundos bisiestos
  }

  // Desplazamiento horario: "Z" (o nada) = UTC; "+hh:mm" / "-hh:mm" se compensa.
  int64_t offset = 0;
  const size_t zone_at = (text.size() >= 19) ? 19 : 10;
  if (text.size() > zone_at) {
    const char sign = text[zone_at];
    if (sign == '+' || sign == '-') {
      int oh = 0, om = 0;
      if (sscanf(text.c_str() + zone_at + 1, "%2d:%2d", &oh, &om) >= 1) {
        if (oh > 14 || om > 59) return 0;
        offset = static_cast<int64_t>(oh) * 3600 + static_cast<int64_t>(om) * 60;
        if (sign == '-') offset = -offset;
      }
    } else if (sign != 'Z' && sign != 'z') {
      // Sufijo desconocido (p. ej. " UTC"): se ignora, se trata como UTC.
    }
  }

  // Epoch UTC sin depender de timegm() ni de la zona horaria del sistema.
  const int64_t days = days_from_civil(year, static_cast<unsigned>(month), static_cast<unsigned>(day));
  const int64_t utc = days * 86400 + static_cast<int64_t>(hour) * 3600 + static_cast<int64_t>(minute) * 60 +
                      static_cast<int64_t>(second) - offset;
  if (utc < 0) return 0;
  return utc;
}

std::string clock_hhmm_local() {
  const time_t t = time(nullptr);
  struct tm tmv;
  memset(&tmv, 0, sizeof(tmv));
  if (localtime_r(&t, &tmv) == nullptr) return std::string("--:--");
  char buf[8];
  snprintf(buf, sizeof(buf), "%02d:%02d", tmv.tm_hour, tmv.tm_min);
  return std::string(buf);
}

std::string date_short_local(int64_t unix_seconds) {
  const time_t t = static_cast<time_t>(unix_seconds);
  struct tm tmv;
  memset(&tmv, 0, sizeof(tmv));
  if (localtime_r(&t, &tmv) == nullptr) return std::string();

  const int year = tmv.tm_year + 1900;
  int month = tmv.tm_mon + 1;
  const int day = tmv.tm_mday;
  if (month < 1 || month > 12) month = 1;

  // Montado a mano por el mismo motivo que iso8601_utc (rangos de struct tm).
  std::string out;
  out.reserve(16);
  if (day < 10) out.push_back('0');
  out += std::to_string(day);
  out.push_back(' ');
  out += kMonthsEs[month - 1];
  out.push_back(' ');
  out += std::to_string(year > 0 ? year : 0);
  return out;
}

}  // namespace delanx

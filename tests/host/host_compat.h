// Delanx — compatibilidad para compilar la capa de datos en el PC (Windows/MinGW).
//
// Se inyecta con -include en las pruebas de host para suplir las diferencias con newlib
// (devkitA64), que es el objetivo real. El codigo de produccion no incluye este fichero.
#pragma once

#if defined(_WIN32) && !defined(__SWITCH__)

#include <direct.h>
#include <time.h>

// newlib: int mkdir(const char* path, mode_t mode). MinGW: int _mkdir(const char*).
#define mkdir(path, mode) _mkdir(path)

static inline struct tm* gmtime_r(const time_t* timer, struct tm* out) {
  return (timer != nullptr && out != nullptr && gmtime_s(out, timer) == 0) ? out : nullptr;
}

static inline struct tm* localtime_r(const time_t* timer, struct tm* out) {
  return (timer != nullptr && out != nullptr && localtime_s(out, timer) == 0) ? out : nullptr;
}

// MinGW no trae <sys/statvfs.h>: se declara lo minimo para que installer.cpp compile y
// free_space_bytes() devuelva 0 ("espacio desconocido"), que es un caso que el instalador
// ya contempla. En la consola se usa el statvfs real de newlib.
struct statvfs {
  unsigned long f_bsize;
  unsigned long f_frsize;
  unsigned long long f_blocks;
  unsigned long long f_bfree;
  unsigned long long f_bavail;
};

static inline int statvfs(const char* path, struct statvfs* out) {
  (void)path;
  (void)out;
  return -1;
}

#endif  // _WIN32 && !__SWITCH__

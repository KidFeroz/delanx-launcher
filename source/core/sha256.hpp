// Delanx — SHA-256 propio (sin mbedtls ni dependencias externas).
// Se implementa a mano porque el checksum de los releases es una comprobacion
// critica y no queremos arrastrar una libreria de criptografia solo para esto.
#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

namespace delanx {

// Hash de un bloque de memoria, en hexadecimal minusculas (64 caracteres).
std::string sha256_hex(const void* data, size_t len);
std::string sha256_hex(const std::string& data);

// Hash de un fichero leyendolo por bloques (no carga el fichero entero en RAM).
// Devuelve "" si no se pudo leer. Llama a progress cada ~1 MB con (leido, total);
// si el fichero no se puede medir, total es 0.
std::string sha256_file(const std::string& path,
                        const std::function<bool(int64_t read, int64_t total)>& progress = {});

}  // namespace delanx

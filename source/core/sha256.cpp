// Delanx — implementacion de SHA-256 (FIPS 180-4) escrita a mano.
// Sin excepciones, sin RTTI, sin dependencias: solo aritmetica de enteros de 32 bits.
#include "core/sha256.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace delanx {
namespace {

// Primeros 32 bits de las partes fraccionarias de las raices cubicas de los 64 primeros primos.
constexpr uint32_t kK[64] = {
    0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
    0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
    0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
    0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
    0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
    0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
    0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
    0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u, 0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u,
};

// Estado incremental: 8 palabras de 32 bits + buffer parcial de 64 bytes.
struct Ctx {
  uint32_t h[8];
  uint8_t block[64];
  size_t used = 0;
  uint64_t total = 0;  // bytes ya procesados (sin contar el bloque parcial)
};

constexpr uint32_t rotr(uint32_t x, unsigned n) { return (x >> n) | (x << (32u - n)); }

void transform(Ctx& c, const uint8_t* data) {
  uint32_t w[64];
  for (unsigned i = 0; i < 16; ++i) {
    w[i] = (static_cast<uint32_t>(data[i * 4 + 0]) << 24) | (static_cast<uint32_t>(data[i * 4 + 1]) << 16) |
           (static_cast<uint32_t>(data[i * 4 + 2]) << 8) | static_cast<uint32_t>(data[i * 4 + 3]);
  }
  for (unsigned i = 16; i < 64; ++i) {
    const uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
    const uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
    w[i] = w[i - 16] + s0 + w[i - 7] + s1;
  }

  uint32_t a = c.h[0], b = c.h[1], cc = c.h[2], d = c.h[3];
  uint32_t e = c.h[4], f = c.h[5], g = c.h[6], hh = c.h[7];

  for (unsigned i = 0; i < 64; ++i) {
    const uint32_t s1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
    const uint32_t ch = (e & f) ^ (~e & g);
    const uint32_t t1 = hh + s1 + ch + kK[i] + w[i];
    const uint32_t s0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
    const uint32_t maj = (a & b) ^ (a & cc) ^ (b & cc);
    const uint32_t t2 = s0 + maj;

    hh = g;
    g = f;
    f = e;
    e = d + t1;
    d = cc;
    cc = b;
    b = a;
    a = t1 + t2;
  }

  c.h[0] += a;
  c.h[1] += b;
  c.h[2] += cc;
  c.h[3] += d;
  c.h[4] += e;
  c.h[5] += f;
  c.h[6] += g;
  c.h[7] += hh;
}

void init(Ctx& c) {
  c.h[0] = 0x6a09e667u;
  c.h[1] = 0xbb67ae85u;
  c.h[2] = 0x3c6ef372u;
  c.h[3] = 0xa54ff53au;
  c.h[4] = 0x510e527fu;
  c.h[5] = 0x9b05688cu;
  c.h[6] = 0x1f83d9abu;
  c.h[7] = 0x5be0cd19u;
  c.used = 0;
  c.total = 0;
  memset(c.block, 0, sizeof(c.block));
}

void update(Ctx& c, const uint8_t* data, size_t len) {
  c.total += len;
  while (len > 0) {
    const size_t space = sizeof(c.block) - c.used;
    const size_t take = (len < space) ? len : space;
    memcpy(c.block + c.used, data, take);
    c.used += take;
    data += take;
    len -= take;
    if (c.used == sizeof(c.block)) {
      transform(c, c.block);
      c.used = 0;
    }
  }
}

void final(Ctx& c, uint8_t out[32]) {
  const uint64_t bits = c.total * 8u;

  // Padding: 0x80, ceros, y los 8 bytes de longitud en big-endian.
  uint8_t pad[72];
  memset(pad, 0, sizeof(pad));
  pad[0] = 0x80;
  const size_t padlen = (c.used < 56) ? (56 - c.used) : (120 - c.used);
  update(c, pad, padlen);

  uint8_t lenbytes[8];
  for (unsigned i = 0; i < 8; ++i) lenbytes[i] = static_cast<uint8_t>((bits >> (56u - i * 8u)) & 0xffu);
  update(c, lenbytes, sizeof(lenbytes));

  for (unsigned i = 0; i < 8; ++i) {
    out[i * 4 + 0] = static_cast<uint8_t>((c.h[i] >> 24) & 0xffu);
    out[i * 4 + 1] = static_cast<uint8_t>((c.h[i] >> 16) & 0xffu);
    out[i * 4 + 2] = static_cast<uint8_t>((c.h[i] >> 8) & 0xffu);
    out[i * 4 + 3] = static_cast<uint8_t>(c.h[i] & 0xffu);
  }
}

std::string to_hex(const uint8_t digest[32]) {
  static const char* kHex = "0123456789abcdef";
  std::string out;
  out.resize(64);
  for (unsigned i = 0; i < 32; ++i) {
    out[i * 2 + 0] = kHex[(digest[i] >> 4) & 0x0fu];
    out[i * 2 + 1] = kHex[digest[i] & 0x0fu];
  }
  return out;
}

}  // namespace

std::string sha256_hex(const void* data, size_t len) {
  Ctx c;
  init(c);
  if (data != nullptr && len > 0) update(c, static_cast<const uint8_t*>(data), len);
  uint8_t digest[32];
  final(c, digest);
  return to_hex(digest);
}

std::string sha256_hex(const std::string& data) { return sha256_hex(data.data(), data.size()); }

std::string sha256_file(const std::string& path, const std::function<bool(int64_t, int64_t)>& progress) {
  FILE* f = fopen(path.c_str(), "rb");
  if (f == nullptr) return std::string();

  // Tamano total para el informe de progreso (0 si no se puede determinar).
  int64_t total = 0;
  if (fseek(f, 0, SEEK_END) == 0) {
    const long end = ftell(f);
    if (end > 0) total = static_cast<int64_t>(end);
    rewind(f);
  }

  Ctx c;
  init(c);

  // 1 MB por bloque: coincide con la cadencia de progreso pedida.
  static const size_t kChunk = 1024u * 1024u;
  uint8_t* buf = static_cast<uint8_t*>(malloc(kChunk));
  if (buf == nullptr) {
    fclose(f);
    return std::string();
  }

  int64_t read_total = 0;
  bool ok = true;
  for (;;) {
    const size_t n = fread(buf, 1, kChunk, f);
    if (n > 0) {
      update(c, buf, n);
      read_total += static_cast<int64_t>(n);
    }
    if (n < kChunk) {
      // Fin de fichero o error.
      if (ferror(f) != 0) ok = false;
      break;
    }
    if (progress && !progress(read_total, total)) ok = false;  // cancelado, pero devolvemos lo leido
    if (!ok) break;
  }

  free(buf);
  fclose(f);
  if (!ok) return std::string();

  if (progress) progress(read_total, total);
  if (read_total != total && total != 0) return std::string();  // el fichero cambio mientras lo leiamos

  uint8_t digest[32];
  final(c, digest);
  return to_hex(digest);
}

}  // namespace delanx

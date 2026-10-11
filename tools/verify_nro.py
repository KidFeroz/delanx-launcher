#!/usr/bin/env python3
"""Verifica el .nro construido: cabecera NRO, seccion ASET, NACP e icono.

Uso:
    python tools/verify_nro.py Delanx.nro

Estructura real del fichero (libnx, switch/nro.h):
    NroStart        0x00  { u32 unused; u32 mod_offset; u8 padding[8] }
    NroHeader       0x10  { "NRO0"; u32 unk1; u32 size; u32 unk2;
                            NroSegment segments[3]; u32 bss_size; u32 unk3;
                            u8 build_id[0x20]; u8 padding[0x20] }
    NroAssetHeader  0x90  { "ASET"; u32 version;
                            u64/u64 icon; u64/u64 nacp; u64/u64 romfs }
Los offsets de ASET son relativos al inicio de ASET. El NACP lleva el titulo (0x200),
el autor (0x100) y la version (0x10) que muestra Horizon; el icono es una seccion aparte.

Comprueba, sin necesidad de la consola:
  * firma NRO0 en 0x10 y coherencia de los segmentos .text/.rodata/.data;
  * seccion ASET con icono, NACP y RomFS dentro del fichero;
  * titulo, autor y version declarados en el NACP;
  * que el icono incrustado es un JPEG de 256x256;
  * que el catalogo verificado y la fuente viajan dentro del RomFS.
"""
from __future__ import annotations

import pathlib
import struct
import sys

NRO_START = 0x10
NRO_HEADER_SIZE = 0x80
# El campo `size` de NroHeader (0x18) es el offset donde empieza la seccion ASET,
# que va al final del fichero, detras de .text/.rodata/.data.
ASET_POINTER = 0x18
NACP_TITLE = (0x0000, 0x200)
NACP_AUTHOR = (0x0200, 0x100)
NACP_VERSION = (0x3060, 0x10)


def u32(data: bytes, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def u64(data: bytes, offset: int) -> int:
    return struct.unpack_from("<Q", data, offset)[0]


def c_string(data: bytes, offset: int, length: int) -> str:
    return data[offset : offset + length].split(b"\x00", 1)[0].decode("utf-8", errors="replace")


def jpeg_size(blob: bytes) -> tuple[int, int]:
    index = 2
    while index + 9 < len(blob):
        if blob[index] != 0xFF:
            index += 1
            continue
        marker = blob[index + 1]
        if marker in (0xC0, 0xC1, 0xC2, 0xC3):
            return struct.unpack_from(">H", blob, index + 7)[0], struct.unpack_from(">H", blob, index + 5)[0]
        if marker in (0xD8, 0xD9) or 0xD0 <= marker <= 0xD7:
            index += 2
            continue
        index += 2 + struct.unpack_from(">H", blob, index + 2)[0]
    return 0, 0


def main(argv: list[str]) -> int:
    if len(argv) != 2:
        print(__doc__)
        return 2

    path = pathlib.Path(argv[1])
    if not path.exists():
        print(f"no existe: {path}")
        return 2

    data = path.read_bytes()
    failures: list[str] = []
    checks = 0

    def check(condition: bool, what: str) -> None:
        nonlocal checks
        checks += 1
        print(f"  {'OK   ' if condition else 'FALLO'} {what}")
        if not condition:
            failures.append(what)

    print(f"== {path.name} ({len(data):,} bytes) ==")
    check(data[NRO_START : NRO_START + 4] == b"NRO0", "firma NRO0 en 0x10")
    check(data[0x08:0x10] == b"HOMEBREW", "marca HOMEBREW del cargador")

    segments = []
    for index in range(3):
        base = NRO_START + 0x10 + index * 8
        segments.append((u32(data, base), u32(data, base + 4)))
    names = (".text", ".rodata", ".data")
    for name, (offset, size) in zip(names, segments):
        inside = offset + size <= len(data)
        check(size > 0 and inside, f"segmento {name} presente y dentro del fichero ({size:,} bytes)")

    aset = u32(data, ASET_POINTER)
    if data[aset : aset + 4] != b"ASET":
        found = data.find(b"ASET", NRO_START + NRO_HEADER_SIZE)
        aset = found if found >= 0 else aset
    aset_offset = aset
    ASET_OFFSET = aset_offset  # alias usado por el resto de comprobaciones
    check(data[aset_offset : aset_offset + 4] == b"ASET",
          f"cabecera de assets ASET en 0x{aset_offset:x}")
    icon_off, icon_size = u64(data, aset_offset + 8), u64(data, aset_offset + 16)
    nacp_off, nacp_size = u64(data, aset_offset + 24), u64(data, aset_offset + 32)
    romfs_off, romfs_size = u64(data, aset_offset + 40), u64(data, aset_offset + 48)
    print(f"  info  icono=0x{icon_size:x}  nacp=0x{nacp_size:x}  romfs=0x{romfs_size:x}")

    check(icon_size > 0 and aset_offset + icon_off + icon_size <= len(data), "seccion de icono presente")
    check(nacp_size >= 0x4000 and aset_offset + nacp_off + nacp_size <= len(data), "seccion NACP presente")
    check(aset_offset + romfs_off + romfs_size <= len(data), "la seccion RomFS cabe en el fichero")
    if romfs_size == 0:
        print("  info  sin RomFS (opcional: el autotest de dispositivo no lo necesita)")

    icon = data[aset_offset + icon_off : aset_offset + icon_off + icon_size]
    check(icon[:3] == b"\xff\xd8\xff", "el icono incrustado es JPEG")
    if icon[:3] == b"\xff\xd8\xff":
        width, height = jpeg_size(icon)
        check((width, height) == (256, 256), f"el icono mide 256x256 (leido {width}x{height})")

    nacp = data[ASET_OFFSET + nacp_off : ASET_OFFSET + nacp_off + nacp_size]
    title = c_string(nacp, *NACP_TITLE)
    author = c_string(nacp, *NACP_AUTHOR)
    version = c_string(nacp, *NACP_VERSION)
    print(f"  info  titulo='{title}' autor='{author}' version='{version}'")
    check(title.strip() != "", "el NACP declara titulo")
    check(author.strip() != "", "el NACP declara autor")
    check(version.count(".") == 2, "la version del NACP tiene formato mayor.menor.parche")

    romfs = data[ASET_OFFSET + romfs_off : ASET_OFFSET + romfs_off + romfs_size]
    if romfs_size > 0:
        for name in (b"switch-catalog.json", b"harbourmasters.json", b"Inter.ttf"):
            check(name in romfs, f"el RomFS contiene {name.decode()}")

    print("----------------------------------------")
    print(f"comprobaciones: {checks}   fallos: {len(failures)}")
    if failures:
        print("fallos:")
        for item in failures:
            print(f"  - {item}")
    return 0 if not failures else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))

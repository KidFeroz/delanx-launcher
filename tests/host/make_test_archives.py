#!/usr/bin/env python3
"""Genera archivos de prueba para las pruebas del instalador.

  malicious.zip    zip con entradas que escapan del destino ("../escaped.txt", "/abs.txt")
  single-root.zip  zip con una unica carpeta raiz ("App/...") para probar strip_root

Uso: python tests/host/make_test_archives.py
"""
from __future__ import annotations

import pathlib
import sys
import zipfile


def main() -> int:
    out = pathlib.Path(__file__).resolve().parent.parent / "fixtures" / "assets"
    out.mkdir(parents=True, exist_ok=True)

    malicious = out / "malicious.zip"
    with zipfile.ZipFile(malicious, "w", zipfile.ZIP_DEFLATED) as archive:
        archive.writestr("ok.txt", "este fichero si es legitimo")
        # Debe ser rechazado por extract_archive (path traversal).
        archive.writestr("../escaped.txt", "no deberia escribirse fuera del destino")
        archive.writestr("/abs.txt", "tampoco esto")

    rooted = out / "single-root.zip"
    with zipfile.ZipFile(rooted, "w", zipfile.ZIP_DEFLATED) as archive:
        archive.writestr("App/app.nro", "NRO falso")
        archive.writestr("App/data/file.txt", "datos")

    # PNG minimo y valido (2x2) para la prueba de la cache de caratulas.
    artwork = out / "artwork-test.png"
    artwork.write_bytes(
        bytes.fromhex(
            "89504e470d0a1a0a0000000d4948445200000002000000020806000000f478d4"
            "fa0000001649444154789c636060f80f04030c0c0c0c8c4c0c0c000f2c0101"
            "9a9b1c8d0000000049454e44ae426082"
        )
    )

    print(f"escrito {malicious} ({malicious.stat().st_size} B)")
    print(f"escrito {rooted} ({rooted.stat().st_size} B)")
    print(f"escrito {artwork} ({artwork.stat().st_size} B)")
    return 0


if __name__ == "__main__":
    sys.exit(main())

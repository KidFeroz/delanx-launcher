#!/usr/bin/env python3
"""Descarga los assets de release reales que usan las pruebas del instalador.

Son paquetes de verdad publicados para Switch, elegidos porque cubren los tres disenos de
paquete que se encuentran en el homebrew:

  Starship-Barnard-Alfa-Switch.zip        .nro en la raiz del zip         (stripRoot=false)
  sphaira.zip                             switch/<app>/<app>.nro          (targetDir "/")
  Spaghettify-Alfredo-Alfa-1-Switch.zip   carpeta raiz unica             (stripRoot=true)
  mGBA-0.10.5-switch.7z                   .7z extraido con libarchive    (mGBA)

No se guardan en el repositorio (pesan ~17 MB): se descargan a demanda.

Uso: python tests/host/fetch_fixtures.py
"""
from __future__ import annotations

import hashlib
import pathlib
import sys
import urllib.request

ASSETS = {
    "Starship-Barnard-Alfa-Switch.zip":
        ("https://github.com/HarbourMasters/Starship/releases/download/v2.0.0/"
         "Starship-Barnard-Alfa-Switch.zip",
         "b64238ad35e5401229576baa98dbd8a517228eac996454f9d55d293888ec9fa9"),
    "sphaira.zip":
        ("https://github.com/NaGaa95/sphaira/releases/download/1.0.8/sphaira.zip",
         "3e7ebacd421917d426cc1ab49622ec4a1ea7943fb571d62aea7176089dc1384c"),
    "Spaghettify-Alfredo-Alfa-1-Switch.zip":
        ("https://github.com/HarbourMasters/SpaghettiKart/releases/download/0.9.9.1/"
         "Spaghettify-Alfredo-Alfa-1-Switch.zip",
         "52dbf4b1d7213d29133d98f92ac86ebcf87854ad3ebd072881b671507592b726"),
    # Unico asset del catalogo empaquetado en .7z (lo extrae libarchive en el instalador).
    "mGBA-0.10.5-switch.7z":
        ("https://github.com/mgba-emu/mgba/releases/download/0.10.5/mGBA-0.10.5-switch.7z",
         "ef7a7f5cea6d6639c8a12ebb282d915e444bf844b658ff5a4dd72886c8cfa016"),
}


def main() -> int:
    out = pathlib.Path(__file__).resolve().parent.parent / "fixtures" / "assets"
    out.mkdir(parents=True, exist_ok=True)
    failures = 0

    for name, (url, expected) in ASSETS.items():
        target = out / name
        if target.exists():
            digest = hashlib.sha256(target.read_bytes()).hexdigest()
            if digest == expected:
                print(f"OK (ya estaba)  {name}")
                continue
            print(f"hash distinto, se vuelve a descargar: {name}")
            target.unlink()

        print(f"descargando {name} ...")
        try:
            with urllib.request.urlopen(url, timeout=240) as response:
                data = response.read()
        except Exception as error:  # noqa: BLE001 - se informa y se sigue con el resto
            print(f"  FALLO al descargar: {error}")
            failures += 1
            continue

        digest = hashlib.sha256(data).hexdigest()
        if digest != expected:
            print(f"  FALLO: sha256 {digest} != {expected}")
            failures += 1
            continue

        target.write_bytes(data)
        print(f"  escrito {target.name} ({len(data):,} B)")

    if failures:
        print(f"\n{failures} asset(s) con problemas")
        return 1
    print("\ntodos los assets listos")
    return 0


if __name__ == "__main__":
    sys.exit(main())

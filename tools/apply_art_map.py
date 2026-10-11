#!/usr/bin/env python3
"""Rellena `appIconUrl` en el catálogo verificado a partir de `romfs/catalog/art-map.json`.

Decisiones de diseño (importantes):

* **Nunca sobrescribe.** Solo escribe en entradas cuyo `appIconUrl` esté ausente, sea
  `null` o cadena vacía/solo espacios. Una URL ya presente se respeta siempre: el catálogo
  verificado manda sobre cualquier mapa de arte.
* **Idempotente.** La segunda ejecución informa de 0 cambios y no reescribe el fichero,
  así que no ensucia el diff ni actualiza la fecha de modificación.
* **Mismo formato que el original.** Se conserva la indentación detectada (el catálogo usa
  2 espacios), el orden de claves, `ensure_ascii=False`, el tipo de salto de línea (LF o
  CRLF) y el salto de línea final, y se escribe UTF-8 sin BOM. El único detalle que
  `json.dumps` no reproduce es `tags`, que en el catálogo va en una sola línea
  (`"tags": ["port", "decomp", "n64", "zelda"]`) mientras que todo lo demás (`switchProof`,
  `entries`, `requiredUserFiles`) es `json.dumps` estándar. Sin ese ajuste, cada una de las
  46 entradas tendría un diff espurio de 5 líneas. El resto del fichero no se toca.
* **Emparejamiento exacto por `folderName`** (tal como está escrito en el catálogo). Las
  claves del mapa que no correspondan a ninguna entrada se reportan como aviso en lugar de
  ignorarse en silencio.
* **Sin red.** Este script no comprueba URLs: la verificación HTTP (200 + `Content-Type`
  de imagen) se hizo al construir `art-map.json` y está documentada en
  `docs/art-libretro.md`.

Uso:
    python tools/apply_art_map.py --check     # simula; no escribe nada
    python tools/apply_art_map.py            # escribe switch-catalog.json
"""
from __future__ import annotations

import argparse
import json
import pathlib
import re
import sys

REPO = pathlib.Path(__file__).resolve().parent.parent
DEFAULT_CATALOG = REPO / "romfs" / "catalog" / "switch-catalog.json"
DEFAULT_MAP = REPO / "romfs" / "catalog" / "art-map.json"

# Claves cuyo valor vacío debe considerarse "sin carátula".
FILLED = "filled"
ALREADY = "already"
UNKNOWN = "unknown"


def detect_indent(raw: str) -> int:
    """Deduce la indentación del JSON a partir de la primera clave de nivel 1."""
    match = re.search(r"\n( +)\"", raw)
    return len(match.group(1)) if match else 2


def detect_newline(raw: str) -> str:
    """Deduce el salto de línea dominante del fichero original."""
    crlf = raw.count("\r\n")
    lf = raw.count("\n") - crlf
    return "\r\n" if crlf > lf else "\n"


# Array de `tags` expandido por json.dumps, para devolverlo a una sola línea.
TAGS_ARRAY = re.compile(r'(\n\s*"tags": )\[\s*\n((?:[^\[\]{}]*\n)*?)(\s*)\]')


def compact_tags(match: re.Match[str]) -> str:
    """Devuelve un array de tags expandido a su forma de una sola línea."""
    items = [line.strip().rstrip(",") for line in match.group(2).splitlines() if line.strip()]
    return f"{match.group(1)}[{', '.join(items)}]"


def render(catalog: dict, indent: int) -> str:
    """Serializa el catálogo conservando el estilo del original.

    `json.dumps` expande todos los arrays a varias líneas, pero el catálogo escribe `tags`
    en una sola (`"tags": ["port", "decomp", "n64", "zelda"]`). Se restaura ese detalle; el
    resto del fichero queda exactamente como lo generaría
    `json.dumps(indent=..., ensure_ascii=False)`.
    """
    text = json.dumps(catalog, indent=indent, ensure_ascii=False)
    return TAGS_ARRAY.sub(compact_tags, text)


def is_empty(value: object) -> bool:
    """True si el campo se considera vacío (ausente, null, cadena vacía o de espacios)."""
    if value is None:
        return True
    if isinstance(value, str):
        return value.strip() == ""
    return False


def load_json(path: pathlib.Path) -> object:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError:
        sys.exit(f"error: no existe {path}")
    except json.JSONDecodeError as exc:
        sys.exit(f"error: {path} no es JSON válido: {exc}")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Rellena appIconUrl en el catálogo verificado desde art-map.json "
                    "(solo donde esté vacío; nunca sobrescribe).")
    parser.add_argument("--catalog", type=pathlib.Path, default=DEFAULT_CATALOG,
                        help=f"catálogo de destino (por defecto: {DEFAULT_CATALOG})")
    parser.add_argument("--map", dest="art_map", type=pathlib.Path, default=DEFAULT_MAP,
                        help=f"mapa folderName -> URL (por defecto: {DEFAULT_MAP})")
    parser.add_argument("--check", action="store_true",
                        help="simula la operación y no escribe ningún fichero")
    args = parser.parse_args()

    catalog_path: pathlib.Path = args.catalog
    map_path: pathlib.Path = args.art_map

    raw = catalog_path.read_text(encoding="utf-8")
    catalog = load_json(catalog_path)
    art_map = load_json(map_path)

    if not isinstance(catalog, dict) or not isinstance(catalog.get("entries"), list):
        sys.exit(f"error: {catalog_path} no tiene una lista 'entries'")
    if not isinstance(art_map, dict) or not isinstance(art_map.get("art"), dict):
        sys.exit(f"error: {map_path} no tiene un objeto 'art'")

    art: dict = art_map["art"]
    entries: list = catalog["entries"]
    known_folders = {e.get("folderName") for e in entries if isinstance(e, dict)}

    filled: list[tuple[str, str]] = []
    already: list[tuple[str, str]] = []

    for entry in entries:
        if not isinstance(entry, dict):
            continue
        folder = entry.get("folderName")
        url = art.get(folder)
        if not isinstance(url, str) or not url.strip():
            continue
        if is_empty(entry.get("appIconUrl")):
            entry["appIconUrl"] = url
            filled.append((folder, url))
        else:
            already.append((folder, entry["appIconUrl"]))

    unknown = sorted(k for k in art if k not in known_folders)

    print(f"catálogo      : {catalog_path}")
    print(f"mapa de arte  : {map_path}  ({len(art)} claves)")
    print(f"entradas      : {len(entries)}")
    print(f"rellenadas    : {len(filled)}")
    for folder, url in filled:
        print(f"  + {folder} -> {url}")
    print(f"ya tenían URL : {len(already)}  (no se tocan)")
    for folder, url in already:
        print(f"  = {folder} (se conserva la existente)")
    if unknown:
        print(f"claves del mapa sin entrada en el catálogo: {len(unknown)}")
        for key in unknown:
            print(f"  ? {key}")

    if not filled:
        print("\n0 cambios: nada que escribir (idempotente).")
        return 0

    if args.check:
        print(f"\n--check: {len(filled)} cambios simulados; no se ha escrito nada.")
        return 0

    indent = detect_indent(raw)
    newline = detect_newline(raw)
    text = render(catalog, indent)
    if newline == "\r\n":
        text = text.replace("\n", "\r\n")
    catalog_path.write_text(text + newline, encoding="utf-8", newline="")
    print(f"\nescrito {catalog_path} ({len(filled)} entradas actualizadas, "
          f"indent={indent}, salto={'CRLF' if newline == chr(13) + chr(10) else 'LF'})")
    return 0


if __name__ == "__main__":
    sys.exit(main())

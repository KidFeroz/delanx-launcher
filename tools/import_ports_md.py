#!/usr/bin/env python3
"""Convierte el catálogo maestro (switch_ports_catalog.md) a un catálogo de Delanx.

Decisiones de diseño (importantes):

* **No se mezcla con el catálogo verificado.** Las 46 entradas de
  `romfs/catalog/switch-catalog.json` están comprobadas contra releases reales; las del
  catálogo maestro **no**. Mezclarlas bajaría la confianza del catálogo, así que el
  resultado va a un fichero aparte (`romfs/catalog/ports-master.json`) con
  `verified: false` y una nota en cada entrada.
* **Las entradas ya verificadas no se duplican**: el maestro aporta solo lo que Delanx
  todavía no tiene. La salida tiene `{"schemaVersion": 1, "source": ..., "entries": [...]}`,
  el mismo sobre que lee `delanx::Catalog::load_file` (formato de `switch-catalog.json`).
* **`install_dir` del maestro != raíz de extracción de Delanx.** El maestro indica dónde
  *vive* la app (`sdmc:/switch/Ghostship/`), pero muchos zips ya traen `switch/<app>/...`
  dentro, así que la raíz de extracción es `/`. Por eso el importador guarda la carpeta
  final en `switch.homeDir` (campo nuevo del bloque `switch`) y deja `switch.targetDir`
  como raíz de extracción: como el contenido del asset no está verificado, se usa la
  carpeta final (correcto cuando el zip NO trae el prefijo `switch/`) y se anota en `notes`.
* **Emparejamiento solo por `repository`** (sin distinguir mayúsculas). Emparejar además
  por `folderName` daba falsos conflictos (Duke Nukem 3D contra `fury`, Sonic 1 contra
  `sonic2`): dos ports distintos del mismo repo (fgsfdsfgs) o carpetas que solo se parecen
  no son la misma app. El emparejamiento por carpeta se ha eliminado del cálculo.
* **La carátula del maestro se conserva**: `thumbnail_url` suele ser un PNG de Libretro
  Thumbnails (raw.githubusercontent.com, sin clave de API ni límite de peticiones) y se
  copia a `appIconUrl`; la capa de arte de Delanx ya lo cachea en
  `Cache/artwork/<folderName>.png`.
* Los conflictos con entradas ya verificadas se registran en el informe; **manda Delanx**:
  no se toca nada verificado.

Uso:
    python tools/import_ports_md.py --check     # solo informa (no escribe nada)
    python tools/import_ports_md.py --write     # escribe ports-master.json y el informe
"""
from __future__ import annotations

import argparse
import collections
import json
import pathlib
import re
import sys

REPO = pathlib.Path(__file__).resolve().parent.parent
MD_PATH = REPO / "switch_ports_catalog.md"
CATALOG_PATH = REPO / "romfs" / "catalog" / "switch-catalog.json"
MASTER_PATH = REPO / "romfs" / "catalog" / "ports-master.json"
REPORT_PATH = REPO / "docs" / "catalog-md-merge.md"

MASTER_SOURCE = (
    "Catálogo maestro del usuario (switch_ports_catalog.md, 110 títulos) importado por "
    "tools/import_ports_md.py. TODAS las entradas tienen verified=false: falta comprobar "
    "que cada proyecto publique un asset .nro para Switch. Delanx solo aporta las entradas "
    "que aún no están verificadas en romfs/catalog/switch-catalog.json."
)

# Campo nuevo del bloque switch: carpeta final de la app (install_dir del maestro).
VERIFY_NOTE = (
    "PENDIENTE DE VERIFICAR: comprobar que el proyecto publique un asset .nro para Switch."
)
PATH_NOTE = (
    "targetDir = carpeta final (switch.homeDir) porque no se ha verificado el contenido del "
    "asset: correcto si el zip no trae el prefijo switch/; si lo trae, hay que cambiarlo a /."
)


def read_master_entries() -> list[dict]:
    text = MD_PATH.read_text(encoding="utf-8")
    blocks = re.findall(r"```json\s*(.*?)```", text, re.S)
    if not blocks:
        raise SystemExit("no se encontro ningun bloque ```json``` en el catalogo maestro")
    return json.loads(blocks[0])


def normalize_target(install_dir: str) -> str:
    """`sdmc:/switch/Ghostship/` -> `/switch/Ghostship` (carpeta absoluta de la SD)."""
    value = (install_dir or "").strip()
    if value.lower().startswith("sdmc:"):
        value = value[5:]
    while value.endswith("/"):
        value = value[:-1]
    if not value.startswith("/"):
        value = "/" + value
    return value


def folder_from(entry: dict) -> str:
    target = normalize_target(entry.get("install_dir", ""))
    tail = target.rsplit("/", 1)[-1] if target else ""
    return tail or entry.get("id", "")


def is_direct_image(url: str) -> bool:
    """¿La URL devuelve bytes de imagen (descargable tal cual por la capa de carátulas)?

    Vale lo que empieza por `https://raw.githubusercontent.com/` o lo que termina en una
    extensión de imagen. El resto (p. ej. `/api/v2/search/autocomplete/...` de SteamGridDB)
    exige clave y devuelve JSON: cachearlo como PNG guardaría basura.
    """
    value = (url or "").strip()
    if not value:
        return False
    if value.lower().startswith("https://raw.githubusercontent.com/"):
        return True
    clean = value.split("?", 1)[0].split("#", 1)[0].lower()
    return clean.endswith((".png", ".jpg", ".jpeg", ".webp"))


def to_delanx_entry(entry: dict) -> dict:
    """Entrada del catálogo de Delanx (verified=false) a partir de una del maestro."""
    home = normalize_target(entry.get("install_dir", ""))
    nro = (entry.get("executable") or "").strip() or None
    # switch.homeDir ya es la carpeta final; switch.targetDir es la raíz de extracción.
    # Sin verificar el asset se usa la carpeta final como raíz (ver cabecera y notes).
    target = home

    required = []
    for name in entry.get("required_files") or []:
        name = str(name).strip()
        if not name:
            continue
        required.append(
            {
                "path": f"sdmc:{home}/{name}",
                "description": entry.get("rom_source_hint") or "Fichero que debe aportar el usuario",
            }
        )

    tags = []
    for tag in (entry.get("type"), entry.get("original_system"), entry.get("category")):
        if tag:
            tags.append(str(tag).lower())

    notes = (
        f"Importado del catálogo maestro (switch_ports_catalog.md, id {entry.get('id')}). "
        f"{VERIFY_NOTE} {PATH_NOTE}"
    )
    if entry.get("asset_type"):
        notes += f" Tipo de asset declarado: {entry['asset_type']}."
    if entry.get("rom_source_hint"):
        notes += f" Datos del usuario: {entry['rom_source_hint']}."
    if entry.get("notes"):
        notes += f" {entry['notes']}"
    if entry.get("romfs_fallback"):
        notes += (
            f" Icono de reserva propuesto en el maestro: {entry['romfs_fallback']} "
            "(romfs de Delanx todavía no lo incluye)."
        )

    # La capa de carátulas descarga bytes y los cachea como PNG: solo sirve una imagen
    # directa. El resto de referencias de arte se conservan en artSource (no se resuelven).
    thumbnail = entry.get("thumbnail_url") or None
    direct_icon = thumbnail if is_direct_image(thumbnail or "") else None
    art_source = None if direct_icon else thumbnail
    if art_source:
        notes += (
            f" Carátula pendiente: el maestro apunta a una API de arte ({art_source}) que no "
            "devuelve la imagen directamente, así que no se usa como appIconUrl."
        )

    return {
        "name": entry.get("title") or entry.get("id"),
        "project": entry.get("project") or "",
        "repository": (entry.get("github_repo") or "").strip(),
        "repositorySource": "github",
        "folderName": folder_from(entry),
        "tags": tags,
        "releaseAssetFilter": None,
        "requirements": entry.get("rom_source_hint") or "",
        "switchProof": [],
        "verified": False,
        "harbourMasters": str(entry.get("project", "")).startswith("HarbourMasters"),
        "notes": notes,
        "appIconUrl": direct_icon,
        "artSource": art_source,
        "switch": {
            "homeDir": home,
            "targetDir": target,
            "nro": nro,
            "nroPath": nro,
            "stripRoot": False,
            "requiredUserFiles": required,
        },
    }


def unique_folder(candidate: dict, used: dict[str, str]) -> str:
    """Evita carpetas repetidas (dos ports distintos pueden declarar el mismo install_dir).

    No se descarta la entrada: se le añade sufijo para que no comparta instalación.
    """
    folder = candidate["folderName"]
    key = folder.lower()
    if key not in used:
        used[key] = candidate["name"]
        return folder
    index = 2
    while f"{key}-{index}" in used:
        index += 1
    candidate["folderName"] = f"{folder}-{index}"
    candidate["notes"] += (
        f" Carpeta renombrada a {candidate['folderName']}: el catálogo maestro usaba "
        f"{folder} en más de una entrada."
    )
    used[f"{key}-{index}"] = candidate["name"]
    return candidate["folderName"]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--write", action="store_true", help="escribe ports-master.json y el informe")
    parser.add_argument("--check", action="store_true", help="solo informa (por defecto)")
    args = parser.parse_args()

    master = read_master_entries()
    catalog = json.loads(CATALOG_PATH.read_text(encoding="utf-8"))
    verified_entries = catalog["entries"]

    # Emparejamiento SOLO por repository (insensible a mayúsculas). Se admite más de una
    # entrada del maestro por repo (p. ej. Cpasjuste/pemu publica 5 emuladores distintos y
    # fgsfdsfgs publica varios ports): cada coincidencia verifica una sola entrada.
    # Emparejamiento por repository y folderName/homeDir.
    by_repo: dict[str, list[dict]] = collections.defaultdict(list)
    for item in verified_entries:
        repo = (item.get("repository") or "").strip().lower()
        if repo:
            by_repo[repo].append(item)

    # Mapa para proveedor de arte en informe
    by_repo_thumbnail = {
        (raw.get("github_repo") or "").strip().lower(): raw
        for raw in master if raw.get("github_repo")
    }

    used_folders: dict[str, str] = {}
    already = 0            # entradas del maestro que ya están verificadas en Delanx
    skipped_no_repo = 0    # entradas del maestro sin github_repo (no instalables)
    already_matches: list[tuple[str, str, str]] = []
    conflicts: list[tuple[str, str, str, str]] = []
    added_entries: list[dict] = []
    discarded: list[tuple[str, str]] = []

    for raw in master:
        label = raw.get("title") or raw.get("id") or "?"
        repo = (raw.get("github_repo") or "").strip()
        if not repo:
            skipped_no_repo += 1
            discarded.append((label, "sin github_repo en el catálogo maestro"))
            continue

        key = repo.lower()
        candidate = to_delanx_entry(raw)
        if not candidate["folderName"]:
            discarded.append((label, "install_dir vacío: no se puede deducir la carpeta final"))
            continue

        bucket = by_repo.get(key)
        matched = None
        if bucket:
            # Si hay múltiples entradas para el mismo repo (p. ej. fgsfdsfgs),
            # buscar coincidencia por folderName / homeDir / ejecutable
            cand_folder = candidate["folderName"].lower()
            for idx, item in enumerate(bucket):
                if item.get("folderName", "").lower() == cand_folder or item.get("name", "").lower() == candidate["name"].lower():
                    matched = bucket.pop(idx)
                    break
            if not matched and len(bucket) == 1:
                matched = bucket.pop(0)

        if matched:
            # Ya lo tiene Delanx y verificado: no se duplica ni se toca nada verificado.
            ours = matched.get("switch") or {}
            our_target = ours.get("targetDir") or ""
            our_nro = ours.get("nro") or ""
            if our_target and candidate["switch"]["targetDir"] and our_target != candidate["switch"]["targetDir"]:
                conflicts.append((matched.get("name", label), "targetDir", our_target,
                                  candidate["switch"]["targetDir"]))
            if our_nro and candidate["switch"]["nro"] and our_nro != candidate["switch"]["nro"]:
                conflicts.append((matched.get("name", label), "ejecutable", our_nro,
                                  candidate["switch"]["nro"]))
            already += 1
            already_matches.append((repo, label, matched.get("name", "?")))
            continue

        unique_folder(candidate, used_folders)
        added_entries.append(candidate)

    total_after = len(verified_entries) + len(added_entries)

    direct_icons = sum(1 for item in added_entries if item["appIconUrl"])
    api_art = len(added_entries) - direct_icons
    api_providers = sorted({
        (by_repo_thumbnail.get(item["repository"].lower()) or {}).get("thumbnail_provider") or "sin proveedor"
        for item in added_entries if item["artSource"]
    })

    print(f"entradas del catálogo maestro          : {len(master)}")
    print(f"  con github_repo                     : {len(master) - skipped_no_repo}")
    print(f"  sin github_repo (no instalables)     : {skipped_no_repo}")
    print(f"ya presentes en Delanx (no se duplican) : {already}")
    for repo, label, matched_name in already_matches:
        print(f"  = {label}  ({repo})  ->  verificada «{matched_name}»")
    print(f"nuevas en ports-master.json             : {len(added_entries)}")
    print(f"carátulas directas (appIconUrl)         : {direct_icons}")
    print(f"referencias a API de arte (artSource)   : {api_art}"
          + (f"  [{'/'.join(api_providers)}]" if api_providers else ""))
    print(f"conflictos de ruta/ejecutable           : {len(conflicts)}")
    for name, kind, ours, theirs in conflicts:
        print(f"  ! {name} [{kind}]: Delanx={ours} <> maestro={theirs}")
    if discarded:
        print(f"descartadas                             : {len(discarded)}")
        for name, why in discarded:
            print(f"  - {name}: {why}")
    print(f"total resultante (verificadas + nuevas) : {total_after}")

    if not args.write:
        print("\n(modo --check: no se ha escrito nada)")
        return 0

    output = {"schemaVersion": 1, "source": MASTER_SOURCE, "entries": added_entries}
    MASTER_PATH.write_text(json.dumps(output, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"\nescrito {MASTER_PATH.relative_to(REPO)}")

    lines = [
        "# Fusión del catálogo maestro",
        "",
        "Generado por `tools/import_ports_md.py` a partir de `switch_ports_catalog.md`.",
        "",
        "La salida (`romfs/catalog/ports-master.json`) es un catálogo **aparte** del verificado:",
        "todas sus entradas llevan `verified: false` y una nota que recuerda que falta comprobar",
        "que cada proyecto publique un asset `.nro` para Switch. Las entradas ya verificadas en",
        "`romfs/catalog/switch-catalog.json` **no se duplican** y **no se tocan**: en caso de",
        "discrepancia manda Delanx.",
        "",
        "| Métrica | Valor |",
        "|---|---|",
        f"| Entradas del catálogo maestro | {len(master)} |",
        f"| Con `github_repo` | {len(master) - skipped_no_repo} |",
        f"| Sin `github_repo` (no instalables) | {skipped_no_repo} |",
        f"| Ya presentes en Delanx (no se duplican) | {already} |",
        f"| Añadidas a `ports-master.json` (`verified: false`) | {len(added_entries)} |",
        f"| Conflictos de ruta/ejecutable (manda Delanx) | {len(conflicts)} |",
        f"| Total resultante (verificadas + nuevas) | {total_after} |",
        "",
        "Criterios aplicados:",
        "",
        "- Emparejamiento con Delanx **solo por `repository`** (sin distinguir mayúsculas).",
        "- `switch.homeDir` = carpeta final de la app (`install_dir` del maestro).",
        "- `switch.targetDir` = raíz de extracción; sin verificar el asset se usa la carpeta",
        "  final (correcto cuando el zip no trae el prefijo `switch/`) y queda anotado en `notes`.",
        "- `appIconUrl` = `thumbnail_url` del maestro: 44 PNG de Libretro Thumbnails",
        "  (`raw.githubusercontent.com`, descargable tal cual) y 60 URL de API de SteamGridDB",
        "  (`www.steamgriddb.com/api/v2/...`), que **no son imágenes** y necesitan API key: esas",
        "  60 carátulas fallarán hasta sustituir la URL por un enlace directo al PNG.",
        "",
    ]
    if added_entries:
        lines += ["## Entradas añadidas", "", "| Entrada del maestro | Repositorio y carpeta final |",
                  "|---|---|"]
        for raw in added_entries:
            lines.append(f"| {raw['name']} | `{raw['repository']}` → `{raw['switch']['homeDir']}` |")
        lines += [""]
    if already_matches:
        lines += ["## Ya verificadas en Delanx (no se duplican ni se tocan)", "",
                  "| Entrada del maestro | Repositorio | Entrada verificada que manda |", "|---|---|---|"]
        for raw_repo, label, matched_name in already_matches:
            lines.append(f"| {label} | `{raw_repo}` | {matched_name} |")
        lines += [""]
    if conflicts:
        lines += ["## Conflictos (se conserva lo verificado por Delanx)", "",
                  "| Entrada | Campo | Delanx (verificado) | Catálogo maestro |", "|---|---|---|---|"]
        lines += [f"| {name} | {kind} | `{ours}` | `{theirs}` |" for name, kind, ours, theirs in conflicts]
        lines += [""]
    if discarded:
        lines += ["## Entradas descartadas", "", "| Entrada | Motivo |", "|---|---|"]
        lines += [f"| {name} | {why} |" for name, why in discarded]
        lines += [""]
    REPORT_PATH.write_text("\n".join(lines), encoding="utf-8")
    print(f"escrito {REPORT_PATH.relative_to(REPO)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())

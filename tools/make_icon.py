#!/usr/bin/env python3
"""Genera el icono de Delanx (256x256 JPEG) que libnx incrusta en el NACP.

Uso:
    python tools/make_icon.py            # escribe ./icon.jpg

Requiere Pillow. El icono debe ser exactamente 256x256 y JPEG: es lo que
`nacptool`/`elf2nro` aceptan para el icono del menu de Horizon.
"""
from __future__ import annotations

import pathlib
import sys

from PIL import Image, ImageDraw, ImageFont

SIZE = 256
BG_TOP = (18, 22, 28)
BG_BOTTOM = (30, 40, 51)
ACCENT = (0, 195, 227)
ACCENT_ALT = (255, 69, 84)
TEXT = (242, 244, 248)


def find_font() -> pathlib.Path | None:
    root = pathlib.Path(__file__).resolve().parent.parent
    for candidate in (root / "romfs" / "fonts" / "Inter.ttf",):
        if candidate.exists():
            return candidate
    return None


def main() -> int:
    image = Image.new("RGB", (SIZE, SIZE), BG_TOP)
    draw = ImageDraw.Draw(image)

    # Fondo con degradado vertical.
    for y in range(SIZE):
        t = y / (SIZE - 1)
        color = tuple(int(BG_TOP[i] + (BG_BOTTOM[i] - BG_TOP[i]) * t) for i in range(3))
        draw.line([(0, y), (SIZE, y)], fill=color)

    # Esquinas redondeadas (la mascara se aplica al final sobre un fondo plano).
    radius = 44
    mask = Image.new("L", (SIZE, SIZE), 0)
    ImageDraw.Draw(mask).rounded_rectangle([0, 0, SIZE - 1, SIZE - 1], radius=radius, fill=255)
    flat = Image.new("RGB", (SIZE, SIZE), BG_TOP)
    image = Image.composite(image, flat, mask)
    draw = ImageDraw.Draw(image)

    # Banda diagonal de acento (guiño a los ports de Harbour Masters).
    draw.polygon([(0, SIZE), (SIZE * 0.42, SIZE), (0, SIZE * 0.58)], fill=ACCENT_ALT)

    # Letra D grande.
    font_path = find_font()
    letter_font = None
    if font_path:
        for size in (168, 150, 132):
            try:
                letter_font = ImageFont.truetype(str(font_path), size)
                break
            except OSError:
                continue
    if letter_font is None:
        letter_font = ImageFont.load_default()

    box = draw.textbbox((0, 0), "D", font=letter_font)
    width = box[2] - box[0]
    height = box[3] - box[1]
    draw.text(((SIZE - width) / 2 - box[0], (SIZE - height) / 2 - box[1] - 6), "D", font=letter_font, fill=TEXT)

    # Punto de acento superior derecho.
    draw.ellipse([SIZE - 62, 34, SIZE - 30, 66], fill=ACCENT)

    out = pathlib.Path(__file__).resolve().parent.parent / "icon.jpg"
    image.save(out, "JPEG", quality=92)
    print(f"icono escrito en {out} ({image.size[0]}x{image.size[1]})")
    return 0


if __name__ == "__main__":
    sys.exit(main())

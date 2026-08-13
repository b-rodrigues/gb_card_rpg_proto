#!/usr/bin/env python3
"""png2gb.py -- PNG -> Game Boy 2bpp tile data converter.

Implements the pipeline stage specified in docs/graphics.md:

    PNG source assets
          |  tools/png2gb.py
    validation (dimensions, 8x8 tile alignment, palette limits, sprite
                size, unsupported colors, duplicate tiles, tile counts)
          |
    GB-native data (tileset bytes, tilemaps, OAM sprite defs, palettes)

This is a minimal first slice: single-image -> tileset only (no tilemap /
OAM defs / duplicate-tile dedup yet -- see TODOs). It intentionally
mirrors the style of the hand-authored tile in src/ui/ui.c (player_sprite_tile)
so its output can be dropped in directly or diffed against hand-authored
data.

Errors are actionable per docs/graphics.md ("which asset, which rule
violated") -- every failure names the source file and the specific
constraint it broke, never a generic traceback.
"""

import sys
import argparse
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    sys.exit("png2gb.py: requires Pillow (`pip install Pillow`)")

MAX_COLORS = 4
TILE_SIZE = 8

# Fixed, global GB palette (lightest -> darkest), matching cgb_palette in
# src/ui/ui.c and the DMG BGP_REG/OBP0_REG = 0xE4 mapping (0b11 10 01 00).
# Colors must map onto THIS fixed 4-shade space, not a palette re-indexed
# per-image -- otherwise two assets that each only use white+black would
# silently get different absolute shades depending on what else happened
# to be in the same PNG.
CANONICAL_SHADES = [
    (255, 255, 255),  # 0: white / lightest (sprite-transparent)
    (170, 170, 170),  # 1: light gray
    (85, 85, 85),      # 2: dark gray
    (0, 0, 0),          # 3: black / darkest
]


class Png2GbError(Exception):
    """Validation failure: (asset path, rule violated, detail)."""
    def __init__(self, asset, rule, detail):
        self.asset = asset
        self.rule = rule
        self.detail = detail
        super().__init__(f"{asset}: [{rule}] {detail}")


def load_and_validate(path):
    """Load a PNG and validate it against the GB tile constraints.
    Returns (PIL.Image in RGB, tiles_x, tiles_y)."""
    asset = str(path)
    try:
        img = Image.open(path)
    except Exception as e:
        raise Png2GbError(asset, "unreadable", str(e))

    img = img.convert("RGB")
    w, h = img.size

    if w % TILE_SIZE != 0 or h % TILE_SIZE != 0:
        raise Png2GbError(
            asset, "tile-alignment",
            f"{w}x{h} is not a multiple of {TILE_SIZE}x{TILE_SIZE} "
            f"(GB tiles are {TILE_SIZE}x{TILE_SIZE} pixels)"
        )

    colors = img.getcolors(maxcolors=256)
    if colors is None or len(colors) > MAX_COLORS:
        n = "more than 256" if colors is None else str(len(colors))
        raise Png2GbError(
            asset, "palette-limit",
            f"image uses {n} distinct colors; GB tiles support at most "
            f"{MAX_COLORS} (2bpp)"
        )

    return img, w // TILE_SIZE, h // TILE_SIZE


def build_shade_map(img, asset):
    """Map each distinct color in the image to its GB shade index (0-3)
    against the FIXED canonical 4-shade palette (not a palette re-indexed
    per-image) -- see CANONICAL_SHADES. Any color that isn't a close
    match to one of the four canonical shades is an "unsupported color"
    per docs/graphics.md and fails validation with an actionable error,
    naming the offending RGB value rather than silently snapping it."""
    colors = [c for _, c in img.getcolors(maxcolors=256)]
    shade_map = {}
    for color in colors:
        best_idx, best_dist = None, None
        for idx, shade in enumerate(CANONICAL_SHADES):
            dist = sum((a - b) ** 2 for a, b in zip(color, shade))
            if best_dist is None or dist < best_dist:
                best_idx, best_dist = idx, dist
        # Exact match required: this is a *validation* tool, not a
        # lossy quantizer -- silently snapping an off-palette color to
        # "the nearest shade" is exactly the kind of silently-broken
        # graphics docs/graphics.md says the converter must not produce.
        if color != CANONICAL_SHADES[best_idx]:
            raise Png2GbError(
                asset, "unsupported-color",
                f"RGB{color} is not one of the 4 canonical GB shades "
                f"{CANONICAL_SHADES}; closest is shade {best_idx} "
                f"{CANONICAL_SHADES[best_idx]} but it is not an exact match"
            )
        shade_map[color] = best_idx
    return shade_map


def encode_tile(img, tile_x, tile_y, shade_map):
    """Encode one 8x8 tile block starting at (tile_x*8, tile_y*8) into
    16 bytes of GB 2bpp tile data (2 bytes per row: low bitplane, high
    bitplane; MSB = leftmost pixel), per the standard GB tile format."""
    px = img.load()
    out = bytearray()
    ox, oy = tile_x * TILE_SIZE, tile_y * TILE_SIZE
    for row in range(TILE_SIZE):
        lo = 0
        hi = 0
        for col in range(TILE_SIZE):
            shade = shade_map[px[ox + col, oy + row]]
            bit_pos = 7 - col
            if shade & 0b01:
                lo |= (1 << bit_pos)
            if shade & 0b10:
                hi |= (1 << bit_pos)
        out.append(lo)
        out.append(hi)
    return bytes(out)


def ascii_preview(tile_bytes):
    """Render a tile's on/off pattern as an ASCII comment, matching the
    style already used for player_sprite_tile in src/ui/ui.c."""
    lines = []
    for row in range(TILE_SIZE):
        lo = tile_bytes[row * 2]
        hi = tile_bytes[row * 2 + 1]
        chars = []
        for col in range(TILE_SIZE):
            bit_pos = 7 - col
            shade = (((hi >> bit_pos) & 1) << 1) | ((lo >> bit_pos) & 1)
            chars.append(" .:#"[shade])
        lines.append("".join(chars))
    return lines


def format_c_array(name, all_tile_bytes, tiles_x, tiles_y):
    """Emit a C byte array in the same style as the hand-authored
    player_sprite_tile array in src/ui/ui.c, with an ASCII-art comment
    per tile row so it stays human-reviewable."""
    lines = [f"static const uint8_t {name}[{len(all_tile_bytes)}] = {{"]
    for t in range(tiles_x * tiles_y):
        tile = all_tile_bytes[t * 16:(t + 1) * 16]
        preview = ascii_preview(tile)
        if tiles_x * tiles_y > 1:
            lines.append(f"    /* tile {t} */")
        for row in range(TILE_SIZE):
            lo, hi = tile[row * 2], tile[row * 2 + 1]
            comma = "," if not (t == tiles_x * tiles_y - 1 and row == TILE_SIZE - 1) else ""
            lines.append(f"    0x{lo:02X}, 0x{hi:02X}{comma}   /* {preview[row]} */")
    lines.append("};")
    return "\n".join(lines)


def convert(path, name):
    img, tiles_x, tiles_y = load_and_validate(path)
    shade_map = build_shade_map(img, str(path))

    all_bytes = bytearray()
    for ty in range(tiles_y):
        for tx in range(tiles_x):
            all_bytes += encode_tile(img, tx, ty, shade_map)

    return all_bytes, tiles_x, tiles_y, format_c_array(name, all_bytes, tiles_x, tiles_y)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("png", type=Path, help="source PNG (dimensions must be multiples of 8x8, <=4 colors)")
    ap.add_argument("--name", default="tile_data", help="C array name (default: tile_data)")
    ap.add_argument("-o", "--out", type=Path, help="write generated C snippet here (default: stdout)")
    args = ap.parse_args()

    try:
        all_bytes, tiles_x, tiles_y, c_src = convert(args.png, args.name)
    except Png2GbError as e:
        print(f"png2gb: {e.asset}: [{e.rule}] {e.detail}", file=sys.stderr)
        sys.exit(1)

    header = (
        f"/* Generated by tools/png2gb.py from {args.png.name} "
        f"({tiles_x * TILE_SIZE}x{tiles_y * TILE_SIZE}px, "
        f"{tiles_x * tiles_y} tile{'s' if tiles_x * tiles_y != 1 else ''}). */\n"
    )
    output = header + c_src + "\n"

    if args.out:
        args.out.write_text(output)
        print(f"png2gb: wrote {args.out} ({len(all_bytes)} bytes, {tiles_x * tiles_y} tile(s))", file=sys.stderr)
    else:
        print(output)


if __name__ == "__main__":
    main()

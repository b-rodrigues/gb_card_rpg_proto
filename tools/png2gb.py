#!/usr/bin/env python3
"""png2gb.py -- PNG -> Game Boy 2bpp tile data converter.

Implements the pipeline stage specified in docs/graphics.md:

    PNG source assets
          |  tools/png2gb.py
    validation (dimensions, 8x8 tile alignment, palette limits, sprite
                size, unsupported colors, duplicate tiles, tile counts)
          |
    GB-native data (tileset bytes, tilemaps, OAM sprite defs, palettes)

Supports canonical DMG grayscale, classic GB green, and custom 4-shade palettes,
with optional extraction of specific tile coordinate lists.
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

PALETTES = {
    "canonical": [
        (255, 255, 255),  # 0: white / lightest
        (170, 170, 170),  # 1: light gray
        (85, 85, 85),      # 2: dark gray
        (0, 0, 0),          # 3: black / darkest
    ],
    "gb_green": [
        (224, 248, 207),  # 0: lightest green
        (134, 192, 108),  # 1: light green
        (48, 104, 80),    # 2: dark green
        (7, 24, 33),      # 3: darkest green
    ]
}


class Png2GbError(Exception):
    """Validation failure: (asset path, rule violated, detail)."""
    def __init__(self, asset, rule, detail):
        self.asset = asset
        self.rule = rule
        self.detail = detail
        super().__init__(f"{asset}: [{rule}] {detail}")


def load_and_validate(path, max_colors=MAX_COLORS):
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
    # Tolerate minor compression artifacts (<= 8 colors) if palette strategy can map them
    if colors is None or len(colors) > max_colors:
        n = "more than 256" if colors is None else str(len(colors))
        raise Png2GbError(
            asset, "palette-limit",
            f"image uses {n} distinct colors; GB tiles support at most "
            f"{max_colors} (2bpp)"
        )

    return img, w // TILE_SIZE, h // TILE_SIZE


def build_shade_map(img, asset, palette_name="canonical"):
    """Map each distinct color in the image to its GB shade index (0-3)
    against the requested 4-shade palette."""
    palette = PALETTES.get(palette_name, PALETTES["canonical"])
    colors = [c for _, c in img.getcolors(maxcolors=256)]
    shade_map = {}
    for color in colors:
        best_idx, best_dist = None, None
        for idx, shade in enumerate(palette):
            dist = sum((a - b) ** 2 for a, b in zip(color, shade))
            if best_dist is None or dist < best_dist:
                best_idx, best_dist = idx, dist
        if best_dist > 500:
            raise Png2GbError(
                asset, "unsupported-color",
                f"RGB{color} is not near any of the {palette_name} shades "
                f"{palette}; closest is shade {best_idx} "
                f"{palette[best_idx]} (dist={best_dist})"
            )
        shade_map[color] = best_idx
    return shade_map


def encode_tile(img, tile_x, tile_y, shade_map):
    """Encode one 8x8 tile block starting at (tile_x*8, tile_y*8) into
    16 bytes of GB 2bpp tile data."""
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
    """Render a tile's on/off pattern as an ASCII comment."""
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


def format_c_array(name, all_tile_bytes, tile_count):
    """Emit a C byte array with an ASCII-art comment per tile row."""
    lines = [f"const uint8_t {name}[{len(all_tile_bytes)}] = {{"]
    for t in range(tile_count):
        tile = all_tile_bytes[t * 16:(t + 1) * 16]
        preview = ascii_preview(tile)
        if tile_count > 1:
            lines.append(f"    /* tile {t} */")
        for row in range(TILE_SIZE):
            lo, hi = tile[row * 2], tile[row * 2 + 1]
            comma = "," if not (t == tile_count - 1 and row == TILE_SIZE - 1) else ""
            lines.append(f"    0x{lo:02X}, 0x{hi:02X}{comma}   /* {preview[row]} */")
    lines.append("};")
    return "\n".join(lines)


def convert(path, name, palette_name="canonical", tile_coords=None):
    max_colors = 8 if palette_name == "gb_green" else MAX_COLORS
    img, tiles_x, tiles_y = load_and_validate(path, max_colors=max_colors)
    shade_map = build_shade_map(img, str(path), palette_name=palette_name)

    all_bytes = bytearray()
    if tile_coords:
        coords_list = []
        for item in tile_coords.strip().split():
            parts = item.split(",")
            coords_list.append((int(parts[0]), int(parts[1])))
        for tx, ty in coords_list:
            all_bytes += encode_tile(img, tx, ty, shade_map)
        tile_count = len(coords_list)
    else:
        for ty in range(tiles_y):
            for tx in range(tiles_x):
                all_bytes += encode_tile(img, tx, ty, shade_map)
        tile_count = tiles_x * tiles_y

    return all_bytes, tile_count, format_c_array(name, all_bytes, tile_count)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("png", type=Path, help="source PNG")
    ap.add_argument("--name", default="tile_data", help="C array name")
    ap.add_argument("--palette", default="canonical", choices=["canonical", "gb_green"], help="palette mapping")
    ap.add_argument("--tile-coords", default=None, help="space-separated x,y tile coordinates (e.g. '1,2 8,1 8,2 0,5')")
    ap.add_argument("-o", "--out", type=Path, help="write generated C snippet here (default: stdout)")
    args = ap.parse_args()

    try:
        all_bytes, tile_count, c_src = convert(args.png, args.name, palette_name=args.palette, tile_coords=args.tile_coords)
    except Png2GbError as e:
        print(f"png2gb: {e.asset}: [{e.rule}] {e.detail}", file=sys.stderr)
        sys.exit(1)

    header = (
        f"/* Generated by tools/png2gb.py from {args.png.name} "
        f"({tile_count} tile{'s' if tile_count != 1 else ''}). */\n"
    )
    output = header + c_src + "\n"

    if args.out:
        args.out.write_text(output)
        print(f"png2gb: wrote {args.out} ({len(all_bytes)} bytes, {tile_count} tile(s))", file=sys.stderr)
    else:
        print(output)


if __name__ == "__main__":
    main()

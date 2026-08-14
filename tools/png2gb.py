"""png2gb.py -- PNG -> Game Boy 2bpp tile data converter.

Implements the pipeline stage specified in docs/graphics.md:

    PNG source assets
          |  tools/png2gb.py
    validation (dimensions, 8x8 tile alignment, palette limits, sprite
                size, unsupported colors, duplicate tiles, tile counts)
          |
    GB-native data (tileset bytes, tilemaps, OAM sprite defs, palettes)

Modes:

* Default (single image): image -> tileset byte array.  Validates
  dimensions, 8x8 alignment, palette limits, unsupported colors, and the
  VRAM tile-count ceiling.
* --tilemap: a multi-tile image -> a deduped tileset PLUS a row-major
  tilemap array (tile indices into the deduped tileset).  Duplicate 8x8
  tiles are emitted once; the tilemap references them by index.
* --sprite: an 8x8 or 8x16 frame sheet -> tile data plus an OAM-style
  definition list (frame, tile id, width/height) for the OAM sprite
  engine.  Validates the sprite-frame dimensions.

The output intentionally mirrors the style of the hand-authored tile in
src/ui/ui.c (player_sprite_tile) so it can be dropped in directly or
diffed against hand-authored data.

Errors are actionable per docs/graphics.md ("which asset, which rule
violated") -- every failure names the source file and the specific
constraint it broke, never a generic traceback.

--selftest runs byte-exact self-checks on generated inputs and exits
non-zero on any failure (deterministic host-side regression test).
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
SPRITE_8X8 = 8
SPRITE_8X16 = 16

# VRAM tile ceiling: 0x8000-0x8FFF is 128 tiles, but the project loads the
# overworld tileset at WORLD_TILE_BASE (>= 128) so it never collides with the
# GBDK console font (0..127).  A single asset must therefore stay within the
# 128-tile window below 0x8800.  Two assets must not overlap their ranges.
VRAM_TILE_LIMIT = 128

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

    tiles_x, tiles_y = w // TILE_SIZE, h // TILE_SIZE
    n_tiles = tiles_x * tiles_y
    if n_tiles > VRAM_TILE_LIMIT:
        raise Png2GbError(
            asset, "tile-count",
            f"{n_tiles} tiles exceed the {VRAM_TILE_LIMIT}-tile VRAM window "
            f"below 0x8800 (the console font occupies 0..127 and the "
            f"overworld tileset loads at WORLD_TILE_BASE >= 128)"
        )

    return img, tiles_x, tiles_y


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


def dedup_tiles(all_tile_bytes):
    """Split the raw tile stream into deduped tiles + an index map.

    Returns (tileset, tilemap) where tileset is the unique 16-byte tiles
    in first-seen order and tilemap[i] is the tileset index for raw
    tile i.  Tiles that never repeat keep their original order so a
    single-frame asset (player_demo.png) round-trips byte-identically.
    """
    unique = []          # list of 16-byte tile blobs
    index = {}           # blob -> tileset index
    tilemap = []
    for i in range(0, len(all_tile_bytes), 16):
        tile = bytes(all_tile_bytes[i:i + 16])
        if tile not in index:
            index[tile] = len(unique)
            unique.append(tile)
        tilemap.append(index[tile])
    return b"".join(unique), tilemap


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


def format_c_array(name, all_tile_bytes, tiles_x, tiles_y, is_global=False):
    """Emit a C byte array in the same style as the hand-authored
    player_sprite_tile array in src/ui/ui.c, with an ASCII-art comment
    per tile row so it stays human-reviewable.  is_global drops the
    'static' so the array can be declared extern and placed in a banked
    ROM region (see --global)."""
    storage = "" if is_global else "static "
    lines = [f"{storage}const uint8_t {name}[{len(all_tile_bytes)}] = {{"]
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


def format_tilemap(name, tilemap, tiles_x, tiles_y, is_global=False):
    """Emit the deduped tilemap as a C 2D array (tile indices)."""
    storage = "" if is_global else "static "
    lines = [
        f"{storage}const uint8_t {name}[{tiles_y}][{tiles_x}] = {{",
    ]
    for y in range(tiles_y):
        row = tilemap[y * tiles_x:(y + 1) * tiles_x]
        body = ", ".join(str(v) for v in row)
        comma = "," if y != tiles_y - 1 else ""
        lines.append(f"    {{ {body} }}{comma}")
    lines.append("};")
    return "\n".join(lines)


def format_oam_defs(name, frames, tile_bytes_per_frame):
    """Emit OAM-style frame definitions: frame index, tile id in the
    deduped tileset, and the frame's pixel dimensions (8x8 or 8x16)."""
    lines = [
        f"static const SpriteFrameDef {name}[{len(frames)}] = {{",
    ]
    for i, (frame_index, w, h) in enumerate(frames):
        comma = "," if i != len(frames) - 1 else ""
        lines.append(
            f"    {{ {frame_index}, {w}, {h} }}{comma}  "
            f"/* frame {i}: {w}x{h} */"
        )
    lines.append("};")
    return "\n".join(lines)


def convert(path, name, is_global=False):
    img, tiles_x, tiles_y = load_and_validate(path)
    shade_map = build_shade_map(img, str(path))

    all_bytes = bytearray()
    for ty in range(tiles_y):
        for tx in range(tiles_x):
            all_bytes += encode_tile(img, tx, ty, shade_map)

    return all_bytes, tiles_x, tiles_y, format_c_array(name, all_bytes, tiles_x, tiles_y, is_global)


def convert_tilemap(path, name, is_global=False):
    """Multi-tile image -> (deduped tileset, tilemap array, C snippets)."""
    img, tiles_x, tiles_y = load_and_validate(path)
    shade_map = build_shade_map(img, str(path))

    all_bytes = bytearray()
    for ty in range(tiles_y):
        for tx in range(tiles_x):
            all_bytes += encode_tile(img, tx, ty, shade_map)

    tileset, tilemap = dedup_tiles(all_bytes)
    n_dup = len(all_bytes) // 16 - len(tileset) // 16

    out = [format_c_array(name + "_tiles", tileset, 1, len(tileset) // 16, is_global),
           format_tilemap(name + "_map", tilemap, tiles_x, tiles_y, is_global)]
    return tileset, tilemap, n_dup, tiles_x, tiles_y, "\n\n".join(out)


def convert_sprite(path, name, frame_h, is_global=False):
    """Sprite frame sheet -> tiles + OAM defs.

    Sprites do NOT dedup tiles: an OAM 8x16 sprite needs its two 8x8 tiles
    contiguous (tile id and id+1), so the tileset is the raw frame sheet in
    row-major tile order.  Each frame def is (top-left tile id, w, h).
    frame_h must be 8 (8x8 sprites) or 16 (8x16 sprites) and the sheet
    width must be exactly 8.
    """
    if frame_h not in (SPRITE_8X8, SPRITE_8X16):
        raise Png2GbError(
            str(path), "sprite-size",
            f"frame height must be {SPRITE_8X8} or {SPRITE_8X16} (got {frame_h})"
        )
    img, tiles_x, tiles_y = load_and_validate(path)
    shade_map = build_shade_map(img, str(path))

    w, h = img.size
    if w != SPRITE_8X8:
        raise Png2GbError(
            str(path), "sprite-size",
            f"{w}x{h}: a sprite sheet must be 8 pixels wide "
            f"(OAM sprites are 8 wide, 8 or 16 tall)"
        )
    if h % frame_h != 0:
        raise Png2GbError(
            str(path), "sprite-size",
            f"{w}x{h}: height must be a multiple of {frame_h} for "
            f"{frame_h}-tall frames"
        )

    all_bytes = bytearray()
    for ty in range(tiles_y):
        for tx in range(tiles_x):
            all_bytes += encode_tile(img, tx, ty, shade_map)

    tiles_per_frame = frame_h // TILE_SIZE
    n_frames = h // frame_h
    frames = [(f * tiles_per_frame, w, frame_h) for f in range(n_frames)]

    out = [format_c_array(name + "_tiles", all_bytes, 1, len(all_bytes) // 16, is_global),
           format_oam_defs(name + "_frames", frames, tiles_per_frame)]
    return all_bytes, frames, n_frames, img.size[0], img.size[1], "\n\n".join(out)


def run_selftest():
    """Deterministic host-side regression tests for the converter."""
    failures = []

    def check(cond, msg):
        if not cond:
            failures.append(msg)

    def make_img(data, w, h):
        # data: list of (r,g,b) rows
        img = Image.new("RGB", (w, h))
        img.putdata([c for row in data for c in row])
        return img

    # Tile 0: all white.  Tile 1: a single dark-gray pixel in the top-left.
    blank = make_img([[(255, 255, 255)] * 8 for _ in range(8)], 8, 8)
    onepx = make_img([[(255, 255, 255)] * 8 for _ in range(8)], 8, 8)
    px = onepx.load()
    px[0, 0] = (85, 85, 85)

    # --selftest runs on an in-memory Path-like; reuse the file pipeline.
    import tempfile
    import os
    with tempfile.TemporaryDirectory() as td:
        p_blank = os.path.join(td, "blank.png")
        p_onepx = os.path.join(td, "onepx.png")
        p_sprite = os.path.join(td, "sprite.png")
        blank.save(p_blank)
        onepx.save(p_onepx)

        # Default mode: 8x8 single tile -> 16 bytes, all-zero low plane.
        b, tx, ty, c = convert(p_blank, "t")
        check(tx == 1 and ty == 1 and len(b) == 16 and b[:16] == bytes(16),
              "selftest: blank 8x8 tile must encode to 16 zero bytes")
        check(b[1] == 0 and b[0] == 0, "selftest: all-white tile is shade 0")

        # Unsupported color must fail with an actionable rule name.
        bad = make_img([[(10, 200, 30)] * 8 for _ in range(8)], 8, 8)
        p_bad = os.path.join(td, "bad.png")
        bad.save(p_bad)
        try:
            convert(p_bad, "t")
            check(False, "selftest: unsupported color must raise")
        except Png2GbError as e:
            check(e.rule == "unsupported-color", "selftest: wrong rule for bad color")

        # Non-aligned dimensions must fail.
        try:
            convert(p_blank, "t")  # 8x8 is aligned; build a 9x9 below
            img9 = Image.new("RGB", (9, 9))
            img9.save(os.path.join(td, "nine.png"))
            convert(os.path.join(td, "nine.png"), "t")
            check(False, "selftest: 9x9 must fail tile-alignment")
        except Png2GbError as e:
            check(e.rule == "tile-alignment", "selftest: wrong rule for 9x9")

        # Dedup: a 16x8 image of blank+onepx -> 2 unique tiles, tilemap [0,1].
        both = Image.new("RGB", (16, 8))
        both.paste(blank, (0, 0))
        both.paste(onepx, (8, 0))
        p_both = os.path.join(td, "both.png")
        both.save(p_both)
        tileset, tilemap, n_dup, tx2, ty2, _ = convert_tilemap(p_both, "both")
        check(tx2 == 2 and ty2 == 1, "selftest: 16x8 image is 2x1 tiles")
        check(n_dup == 0, "selftest: no duplicates in blank+onepx")
        check(len(tilemap) == 2 and tilemap == [0, 1], "selftest: tilemap [0,1]")
        check(len(tileset) == 32, "selftest: deduped tileset is 2 tiles")

        # Dedup with an actual duplicate: blank twice + onepx -> 2 unique.
        dup = Image.new("RGB", (24, 8))
        dup.paste(blank, (0, 0))
        dup.paste(blank, (8, 0))
        dup.paste(onepx, (16, 0))
        p_dup = os.path.join(td, "dup.png")
        dup.save(p_dup)
        tileset2, tilemap2, n_dup2, _, _, _ = convert_tilemap(p_dup, "dup")
        check(n_dup2 == 1, "selftest: one duplicate tile detected")
        check(len(tileset2) == 32, "selftest: deduped tileset is 2 tiles")
        check(tilemap2 == [0, 0, 1], "selftest: dup tilemap [0,0,1]")

        # Sprite mode: 8x16 sheet (two 8x8 frames) -> 2 frames, 8x8 each.
        sheet = Image.new("RGB", (8, 16))
        sheet.paste(onepx, (0, 0))
        sheet.paste(blank, (0, 8))
        p_sheet = os.path.join(td, "sheet.png")
        sheet.save(p_sheet)
        tileset3, frames, n_frames, _, _, _ = convert_sprite(p_sheet, "hero", SPRITE_8X8)
        check(n_frames == 2 and len(frames) == 2, "selftest: 8x16 sheet has 2 frames")
        check(len(tileset3) == 32, "selftest: sprite tileset keeps both 8x8 tiles")
        check(frames[0] == (0, 8, 8) and frames[1] == (1, 8, 8),
              "selftest: frames reference raw tile ids 0 and 1")

        # 8x16 sprites: 8x32 sheet -> 2 frames of two contiguous tiles each.
        sheet16 = Image.new("RGB", (8, 32))
        sheet16.paste(onepx, (0, 0))
        sheet16.paste(blank, (0, 8))
        sheet16.paste(onepx, (0, 16))
        sheet16.paste(blank, (0, 24))
        p_sheet16 = os.path.join(td, "sheet16.png")
        sheet16.save(p_sheet16)
        ts16, frames16, nf16, _, _, _ = convert_sprite(p_sheet16, "hero16", SPRITE_8X16)
        check(nf16 == 2, "selftest: 8x32 sheet has 2 8x16 frames")
        check(frames16[0] == (0, 8, 16) and frames16[1] == (2, 8, 16),
              "selftest: 8x16 frames use contiguous tile id pairs (0,1) (2,3)")

        # Invalid sprite width (16x8) must fail sprite-size.
        try:
            convert_sprite(p_both, "t", SPRITE_8X8)  # 16x8 is not 8 wide
            check(False, "selftest: 16x8 must fail sprite-size")
        except Png2GbError as e:
            check(e.rule == "sprite-size", "selftest: wrong rule for 16x8 sprite")

    if failures:
        for f in failures:
            print(f"png2gb selftest: FAIL: {f}", file=sys.stderr)
        sys.exit(1)
    print("png2gb selftest: all checks passed")


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("png", nargs="?", type=Path,
                    help="source PNG (dimensions must be multiples of 8x8, <=4 colors)")
    ap.add_argument("--name", default="tile_data", help="C array name (default: tile_data)")
    ap.add_argument("-o", "--out", type=Path, help="write generated C snippet here (default: stdout)")
    ap.add_argument("--tilemap", action="store_true",
                    help="emit a deduped tileset + a row-major tilemap array")
    ap.add_argument("--sprite", metavar="HEIGHT", type=int, choices=[8, 16],
                    help="validate as a sprite frame sheet (8 wide, 8 or 16 tall per frame) and emit OAM defs")
    ap.add_argument("--selftest", action="store_true", help="run built-in self-checks and exit")
    ap.add_argument("--global", dest="is_global", action="store_true",
                    help="emit non-static const arrays (for placing in a banked ROM region)")
    args = ap.parse_args()

    if args.selftest:
        run_selftest()
        return

    if not args.png:
        ap.error("the following arguments are required: png")

    try:
        if args.tilemap:
            all_bytes, _, n_dup, tiles_x, tiles_y, c_src = convert_tilemap(args.png, args.name, args.is_global)
            n_unique = len(all_bytes) // 16
            detail = (f"({n_unique} unique of {tiles_x * tiles_y} tiles, "
                      f"{n_dup} duplicate(s) deduped)")
        elif args.sprite is not None:
            all_bytes, _, n_frames, w, h, c_src = convert_sprite(args.png, args.name, args.sprite, args.is_global)
            tiles_x, tiles_y = w // TILE_SIZE, h // TILE_SIZE
            detail = f"({n_frames} frame(s))"
        else:
            all_bytes, tiles_x, tiles_y, c_src = convert(args.png, args.name, args.is_global)
            detail = ""
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
        print(f"png2gb: wrote {args.out} ({len(all_bytes)} bytes, {tiles_x * tiles_y} tile(s)) {detail}", file=sys.stderr)
    else:
        print(output)


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""Verify the console-font to VRAM tile mapping via the HUD tilemap mirror.

The GBDK console font (font_ibm) packs its glyphs starting at space (0x20),
so the tile for char `ch` is `ui_font_tile_base + (ch - ' ')`.  A regression
that drops the `- ' '` offset renders every character as the glyph for
`ch + 0x20` (e.g. `:` renders as `Z`), which shows up as garbled HUD,
dialogue, and menu text -- while the semantic g_ui_screen_buf stays correct,
so the harness assertions never catch it.

This check drives the debug ROM to the overworld (where the HUD window is
visible) and cross-checks the *tile* the ROM wrote against the *char* it
meant to render:

  tile == ui_font_tile_base + (ord(ch) - 0x20)

The HUD is read through the DEBUG-only g_hud_tilemap_mirror (WRAM), written
alongside each ui_hud_put_char write.  A WRAM mirror is used because the
window tilemap lives in VRAM, whose writes can be silently dropped during
scanout (mode 3) -- the mirror captures the ROM's write *intent*, which is
exactly what the font-mapping regression affects.

The check solves the base from the first non-space HUD cell and asserts it
is consistent across every cell (including spaces, which must map to the
blank tile `base + 0`), and that the base is 0 (font_ibm is the first font
loaded, so its first_tile is 0).

Run inside the Nix dev shell:
    make verify-font
or: nix develop --command python3 tools/verify_font.py

Exits non-zero if any assertion fails.
"""

import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from emulator import EmulatorSession

ROM = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                   "..", "build", "rpg_card_proto_debug.gb")
SCENARIOS = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                         "scenarios", "tests")

failures = []


def check(label, expected, actual):
    ok = actual == expected
    print(f"  [{'OK ' if ok else 'FAIL'}] {label}: expected {expected}, got {actual}")
    if not ok:
        failures.append(label)


def main():
    sess = EmulatorSession(rom_path=ROM)
    try:
        sess.connect()
    except Exception as e:
        print(f"CONNECT FAILED: {e}")
        return 1

    try:
        with open(os.path.join(SCENARIOS, "first_encounter.json")) as f:
            sess.load_scenario(json.load(f))
        sess.step(1)
        sess.wait(2)

        mirror = sess.get_hud_tilemap_mirror()
        if any(v is None for v in mirror):
            print("HUD mirror read returned None bytes")
            return 1
        screen = sess.get_screen_buf().split("\n")

        # Semantic HUD rows (12..17) give the chars the ROM intended.
        cells = []
        for y in range(6):
            for x in range(20):
                ch = screen[12 + y][x]
                tile = mirror[y * 32 + x]
                cells.append((y, x, ch, tile))

        # Solve the font base from the first non-space cell.
        base = None
        for y, x, ch, tile in cells:
            if ch != ' ':
                base = tile - (ord(ch) - 0x20)
                break
        check("font tile base solved from HUD", True, base is not None)
        if base is None:
            return 1
        check("font tile base is 0 (first loaded font)", 0, base)

        # Every cell must satisfy tile == base + (ch - ' ').
        bad = [(y, x, ch, tile) for y, x, ch, tile in cells
               if tile != base + (ord(ch) - 0x20 if ch != ' ' else 0)]
        check("all HUD cells use tile == base + (ch - ' ')", [], bad)

        print("  HUD text:", screen[12][:20])
        print("  HUD text:", screen[13][:20])
        print("  HUD text:", screen[17][:20])
    finally:
        sess.disconnect()

    if failures:
        print(f"\nFONT VERIFICATION FAILED: {failures}")
        return 1
    print("\nFONT VERIFICATION OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())

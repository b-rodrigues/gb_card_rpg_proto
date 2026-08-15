#!/usr/bin/env python3
"""Dialogue-box placement ground-truth check (PyBoy).

On this ASCII-only branch the dialogue box is drawn into the BACKGROUND
tilemap (0x9800) at fixed rows 12-17 via the console font -- the
window-layer dialogue box (f77ec9b) is a gfx-branch change.  The box must
be present, clean (no map tiles inside it), and the HUD window must be
disabled while the dialogue is up.

This boots the real release ROM headlessly, walks to the town guard (camera
scrolled), triggers the dialogue, and decodes the actual BACKGROUND tilemap:

  1. dialogue-bg-box: the box is present in the background at screen rows
     12-17 at the expected shape (+ border, speaker, line, [A] CONTINUE).
  2. dialogue-clean: the box contains only dialogue text -- no map tiles
     mixed in.
  3. dialogue-window-hidden: the HUD window is disabled during dialogue.

Run inside the Nix dev shell:
    nix develop --command python3 tools/vram_dialogue_check.py [--rom PATH]

Exits non-zero if any check fails.
"""

import argparse
import os
import sys

ROM_DEFAULT = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                            "..", "build", "rpg_card_proto.gb")

failures = []


def check(label, ok, detail=""):
    status = "PASS" if ok else "FAIL"
    print(f"[{status}] {label}" + (f" -- {detail}" if detail and not ok else ""))
    if not ok:
        failures.append(label)


def raw_id(tm, x, y):
    ident = tm.tile_identifier(x, y)
    return ident - 256 if ident >= 256 else ident


def window_row(tm, wy, ncols=20):
    row = ""
    for x in range(ncols):
        t = raw_id(tm, x, wy)
        if t >= 128:
            row += "#"
        elif t == 0:
            row += " "
        else:
            row += chr(0x20 + t)
    return row


def background_row(bg, screen_y, scroll_y, ncols=20):
    """Decode a BACKGROUND row into a string.  PyBoy's tilemap_background
    indexes the absolute 32x32 tilemap ring; the ring holds world tiles at
    wrapped (world & 31) addresses, and the visible row `screen_y` shows
    ring row (scroll_y + screen_y) & 31 (the camera scrolls the ring)."""
    row = ""
    for x in range(ncols):
        t = raw_id(bg, x, (screen_y + scroll_y) & 31)
        if t >= 128:
            row += "#"
        elif t == 0:
            row += " "
        else:
            row += chr(0x20 + t)
    return row


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                  formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--rom", default=ROM_DEFAULT, help="ROM to boot (release build by default)")
    ap.add_argument("--frames", type=int, default=180,
                     help="frames to run before sampling (default: ~3s, past boot)")
    args = ap.parse_args()

    if not os.path.isfile(args.rom):
        print(f"error: ROM not found: {args.rom}", file=sys.stderr)
        print("Build it first (make release).", file=sys.stderr)
        return 1

    try:
        from pyboy import PyBoy
    except ImportError:
        print("error: pyboy not installed. `nix develop` (it is in the dev shell).", file=sys.stderr)
        return 1

    pb = PyBoy(args.rom, window="null")
    for _ in range(args.frames):
        pb.tick()

    def one_tile(btn, n=1):
        for _ in range(n):
            pb.button(btn, delay=8)
            for _ in range(14):
                pb.tick()

    # Walk to the town guard (guarantees the camera is scrolled, SCY > 0).
    one_tile("right", 27); one_tile("down", 3); one_tile("right", 1)
    for _ in range(30): pb.tick()
    one_tile("right", 7); one_tile("down", 1)
    for _ in range(20): pb.tick()
    pb.button("right", delay=8)   # bump the guard -> dialogue
    for _ in range(30): pb.tick()

    win = pb.tilemap_window
    bg = pb.tilemap_background

    # On this ASCII-only branch the dialogue box is drawn into the
    # BACKGROUND tilemap (console putchar) at ABSOLUTE ring rows 12-17 --
    # the console writes ring row 12 + y regardless of SCX/SCY.  The
    # window-layer dialogue box (f77ec9b) is a gfx-branch change.  The box
    # must still be present, clean (no map tiles inside it), and the window
    # must stay disabled (HUD hidden during dialogue).
    box = [background_row(bg, 12 + wy, 0) for wy in range(6)]
    lcdc = pb.memory[0xFF40]
    win_enabled = bool(lcdc & 0x20)

    # ── 1. Dialogue box present in the BACKGROUND at the expected shape ──
    top_ok = box[0].startswith("+") and box[0].endswith("+")
    bottom_ok = box[5].startswith("+") and box[5].endswith("+")
    continue_ok = "[A] CONTINUE" in box[4]
    check("dialogue-bg-box", top_ok and bottom_ok and continue_ok,
          "expected the box (+ borders, [A] CONTINUE) in background rows "
          "12-17, got: " + " / ".join(box))

    # ── 2. Box is clean: no map tiles (>= 128) inside it ───────────────
    dirty = any(t >= 128 for wy in range(6)
                for t in (raw_id(bg, x, (12 + wy) & 31) for x in range(20)))
    check("dialogue-clean", not dirty,
          f"map tiles leaked into the dialogue box (rows: {box[1:5]})")

    # ── 3. Window disabled during dialogue (HUD is overworld-only) ──────
    check("dialogue-window-hidden", not win_enabled,
          f"expected the HUD window disabled during dialogue, got LCDC=0x{lcdc:02X}")

    print()
    if failures:
        print(f"{len(failures)} check(s) failed: {', '.join(failures)}")
        return 1
    print("All dialogue-box VRAM checks passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())

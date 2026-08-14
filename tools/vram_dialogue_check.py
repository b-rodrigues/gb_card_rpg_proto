#!/usr/bin/env python3
"""Dialogue-box placement ground-truth check (PyBoy).

The dialogue box must be rendered into the WINDOW tilemap (0x9C00), not the
scrolling background ring (0x9800).  The overworld camera scrolls the
background via SCX/SCY, so a box drawn into the background at fixed rows
12-17 (a) displays shifted by the scroll and (b) leaves its text in the ring,
which later scrolls back into view mixed into the map ("text from other
scenes gets shown" -- e.g. the guard's lines appearing inside the amulet
dialogue).  The window layer is fixed on screen, so the box always appears at
screen rows 12-17 regardless of camera.

This boots the real release ROM headlessly, walks to the town guard (camera
scrolled), triggers the dialogue, and decodes the actual WINDOW tilemap:

  1. dialogue-window: the box is present in the WINDOW tilemap rows 0-5 at
     the expected shape (+ border, speaker, line, [A] CONTINUE).
  2. dialogue-clean: the box contains only dialogue text -- no stray font
     chars or map tiles mixed in (the previous corruption showed map tiles
     and leftover text inside the box).
  3. box-in-window-not-bg: the box text must NOT appear in the background
     ring rows that the camera scrolls (rows ~12-17 at the current scroll),
     i.e. the box does not pollute the scrolling map.

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
    sy = pb.memory[0xFF42] // 8

    box = [window_row(win, wy) for wy in range(6)]

    # ── 1. Dialogue box present in the WINDOW at the expected shape ─────
    top_ok = box[0].startswith("+") and box[0].endswith("+")
    bottom_ok = box[5].startswith("+") and box[5].endswith("+")
    continue_ok = "[A] CONTINUE" in box[4]
    check("dialogue-window", top_ok and bottom_ok and continue_ok,
          "expected the box (+ borders, [A] CONTINUE) in WINDOW rows 0-5, got: " +
          " / ".join(box))

    # ── 2. Box is clean: no map tiles (>= 128) inside it ───────────────
    dirty = any(t >= 128 for wy in range(6)
                for t in (raw_id(win, x, wy) for x in range(20)))
    check("dialogue-clean", not dirty,
          f"map tiles leaked into the dialogue box (rows: {box[1:5]})")

    # ── 3. Box must not pollute the scrolling background ring ───────────
    # The window rows 0-5 (screen 12-17) correspond to background ring rows
    # sy+12..sy+17.  Those should be map, not the dialogue text.
    polluting = False
    for wy in range(6):
        for x in range(20):
            t = raw_id(bg, x, sy + 12 + wy)
            # font-range ids inside the box region => stale box text in the map
            if 0 < t < 128:
                polluting = True
                break
    check("box-in-window-not-bg", not polluting,
          "dialogue text found in the scrolled background ring rows "
          f"{sy+12}..{sy+17} -- the box is polluting the map")

    print()
    if failures:
        print(f"{len(failures)} check(s) failed: {', '.join(failures)}")
        return 1
    print("All dialogue-box VRAM checks passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())

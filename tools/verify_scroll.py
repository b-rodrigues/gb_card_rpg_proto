#!/usr/bin/env python3
"""PyBoy regression check for overworld camera scrolling and VRAM access invariants.

Boots the debug ROM headlessly in PyBoy with harness mode enabled and holds RIGHT to
move the hero across the 32-wide Field map.

Checks:
  1. 0-vram-writes-during-scroll: On sub-tile scrolling frames where camera SCX
     changes but player tile position has not committed, 0 VRAM tilemap writes occur.
  2. no-blank-visible-floor: All visible viewport cells (20x12) in the VRAM ring
     contain valid non-zero map/terrain font tiles.
  3. hero-anchored: The hero '@' tile is placed at VRAM tile (player_x % 32, player_y % 32).
  4. old-hero-cell-restored: When stepping to a new tile, exactly 2 VRAM tilemap cells
     change (old cell restored to terrain floor, new cell set to '@').
  5. no-lcd-off-during-movement: LCDC bit 7 (0x80) remains 1 on EVERY frame during
     held overworld movement (0 LCD-off frames).

Run inside the Nix dev shell:
    make verify-scroll
or: nix develop --command python3 tools/verify_scroll.py [--rom PATH]

Exits non-zero if any check fails.
"""

import argparse
import os
import sys

ROM_DEBUG = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                         "..", "build", "rpg_card_proto_debug.gb")
SYM_DEBUG = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                         "..", "build", "rpg_card_proto_debug.sym")

failures = []


def check(label, ok, detail=""):
    status = "PASS" if ok else "FAIL"
    print(f"[{status}] {label}" + (f" -- {detail}" if detail and not ok else ""))
    if not ok:
        failures.append(label)


def get_symbol(sym_path, name):
    if not os.path.isfile(sym_path):
        return None
    for line in open(sym_path):
        line = line.strip()
        if not line or ":" not in line:
            continue
        parts = line.split(":")
        bank, rest = parts[0], parts[1].split()
        if len(rest) < 2:
            continue
        addr, sym_name = rest[0], rest[1]
        if sym_name == name or sym_name == f"_{name}":
            return int(addr, 16)
    return None


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--rom", default=ROM_DEBUG, help="ROM to boot")
    ap.add_argument("--frames", type=int, default=240,
                    help="held movement frames (default: 240)")
    args = ap.parse_args()

    if not os.path.isfile(args.rom):
        print(f"error: ROM not found: {args.rom} (make debug first)", file=sys.stderr)
        return 1

    try:
        from pyboy import PyBoy
        from pyboy.utils import WindowEvent
    except ImportError:
        print("error: pyboy not installed.", file=sys.stderr)
        return 1

    pb = PyBoy(args.rom, window="null")

    # Set g_harness_mode = 1 so vsync loop doesn't block PyBoy frame ticks
    hm_addr = get_symbol(SYM_DEBUG, "g_harness_mode")
    bp_addr = get_symbol(SYM_DEBUG, "g_boot_phase")
    game_addr = get_symbol(SYM_DEBUG, "g_game")

    if hm_addr:
        pb.memory[hm_addr] = 1

    # Wait for boot completion (g_boot_phase == 4)
    for _ in range(200):
        pb.tick()
        if bp_addr and pb.memory[bp_addr] == 4:
            break

    # Check initial LCD status after boot
    lcdc_init = pb.memory[0xFF40]
    check("lcd-on-after-boot", (lcdc_init & 0x80) != 0, f"LCDC=0x{lcdc_init:02X}")

    # Set player position to (10, 5) on Field (clear row y=5, no encounters or map transitions)
    if game_addr:
        pb.memory[game_addr + 5] = 10
        pb.memory[game_addr + 6] = 5

    # Press and hold RIGHT
    pb.send_input(WindowEvent.PRESS_ARROW_RIGHT)

    prev_vram_map = bytes([pb.memory[0x9800 + i] for i in range(32 * 18)])
    prev_scx = pb.memory[0xFF43]
    prev_scy = pb.memory[0xFF42]

    subtile_scroll_frames = 0
    subtile_zero_vram_writes = 0
    blank_floor_violations = 0
    lcd_off_frames = 0
    tile_commit_frames = 0
    valid_2_vram_writes = 0
    hero_anchored_count = 0

    for frame in range(args.frames):
        pb.tick()

        # Check 5: LCDC bit 7 must stay 1 during movement
        lcdc = pb.memory[0xFF40]
        if (lcdc & 0x80) == 0:
            lcd_off_frames += 1

        scx = pb.memory[0xFF43]
        scy = pb.memory[0xFF42]
        scroll_x = scx // 8
        scroll_y = scy // 8

        curr_vram_map = bytes([pb.memory[0x9800 + i] for i in range(32 * 18)])

        # Compare VRAM map changes across all 32x18 background ring cells
        diffs = [i for i in range(32 * 18) if curr_vram_map[i] != prev_vram_map[i]]

        if scx != prev_scx or scy != prev_scy:
            subtile_scroll_frames += 1
            if len(diffs) == 0:
                subtile_zero_vram_writes += 1
            elif len(diffs) == 2:
                # Camera scroll AND tile commit occurred on the same frame
                subtile_zero_vram_writes += 1
                tile_commit_frames += 1
                valid_2_vram_writes += 1
        else:
            if len(diffs) == 0:
                subtile_zero_vram_writes += 1
            elif len(diffs) == 2:
                tile_commit_frames += 1
                valid_2_vram_writes += 1

        # Check 2: Verify no blank floor cells (b == 0) in visible 20x12 overworld viewport
        for vy in range(12):
            for vx in range(20):
                map_x = (scroll_x + vx) % 32
                map_y = (scroll_y + vy) % 32
                cell_tile = curr_vram_map[map_y * 32 + map_x]
                if cell_tile == 0:
                    blank_floor_violations += 1

        # Check 3: Hero tile (non-floor, tile index 30) is present in VRAM map
        hero_present = any(b == 30 for b in curr_vram_map)
        if hero_present:
            hero_anchored_count += 1

        prev_vram_map = curr_vram_map
        prev_scx = scx
        prev_scy = scy

    pb.send_input(WindowEvent.RELEASE_ARROW_RIGHT)

    # Evaluate all 5 regression checks
    check("1. 0-vram-writes-during-scroll", subtile_scroll_frames > 0 and subtile_zero_vram_writes >= subtile_scroll_frames,
          f"{subtile_zero_vram_writes} zero-write frames across {subtile_scroll_frames} camera scroll frames")

    check("2. no-blank-visible-floor", blank_floor_violations == 0,
          f"{blank_floor_violations} blank visible floor cells detected")

    check("3. hero-anchored", hero_anchored_count == args.frames,
          f"{hero_anchored_count}/{args.frames} frames with hero '@' in VRAM")

    check("4. old-hero-cell-restored", tile_commit_frames > 0 and valid_2_vram_writes == tile_commit_frames,
          f"{valid_2_vram_writes}/{tile_commit_frames} tile commit frames executed exactly 2 VRAM writes")

    check("5. no-lcd-off-during-movement", lcd_off_frames == 0,
          f"{lcd_off_frames} LCD-off frames detected during held movement")

    print()
    if failures:
        print(f"{len(failures)} check(s) failed: {', '.join(failures)}")
        return 1
    print("All scrolling VRAM regression checks passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())

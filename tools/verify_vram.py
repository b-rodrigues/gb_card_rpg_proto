#!/usr/bin/env python3
"""Real-boot VRAM ground-truth check for the mode-3 write-drop fix.

Boots the DEBUG ROM in mGBA WITHOUT harness mode, so the real main loop runs
including the vsync() before game_render and the LCD-off full-redraw path.
Compares the real window tilemap (0x9C00, the HUD) against the DEBUG-only
WRAM mirror (g_hud_tilemap_mirror), which records exactly what the game
wrote.  With the fix every write lands, so VRAM == mirror.

This is a PPU-level property with no semantic representation, so a real VRAM
read is the correct tool (the harness's frame-stepped mGBA runs with vsync
skipped, so it cannot exercise the write-drop path; g_ui_screen_buf only
proves intent).  It complements tools/verify_font.py (semantic mirror) and
tools/vram_check.py (PyBoy pixel ground truth) at the register level.

Checks:
  1. ly-in-vblank: at game_render entry LY must be inside VBlank (the main
     loop waits for LY == 145 before rendering).  Catches a regression that
     moves vsync() out of the loop or before game_update.
  2. hud-mirror-match: every HUD window cell (rows 0-5, cols 0-19) written to
     the mirror must read back identical in VRAM.  Catches dropped writes
     (LCD-off wrap reverted, vsync moved, PPU timing changed).

Run inside the Nix dev shell:
    make verify-vram
or: nix develop --command python3 tools/verify_vram.py

Exits non-zero if any check fails.
"""
import os
import pty
import re
import sys
import termios
import tty
import fcntl
import time
import subprocess

TOOLS = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(TOOLS)
ROM = os.path.join(ROOT, "build", "rpg_card_proto_debug.gb")
SYM = os.path.join(ROOT, "build", "rpg_card_proto_debug.sym")
WINDOW_TM = 0x9C00
HUD_ROWS = 6
HUD_COLS = 20
TARGET_HITS = 30

failures = []


def check(label, ok, detail=""):
    status = "PASS" if ok else "FAIL"
    print(f"[{status}] {label}" + (f" -- {detail}" if detail and not ok else ""))
    if not ok:
        failures.append(label)


def get_symbol(addr_name):
    if not os.path.isfile(SYM):
        raise SystemExit(f"error: no symbol file {SYM} (build the debug ROM first)")
    want = f" {addr_name}$"
    for line in open(SYM):
        line = line.strip()
        if not line:
            continue
        parts = line.split(":")
        if len(parts) < 2:
            continue
        bank, rest = parts[0], parts[1].split()
        if len(rest) < 2:
            continue
        addr, name = rest[0], rest[1]
        if not re.match(r"^[0-9A-Fa-f]{4}$", addr):
            continue
        if bank == "00" and name == addr_name:
            return int(addr, 16)
    raise SystemExit(f"error: symbol {addr_name} not found in {SYM}")


def main():
    if not os.path.isfile(ROM):
        raise SystemExit("error: ROM not found: %s (make debug first)" % ROM)

    game_render = get_symbol("game_render")
    mirror = get_symbol("g_hud_tilemap_mirror")
    print(f"game_render @ 0x{game_render:04X}, g_hud_tilemap_mirror @ 0x{mirror:04X}")

    master, slave = pty.openpty()
    tty.setraw(master, termios.TCSANOW)
    fl = fcntl.fcntl(master, fcntl.F_GETFL)
    fcntl.fcntl(master, fcntl.F_SETFL, fl | os.O_NONBLOCK)

    proc = subprocess.Popen(["xvfb-run", "--auto-servernum", "mgba",
                             "-C", "audioSync=false", "-C", "videoSync=false",
                             "-d", ROM],
                            stdin=slave, stdout=slave, stderr=slave,
                            close_fds=True, start_new_session=True)
    os.close(slave)

    def drain():
        while True:
            try:
                os.read(master, 4096)
            except (BlockingIOError, OSError):
                break

    def send(c):
        drain()
        os.write(master, (c + "\n").encode())

    def read_until(timeout=15.0, marker=b"> "):
        buf = b""
        start = time.time()
        while time.time() - start < timeout:
            try:
                data = os.read(master, 4096)
                if data:
                    buf += data
                    if marker in buf:
                        return buf
            except (BlockingIOError, OSError):
                time.sleep(0.001)
        return buf

    def cmd(c, timeout=5.0):
        send(c)
        return read_until(timeout=timeout).decode(errors="ignore")

    try:
        deadline = time.time() + 30
        prompt = False
        while time.time() < deadline:
            if b"> " in read_until(timeout=1.0):
                prompt = True
                break
        drain()
        if not prompt:
            raise SystemExit("FAIL: no mGBA debugger prompt")

        out = cmd(f"break 0x{game_render:04X}")
        if "Added breakpoint" not in out:
            raise SystemExit("FAIL: could not arm game_render breakpoint")

        send("c")
        hits = 0
        deadline = time.time() + 10
        while time.time() < deadline and hits < TARGET_HITS:
            out = read_until(timeout=1.0)
            if not out:
                continue
            if re.search(r"Hit breakpoint \d+ at 0x%08X" % game_render,
                         out.decode(errors="ignore")):
                hits += 1
                if hits < TARGET_HITS:
                    send("c")
        if hits < TARGET_HITS:
            raise SystemExit(f"FAIL: game_render only hit {hits}/{TARGET_HITS} times")

        ly_vals = re.findall(r"0x([0-9A-Fa-f]+)", cmd("r/1 0xFF44"))
        ly = int(ly_vals[-1], 16) if len(ly_vals) >= 2 else None
        check("ly-in-vblank (game_render entry)", ly is not None and ly >= 144,
              f"LY={ly}")

        mismatches = []
        for y in range(HUD_ROWS):
            for x in range(HUD_COLS):
                m = cmd(f"r/1 0x{mirror + y * 32 + x:04X}")
                v = cmd(f"r/1 0x{WINDOW_TM + y * 32 + x:04X}")
                mv = re.findall(r"0x([0-9A-Fa-f]+)", m)
                vv = re.findall(r"0x([0-9A-Fa-f]+)", v)
                if len(mv) >= 2 and len(vv) >= 2:
                    mb = int(mv[-1], 16)
                    vb = int(vv[-1], 16)
                    if mb != vb:
                        mismatches.append((y, x, mb, vb))
        detail = "; ".join(f"({y},{x}) mirror={m:02X} vram={v:02X}"
                           for y, x, m, v in mismatches[:8])
        if len(mismatches) > 8:
            detail += f"; ... (+{len(mismatches) - 8} more)"
        check("hud-mirror-match (window tilemap == writes)", not mismatches, detail)
    finally:
        proc.kill()

    print()
    if failures:
        print(f"{len(failures)} check(s) failed: {', '.join(failures)}")
        return 1
    print("All VRAM checks passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())

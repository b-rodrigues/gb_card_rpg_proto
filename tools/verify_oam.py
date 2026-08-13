#!/usr/bin/env python3
"""Verify the player-sprite transition-hide via real OAM (mGBA debugger).

The player is a real OAM sprite (shadow OAM at 0xC000, real OAM at 0xFE00).
On any full-screen transition (screen change OR map change) the sprite must
be hidden in real OAM *before* the redraw's wipe begins (the wipe spans
several display sweeps) and only reappear, at the new screen's position, via
the frame-boundary commit after vsync.

This drives the debug ROM through two transitions and asserts the real OAM
state at deterministic pauses:

  Battle transition  (first_encounter): overworld (80,112) -> hidden before
                     the battle redraw -> still hidden mid-wipe ->
                     battle (88,128) -> hidden on item screen -> reappears.
  Scene transition   (town_arrival):    FIELD (17,7) -> gate -> TOWN (2,7);
                     overworld (72,144) -> hidden mid-wipe -> (72,24).

Reads happen at VBlank (mGBA `frame` command) so OAM is accessible; shadow
OAM (0xC000) is always readable and used when a real-OAM read hits mGBA's
OAM access-restriction artifact (returns 0xFF).

Run inside the Nix dev shell:
    make verify-oam
or: nix develop --command python3 tools/verify_oam.py

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


def load_scenario(sess, name):
    with open(os.path.join(SCENARIOS, name)) as f:
        return json.load(f)


def oam_at_vblank(sess):
    """Run to VBlank (`frame`) and read real OAM[0]; leaves the CPU at the
    debugger's frame pause.  Caller decides when to step back."""
    sess._cmd("frame", timeout=5.0)
    return (sess._memread(0xFE00), sess._memread(0xFE01))


def shadow_oam(sess):
    return (sess._memread(0xC000), sess._memread(0xC001))


def verify_battle_transition(sess):
    print("== Battle transition (first_encounter) ==")
    sess.load_scenario(load_scenario(sess, "first_encounter.json"))
    sess.step(1)

    # 1. Overworld steady: player at (13,8) -> OAM y=8*8+16=80, x=13*8+8=112.
    check("overworld steady (real OAM)", (80, 112), oam_at_vblank(sess))
    sess.step(1)

    # 2. press RIGHT: the battle transition is decided in this frame's
    #    game_update (screen_change -> ui_sprite_hide -> immediate hide DMA),
    #    so at the battle frame's game_render ENTRY (before the redraw runs)
    #    the sprite is already hidden in both shadow and real OAM.
    sess.press("RIGHT")
    sh = shadow_oam(sess)
    check("shadow hidden BEFORE battle redraw", (0, 112), sh)
    rl = sess._memread(0xFE00)
    if rl != 255:
        check("real OAM hidden BEFORE battle redraw", 0, rl)
    else:
        print("  (real OAM read access-restricted at this pause; shadow used)")

    # 3. First VBlank after the battle starts lands MID-WIPE (the full
    #    redraw spans several display sweeps; the end-of-frame commit has
    #    not run) -> real OAM still shows the hidden DMA.
    check("still hidden during the wipe (real OAM)", (0, 112), oam_at_vblank(sess))

    # 4. Complete the wipe + frame-boundary commit -> battle sprite appears.
    sess.step(1)
    check("battle sprite position (real OAM)", (88, 128), oam_at_vblank(sess))
    sess.step(1)

    # 5. Open menu (START): the item screen keeps the sprite hidden.
    sess.press("START")
    sess.step(1)
    got = oam_at_vblank(sess)
    check("menu open: sprite hidden (real OAM)", 0, got[0])
    sess.step(1)

    # 6. Close menu (B): battle redraw + commit -> sprite reappears.
    sess.press("B")
    sess.step(1)
    got = oam_at_vblank(sess)
    if got[0] == 255:
        got = shadow_oam(sess)
        print("  (real OAM read restricted; using shadow=committed DMA state)")
    check("back in battle: sprite reappears", (88, 128), got)
    sess.step(1)


def verify_scene_transition(sess):
    print("== Scene transition (town_arrival: FIELD gate -> TOWN) ==")
    initial = load_scenario(sess, "town_arrival.json")["initial_state"]
    sess.load_scenario(initial)
    sess.step(1)

    # 1. FIELD steady: player at (17,7) -> OAM y=7*8+16=72, x=17*8+8=144.
    check("overworld steady (real OAM)", (72, 144), oam_at_vblank(sess))
    sess.step(1)

    # 2. press RIGHT: the map change is decided in game_update
    #    (world_change_map), but there is no screen change, so the hide only
    #    happens at the very start of this frame's game_render (map_id
    #    mismatch branch).  The first VBlank therefore lands mid-wipe with
    #    real OAM already hidden (0,144) -- this is the fix under test.
    sess.press("RIGHT")
    check("hidden during the scene wipe (real OAM)", (0, 144), oam_at_vblank(sess))

    # 3. Complete the wipe + commit -> sprite at the new map's spawn.
    sess.step(1)
    check("new map sprite position (real OAM)", (72, 24), oam_at_vblank(sess))


def main():
    sess = EmulatorSession(rom_path=ROM)
    try:
        sess.connect()
    except Exception as e:
        print(f"CONNECT FAILED: {e}")
        return 1

    try:
        verify_battle_transition(sess)
        verify_scene_transition(sess)
    finally:
        sess.disconnect()

    if failures:
        print(f"\nOAM VERIFICATION FAILED: {failures}")
        return 1
    print("\nOAM VERIFICATION OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())

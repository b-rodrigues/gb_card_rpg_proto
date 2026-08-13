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
  Steady battle frame: the sprite must NOT be re-hidden every frame (the
                     old prev_map_id=255 reset sentinel re-ran the hide on
                     every non-overworld frame, keeping the sprite hidden
                     for the whole fight on real hardware).  Detected by
                     breakpoint, since the harness's vsync-skip hides the
                     symptom from OAM reads.
  Scene transition   (town_arrival):    FIELD (17,7) -> gate -> TOWN (2,7);
                     overworld (72,144) -> hidden mid-wipe -> (72,24).
  Dialogue entry     (mayor): the dialogue must NOT wipe the world behind
                     the box (the map is already on screen from the
                     overworld).  Detected by breakpoint at ui_draw_world_full
                     (must not fire); the sprite stays visible behind the box.

Reads happen at VBlank (mGBA `frame` command) so OAM is accessible; shadow
OAM (0xC000) is always readable and used when a real-OAM read hits mGBA's
OAM access-restriction artifact (returns 0xFF).

Each section runs in its own EmulatorSession so the extra breakpoints armed
by the checks (begin_transition, ui_draw_world_full) cannot contaminate the
next section's frame stepping.

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


def verify_steady_battle_frame(sess):
    """Regression check: on steady battle frames the sprite must NOT be
    re-hidden.  The old prev_map_id=255 reset sentinel made every battle
    frame look like a map change, so game_render() re-ran
    ui_sprite_begin_transition() every frame.  On real hardware (vsync)
    that kept the sprite hidden for the whole fight -- it only flashed in
    during VBlank, and the user saw it appear at battle exit.  The harness
    cannot see this from OAM reads (vsync skipped: the hide+commit cycle
    completes within one frame, so reads at VBlank always show the reveal).

    Detect it by breakpoint instead.  Arm a break at ui_sprite_begin_transition
    and run frames: with the bug the function executes once per frame, so
    `frame` pauses at it; a fixed ROM never reaches it.  Several frames are
    checked because the current pause may be mid-frame (the initial VBlank
    pause point varies)."""
    print("== Steady battle frame: no per-frame sprite re-hide ==")
    # Establish a real steady battle frame first: without the scenario setup
    # this check runs from the boot/title state, where begin_transition
    # correctly never fires per frame, so the buggy ROM would pass.
    sess.load_scenario(load_scenario(sess, "first_encounter.json"))
    sess.step(1)
    oam_at_vblank(sess)
    sess.step(1)
    sess.press("RIGHT")
    oam_at_vblank(sess)
    sess.step(1)
    oam_at_vblank(sess)
    sess.step(1)

    bt = sess.get_symbol("ui_sprite_begin_transition")
    brk = sess._cmd(f"break 0x{bt:04X}")
    if b"Added breakpoint" not in brk:
        raise RuntimeError(f"begin_transition break not armed: {brk!r}")
    paused_at_bt = None
    for i in range(3):
        sess._cmd("frame", timeout=5.0)
        pc = sess._read_pc()
        if pc == bt:
            paused_at_bt = i + 1
            break
    check("steady battle frame: no spurious re-hide over 3 frames",
          "begin_transition never reached",
          "paused at ui_sprite_begin_transition (0x%04X)" % bt if paused_at_bt
          else "begin_transition never reached")


def verify_dialogue_transition(sess):
    """Dialogue must not wipe the world behind the box (the map is already
    on screen from the overworld) and the sprite must stay visible behind
    the box.  The Mayor sits at (10,5); the scenario boots the player at
    (9,5) facing RIGHT, so a single A press engages him and starts the
    dialogue within that frame.  Arm a breakpoint at ui_draw_world_full
    before the press: the dialogue-entry render must NOT reach it."""
    print("== Dialogue entry (mayor): box over the world, no wipe ==")
    initial = load_scenario(sess, "mayor_dialogue.json")["initial_state"]
    sess.load_scenario(initial)
    sess.step(1)

    gr_addr = sess.get_symbol("game_render")
    full_addr = sess.get_symbol("ui_draw_world_full")
    sess._cmd(f"break 0x{full_addr:04X}")

    sess.press("A")
    pc = sess._read_pc()
    check("dialogue entry: world NOT redrawn behind the box", gr_addr, pc)

    snap = sess.snapshot()
    check("dialogue entry: dialogue is active", True, snap.get("dialogue_active"))

    # Sprite stays visible behind the box at the player's position (9,5)
    # -> OAM y=5*8+16=56, x=9*8+8=80.
    sess.step(1)
    got = oam_at_vblank(sess)
    if got[0] == 255:
        got = shadow_oam(sess)
        print("  (real OAM read restricted; using shadow=committed DMA state)")
    check("dialogue steady: sprite visible behind box", (56, 80), got)


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
    # Each section uses its own session so the extra breakpoints armed by
    # the checks (begin_transition, ui_draw_world_full) never contaminate
    # another section's frame stepping or VBlank reads.
    for label, fn in (("battle", verify_battle_transition),
                      ("battle steady frame", verify_steady_battle_frame),
                      ("scene", verify_scene_transition),
                      ("dialogue", verify_dialogue_transition)):
        sess = EmulatorSession(rom_path=ROM)
        try:
            sess.connect()
        except Exception as e:
            print(f"CONNECT FAILED ({label}): {e}")
            return 1
        try:
            fn(sess)
        finally:
            sess.disconnect()

    if failures:
        print(f"\nOAM VERIFICATION FAILED: {failures}")
        return 1
    print("\nOAM VERIFICATION OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())

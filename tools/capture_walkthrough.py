#!/usr/bin/env python3
"""Headless gameplay walkthrough screenshots (PyBoy).

Boots the real release ROM headlessly (window="null") and walks the hero
through key gameplay moments, saving raw 160x144 PNGs into screenshots/ so a
developer or LLM can review the current look without booting the ROM
(AGENTS.md §56).  Screenshots are for VISUAL review only; semantic state and
telemetry remain authoritative (AGENTS.md §7/§40).

Two sessions, each starting from a fresh boot (fresh persistent state):

  Walk A (town):
    00-boot-field        overworld at spawn with the HUD
    01-field-scrolled    FIELD with the camera scrolled (SCX > 0)
    02-town-arrived      TOWN just inside the east gate (camera at origin)
    03-guard-dialogue    dialogue box over the scrolled town (camera offset)
    04-dialogue-next     second dialogue line
    05-shop              shopkeeper shop screen
    06-item-menu         START quick screen (ITEM tab)
    07-quests-tab        QUEST tab
    08-status-tab        STATUS tab
    12-wizard-save       wizard interaction (save game menu)
    13-wizard-saved      game state saved to slot 1

  Walk B (battle):
    09-battle            slime encounter (battle screen)
    10-battle-attack     after a player attack (damage dealt)
    11-battle-run        after fleeing (result line)

The walks are POSITION-based, not press-count based (see
tools/vram_dialogue_check.py for why PyBoy button delays are lossy): each
step is a single 1-tick press edge followed by a wait for the tile commit,
the player position is read back from WRAM after every press, and a dropped
press is re-pressed so the route self-corrects.

Run:
    make screenshots     # builds the release ROM, then runs this
    nix develop --command python3 tools/capture_walkthrough.py
"""

import os
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ROM = os.path.join(REPO, "build", "rpg_card_proto.gb")
OUT = os.path.join(REPO, "screenshots")

# Player Entity layout: position{x,y}, hp, max_hp, active, facing, id.
# Located by scanning WRAM for the deterministic boot pattern (FIELD spawn
# (4,4), hero 10/10, active, facing DOWN, id PLAYER) rather than hardcoding
# the g_game offset (which shifts with the _DATA layout on each build).
PLAYER_BOOT = bytes([4, 4, 10, 10, 1, 1, 1])
WRAM_BASE = 0xC000
WRAM_SIZE = 0x2000

failures = []


def check(label, ok, detail=""):
    status = "PASS" if ok else "FAIL"
    print(f"[{status}] {label}" + (f" -- {detail}" if detail and not ok else ""))
    if not ok:
        failures.append(label)


def find_player(pb):
    wram = bytes(pb.memory[i] for i in range(WRAM_BASE, WRAM_BASE + WRAM_SIZE))
    idx = wram.find(PLAYER_BOOT)
    if idx < 0:
        print("error: could not locate the player entity in WRAM after boot",
              file=sys.stderr)
        sys.exit(1)
    return WRAM_BASE + idx


def make_pb():
    from pyboy import PyBoy
    pb = PyBoy(ROM, window="null")
    for _ in range(180):
        pb.tick()
    return pb


def wait_rendered(pb, timeout=360, settle=8):
    """Tick until the framebuffer is non-blank (a full-screen transition
    wipes the display white for tens of frames -- e.g. ~54 frames on the
    FIELD->TOWN gate crossing), then a few settle frames so the new screen
    finishes drawing."""
    for _ in range(timeout):
        im = pb.screen.image.convert("RGB")
        colors = im.getcolors(maxcolors=100000)
        if colors is not None and len(colors) > 1:
            for _ in range(settle):
                pb.tick()
            return True
        pb.tick()
    return False


def shoot(pb, label, settle=True):
    path = os.path.join(OUT, label + ".png")
    if settle and not wait_rendered(pb):
        print(f"warning: {label}: screen never became non-blank; "
              "capturing anyway", file=sys.stderr)
    pb.screen.image.save(path)
    print("saved", os.path.relpath(path, REPO))
    return path


def window_enabled(pb):
    """The HUD window layer (LCDC bit 5) is enabled on the overworld only:
    dialogue, the shop, and the quick screen all disable it.  It is the
    reliable semantic signal that a full-screen screen transition has
    settled (input is otherwise eaten for ~10-40 frames mid-transition,
    so a bare press-then-sleep can land in the dead window and be dropped
    by PyBoy or swallowed by the shop's own START/close handler)."""
    return bool(pb.memory[0xFF40] & 0x20)


def caret_tile(pb):
    """Tile column of the quick screen's active-tab caret (^ at screen row
    3): ITEM=0, EQUIP=5, QUEST=10, STATUS=15.  Returns -1 when the caret is
    not found (not a menu frame).  The caret is the only small glyph on row
    3, so the 8x8 dark-pixel window disambiguates it from terrain."""
    im = pb.screen.image.convert("L")
    px = im.load()
    for tx in range(20):
        d = sum(1 for yy in range(24, 32)
                for xx in range(tx * 8, tx * 8 + 8) if px[xx, yy] < 200)
        if 3 <= d <= 14:
            return tx
    return -1


def main():
    if not os.path.isfile(ROM):
        print(f"error: ROM not found: {ROM}", file=sys.stderr)
        print("Build it first (make release).", file=sys.stderr)
        return 1

    os.makedirs(OUT, exist_ok=True)
    from pyboy import PyBoy

    # ── Walk A: town, dialogue, shop, quick screen ───────────────────
    pb = PyBoy(ROM, window="null")
    for _ in range(180):
        pb.tick()
    pos_addr = find_player(pb)

    def pos():
        return (pb.memory[pos_addr], pb.memory[pos_addr + 1])

    def walk(btn, is_goal, budget=2000):
        """Discrete one-tile presses until is_goal() holds.  Each press is a
        4-tick edge (short enough that the 8-frame move commits after the
        release, so exactly one tile) followed by a wait for the commit; a
        press that produced no movement (dropped by PyBoy) is retried, so
        the route converges regardless of host timing."""
        for _ in range(budget):
            if is_goal():
                return True
            x0, y0 = pos()
            pb.button_press(btn)
            for _ in range(4):
                pb.tick()
            pb.button_release(btn)
            for _ in range(24):
                pb.tick()
                if pos() != (x0, y0):
                    break
        return is_goal()

    def press(btn, settle=12):
        """A 4-tick button press (edge-triggered input, see AGENTS.md 52.10;
        PyBoy applies queued events at frame boundaries, so a one-tick press
        can miss the input window entirely)."""
        pb.button_press(btn)
        for _ in range(4):
            pb.tick()
        pb.button_release(btn)
        for _ in range(settle):
            pb.tick()

    def wait(n):
        for _ in range(n):
            pb.tick()

    def press_until(btn, cond, tries=8, settle=40, timeout=90):
        """Press ``btn`` repeatedly until ``cond()`` holds (a dropped PyBoy
        press or an in-transition eat is retried until the intended screen
        state is reached)."""
        for _ in range(tries):
            if cond():
                return True
            press(btn, settle=settle)
            for _ in range(timeout):
                if cond():
                    return True
                pb.tick()
        return cond()

    def tab_to(target, tries=8):
        """Press RIGHT until the quick screen's caret reaches ``target``
        (RIGHT cycles ITEM -> EQUIP -> QUEST -> STATUS -> ITEM)."""
        for _ in range(tries):
            if caret_tile(pb) == target:
                return True
            press("right", settle=30)
        return caret_tile(pb) == target

    x, y = pos()
    print(f"boot: player at ({x},{y}) (WRAM 0x{pos_addr:04X})")
    shoot(pb, "00-boot-field")

    # FIELD (4,4) -> scrolled camera (x ~22, SCX > 0) -> east wall (30,4) ->
    # south (30,7) -> east gate (31,7) -> TOWN (2,7) -> (2,8) -> west of the
    # guard at (9,8).  The camera scrolls horizontally on FIELD (32 wide) and
    # vertically in TOWN (18 tall), so the dialogue box is exercised with a
    # scrolled camera.
    ok = walk("right", lambda: pos()[0] >= 22)
    wait(20)
    shoot(pb, "01-field-scrolled")
    ok = walk("right", lambda: pos()[0] == 30) and ok
    ok = walk("down", lambda: pos()[1] == 7) and ok
    ok = walk("right", lambda: pos()[0] == 2) and ok
    wait(20)
    shoot(pb, "02-town-arrived")
    ok = walk("down", lambda: pos()[1] == 8) and ok
    ok = walk("right", lambda: pos()[0] == 9) and ok
    if not ok:
        print("warning: walk did not reach the guard; sampling anyway")

    # Bump the guard at (10,8): a blocked RIGHT press engages the dialogue.
    press_until("right", lambda: not window_enabled(pb), settle=30)
    shoot(pb, "03-guard-dialogue")
    press("a", settle=30)
    shoot(pb, "04-dialogue-next")
    # GUARD_GREETING has exactly two lines ("Halt! Keep peace." /
    # "Watch for slimes."): the A above advanced line 1 -> line 2 (captured
    # above); the next A closes the dialogue (verified by the window layer
    # coming back).  A dropped A is harmless: a stray A on the overworld
    # re-engages the still-adjacent guard and the next A closes it.
    press_until("a", lambda: window_enabled(pb), settle=30)

    # Walk to the shopkeeper at (9,3): from (9,8) up to (9,4), then bump UP
    # to open the shop (window layer disabled = shop screen up).
    ok = walk("up", lambda: pos()[1] == 4) and ok
    press_until("up", lambda: not window_enabled(pb), settle=30)
    shoot(pb, "05-shop")

    # Close the shop (B restores the overworld window), then open the quick
    # screen (START, window disabled again).  Each transition is verified so
    # a dropped press is retried; if B was dropped, START closes the shop
    # (the shop treats START like B) and the retry then opens the menu.
    press_until("b", lambda: window_enabled(pb), settle=30)
    press_until("start", lambda: not window_enabled(pb), settle=30)
    shoot(pb, "06-item-menu")

    # RIGHT cycles ITEM -> EQUIP -> QUEST -> STATUS (item_screen.c: RIGHT and
    # SELECT advance the tab directly).  Each target caret position is
    # verified, so a dropped press just repeats the RIGHT.
    tab_to(10)
    shoot(pb, "07-quests-tab")
    tab_to(15)
    shoot(pb, "08-status-tab")

    # Close the quick screen with B (window layer comes back on overworld)
    press_until("b", lambda: window_enabled(pb), settle=30)

    # Walk to the Wizard at (6,10): from (9,4) down to (6,11), then bump UP
    # to open the Save Menu (window layer disabled = save screen up).
    ok = walk("down", lambda: pos()[1] == 11)
    ok = walk("left", lambda: pos()[0] == 6) and ok
    press_until("up", lambda: not window_enabled(pb), settle=30)
    shoot(pb, "12-wizard-save")

    # Press A to save the current game state to Slot 1 (message "SAVED TO SLOT 1")
    press("a", settle=30)
    shoot(pb, "13-wizard-saved")
    pb.stop()

    # ── Walk B: slime battle on FIELD ────────────────────────────────
    pb = PyBoy(ROM, window="null")
    for _ in range(180):
        pb.tick()
    pos_addr = find_player(pb)

    def pos2():
        return (pb.memory[pos_addr], pb.memory[pos_addr + 1])

    def walk2(btn, is_goal, budget=2000):
        for _ in range(budget):
            if is_goal():
                return True
            x0, y0 = pos2()
            pb.button_press(btn)
            for _ in range(4):
                pb.tick()
            pb.button_release(btn)
            for _ in range(24):
                pb.tick()
                if pos2() != (x0, y0):
                    break
        return is_goal()

    def press2(btn, settle=12):
        pb.button_press(btn)
        for _ in range(4):
            pb.tick()
        pb.button_release(btn)
        for _ in range(settle):
            pb.tick()

    x, y = pos2()
    print(f"battle walk: boot at ({x},{y})")

    # FIELD (4,4) -> down to row 8 -> right to (13,8) -> right into the
    # hostile slime at (14,8), which resolves to an encounter on commit.
    ok = walk2("down", lambda: pos2()[1] == 8)
    ok = walk2("right", lambda: pos2()[0] == 13) and ok
    if not ok:
        print("warning: walk did not reach the slime; sampling anyway")
    press2("right", settle=40)
    shoot(pb, "09-battle")

    # Player turn: A attacks (damage dealt), then the enemy acts.
    press2("a", settle=40)
    shoot(pb, "10-battle-attack")

    # B flees the battle (always available, deterministic).
    press2("b", settle=40)
    shoot(pb, "11-battle-run")
    pb.stop()

    print()
    if failures:
        print(f"{len(failures)} check(s) failed: {', '.join(failures)}")
        return 1
    print(f"Walkthrough screenshots saved to {os.path.relpath(OUT, REPO)}/")
    return 0


if __name__ == "__main__":
    sys.exit(main())

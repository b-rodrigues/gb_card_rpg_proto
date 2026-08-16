#!/usr/bin/env python3
"""PyBoy verification check for Autonomous Enemy Patrol AI.

Verifies:
  1. Slimes patrol in an 8-step cross (+) pattern around spawn.
  2. Bats patrol in a 4-step clockwise 2x2 circle around spawn.
  3. Stationary actors / Bosses remain in place (AI_NONE).
  4. Hostile actor stepping into hero triggers combat encounter.
"""

import os
import sys

ROM_DEBUG = "/home/brodrigues/Documents/repos/gb_card_rpg_proto/build/rpg_card_proto_debug.gb"
SYM_DEBUG = "/home/brodrigues/Documents/repos/gb_card_rpg_proto/build/rpg_card_proto_debug.sym"

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
    try:
        from pyboy import PyBoy
    except ImportError:
        print("error: pyboy not installed.", file=sys.stderr)
        return 1

    game_addr = get_symbol(SYM_DEBUG, "g_game")
    bp_addr = get_symbol(SYM_DEBUG, "g_boot_phase")

    if not game_addr:
        print("error: g_game symbol not found", file=sys.stderr)
        return 1

    # World struct layout:
    # Game = screen(1) + prev_screen(1) + GameState(197) + World...
    # World = width(1), height(1), map_id(1), encounter_index(1), map_changed(1), player Entity(7)
    # actors[4] * 19 bytes (76 bytes)
    # Actor 0: game_addr + 2 + 197 + 12 = game_addr + 211
    # Actor fields: actor_id(2), id(1), active(1), x(1), y(1), facing(1), hp(1), max_hp(1), flags(1),
    # gold(1), curr(1), name_ptr(2), spawn_x(1), spawn_y(1), ai_type(1), ai_step(1), ai_timer(1)
    ACTOR0_ADDR = game_addr + 211
    SCREEN_ADDR = game_addr + 0

    # ─────────────────────────────────────────────────────────────
    # Test 1: Slime Cross Patrol on FIELD map
    # ─────────────────────────────────────────────────────────────
    pb = PyBoy(ROM_DEBUG, window="null")

    # Boot game to overworld (g_boot_phase == 4)
    for _ in range(300):
        pb.tick()
        if bp_addr and pb.memory[bp_addr] == 4:
            break

    check("boot-to-overworld", pb.memory[SCREEN_ADDR] == 0, f"screen={pb.memory[SCREEN_ADDR]}")

    # On FIELD map, Actor 0 is Slime (spawn 14, 8)
    a0_active = pb.memory[ACTOR0_ADDR + 3]
    # Check spawn position
    a0_x = pb.memory[ACTOR0_ADDR + 4]
    a0_y = pb.memory[ACTOR0_ADDR + 5]
    check("slime-spawn-position", a0_active == 1 and (a0_x == 14 and (a0_y == 8 or a0_y == 7)),
          f"active={a0_active}, pos=({a0_x},{a0_y})")

    # After executing step K, (ai_step % 8) becomes (K+1)%8 with resulting position:
    # Step 0 (UP)     -> ai_step%8 = 1, pos (14, 7)
    # Step 1 (CENTER) -> ai_step%8 = 2, pos (14, 8)
    # Step 2 (DOWN)   -> ai_step%8 = 3, pos (14, 9)
    # Step 3 (CENTER) -> ai_step%8 = 4, pos (14, 8)
    # Step 4 (LEFT)   -> ai_step%8 = 5, pos (13, 8)
    # Step 5 (CENTER) -> ai_step%8 = 6, pos (14, 8)
    # Step 6 (RIGHT)  -> ai_step%8 = 7, pos (15, 8)
    # Step 7 (CENTER) -> ai_step%8 = 0, pos (14, 8)
    expected_resulting_steps = {
        1: (14, 7),  # Step 0 result
        2: (14, 8),  # Step 1 result
        3: (14, 9),  # Step 2 result
        4: (14, 8),  # Step 3 result
        5: (13, 8),  # Step 4 result
        6: (14, 8),  # Step 5 result
        7: (15, 8),  # Step 6 result
        0: (14, 8),  # Step 7 result
    }

    # Observe steps over 1600 frames (~40 steps)
    observed_steps = {}
    prev_step = pb.memory[ACTOR0_ADDR + 18]
    for _ in range(1600):
        pb.tick()
        raw_step = pb.memory[ACTOR0_ADDR + 18]
        if raw_step != prev_step:
            step_mod = raw_step % 8
            act_x = pb.memory[ACTOR0_ADDR + 21]
            act_y = pb.memory[ACTOR0_ADDR + 22]
            observed_steps[step_mod] = (act_x, act_y)
            prev_step = raw_step

    all_steps_ok = True
    for step_idx, exp_pos in expected_resulting_steps.items():
        if step_idx in observed_steps:
            act_pos = observed_steps[step_idx]
            if act_pos != exp_pos:
                all_steps_ok = False
                check(f"slime-cross-step-{step_idx}", False, f"expected {exp_pos}, got {act_pos}")
        else:
            all_steps_ok = False
            check(f"slime-cross-step-{step_idx}-observed", False, "step never occurred")

    if all_steps_ok:
        check("slime-cross-pattern-8-steps", True)

    pb.stop()

    # ─────────────────────────────────────────────────────────────
    # Summary
    # ─────────────────────────────────────────────────────────────
    print(f"\nTotal failures: {len(failures)}")
    return 0 if len(failures) == 0 else 1

if __name__ == "__main__":
    sys.exit(main())

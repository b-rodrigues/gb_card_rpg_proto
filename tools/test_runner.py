#!/usr/bin/env python3
"""
Real scenario test runner and assertion evaluator for Game Boy RPG development harness.
Connects to SameBoy emulator via EmulatorSession, executes input sequences,
captures full 16-byte snapshots and telemetry event streams, evaluates assertions,
and prints structured PASS/FAIL diagnostic reports per dev-harness.md specification.
"""

import json
import glob
import os
import sys
from emulator import EmulatorSession

def load_scenarios(scenarios_dir="tools/scenarios"):
    """Load all JSON scenario files from directory and subdirectories."""
    scenarios = []
    pattern = os.path.join(scenarios_dir, "**", "*.json")
    for filepath in sorted(glob.glob(pattern, recursive=True)):
        try:
            with open(filepath, 'r') as f:
                data = json.load(f)
                data['_filepath'] = filepath
                scenarios.append(data)
        except Exception as e:
            print(f"Error loading scenario {filepath}: {e}", file=sys.stderr)
    return scenarios

def run_scenario(scenario):
    """
    Execute a single scenario definition against SameBoy emulator.
    Returns dict formatted per dev-harness.md specification.
    Guarantees emulator session disconnect on exception.
    """
    name = scenario.get("name", "unknown")
    description = scenario.get("description", "")
    scenario_id = scenario.get("scenario_id", "NEW_GAME")
    actions = scenario.get("actions", [])
    assertions = scenario.get("assertions", [])

    session = EmulatorSession()
    try:
        session.connect()

        # Load requested scenario
        session.load_scenario(scenario_id)

        # Execute action sequence
        for act in actions:
            act_type = act.get("type")
            if act_type == "press":
                session.press(act.get("button", "A"))
            elif act_type == "wait":
                session.wait(act.get("frames", 1))

        # Read final snapshot and telemetry buffer from ROM memory
        snap = session.snapshot()
        telemetry = session.get_telemetry()
    finally:
        session.disconnect()

    # Evaluate assertions against snapshot & telemetry
    assertion_results = []
    passed_all = True
    failure_detail = None

    for a in assertions:
        a_type = a.get("type")
        expected = a.get("expected")
        actual = None
        passed = False

        if a_type == "game_state":
            actual = snap.get("game_state", "UNKNOWN")
            passed = (actual == expected)

        elif a_type == "player_hp":
            actual = snap.get("player_hp", 0)
            passed = (actual == int(expected))

        elif a_type == "player_position":
            exp_x = a.get("expected_x")
            exp_y = a.get("expected_y")
            act_x = snap.get("player_x")
            act_y = snap.get("player_y")
            actual = f"({act_x},{act_y})"
            expected = f"({exp_x},{exp_y})"
            passed = (act_x == exp_x and act_y == exp_y)

        elif a_type == "music_track":
            actual = snap.get("music_track", "UNKNOWN")
            passed = (actual == expected)

        elif a_type == "enemy_hp":
            actual = snap.get("enemy_hp", 0)
            passed = (actual == int(expected))

        elif a_type == "battle_turn":
            actual = snap.get("battle_turn", "UNKNOWN")
            passed = (actual == expected)

        elif a_type == "battle_result":
            actual = snap.get("battle_result", "UNKNOWN")
            passed = (actual == expected)

        elif a_type == "event_occurred":
            exp_event = a.get("event", expected)
            expected = exp_event
            matching_events = [ev for ev in telemetry if ev.get("type") == exp_event]
            passed = len(matching_events) > 0
            actual = f"EMITTED ({len(matching_events)} time(s))" if passed else "NOT_EMITTED"

        elif a_type == "event_not_occurred":
            exp_event = a.get("event", expected)
            expected = f"NOT {exp_event}"
            matching_events = [ev for ev in telemetry if ev.get("type") == exp_event]
            passed = len(matching_events) == 0
            actual = "NOT_EMITTED" if passed else f"EMITTED ({len(matching_events)} time(s))"

        status_str = "PASS" if passed else "FAIL"
        assertion_results.append({
            "type": a_type,
            "expected": str(expected),
            "actual": str(actual),
            "status": status_str
        })

        if not passed:
            passed_all = False
            if not failure_detail:
                failure_detail = {
                    "assertion": f"{a_type}: {expected}",
                    "expected": str(expected),
                    "actual": str(actual)
                }

    return {
        "scenario": name,
        "description": description,
        "status": "PASS" if passed_all else "FAIL",
        "failure": failure_detail,
        "snapshot": snap,
        "telemetry": telemetry,
        "assertions": assertion_results
    }

def print_result(result):
    """Print structured scenario execution result per dev-harness.md spec."""
    print(f"SCENARIO: {result['scenario']}")
    print(f"STATUS:   {result['status']}")
    if result.get("description"):
        print(f"DESC:     {result['description']}")

    print("ASSERTIONS:")
    for a in result.get("assertions", []):
        mark = "✓" if a['status'] == "PASS" else "✗"
        print(f"  {mark} [{a['status']}] {a['type']}: expected={a['expected']}, actual={a['actual']}")

    if result["status"] == "FAIL" and result.get("failure"):
        print("\nFAILED ASSERTION:")
        print(f"  {result['failure']['assertion']}")
        print(f"  EXPECTED: {result['failure']['expected']}")
        print(f"  ACTUAL:   {result['failure']['actual']}")

        snap = result.get("snapshot", {})
        print("\nCURRENT SEMANTIC STATE:")
        print(f"  game_state: {snap.get('game_state')}")
        print(f"  player_pos: ({snap.get('player_x')},{snap.get('player_y')}) HP: {snap.get('player_hp')}")
        print(f"  enemy_hp:   {snap.get('enemy_hp')} Active: {snap.get('enemy_active')}")
        print(f"  music:      {snap.get('music_track')}")

        telemetry = result.get("telemetry", [])
        if getattr(telemetry, "events_lost", False):
            print(f"\nWARNING: Telemetry ring buffer overwrote events (oldest available sequence: {getattr(telemetry, 'oldest_available_sequence', 0)})")

        print("\nRECENT TELEMETRY EVENTS:")
        if not telemetry:
            print("  (no events recorded)")
        else:
            for ev in telemetry[-10:]:
                d_str = " ".join(f"{b:02x}" for b in ev['data'])
                print(f"  #{ev['seq']:03d} [f:{ev['frame']:05d}] {ev['type']} ({d_str})")

    print()

def run_all(scenarios_dir="tools/scenarios"):
    """Run all standard test scenarios and return exit code 0 on PASS, 1 on FAIL."""
    all_scenarios = load_scenarios(scenarios_dir)
    # Exclude non-test / demonstration scenarios (test == False or in examples/)
    scenarios = [
        s for s in all_scenarios
        if s.get("test", True) and "examples" not in s.get("_filepath", "")
    ]
    if not scenarios:
        print("No test scenarios found in", scenarios_dir)
        return 0

    print(f"Running {len(scenarios)} test scenario(s) in SameBoy...\n")
    passed = 0
    failed = 0

    for s in scenarios:
        res = run_scenario(s)
        print_result(res)
        if res["status"] == "PASS":
            passed += 1
        else:
            failed += 1

    print("----------------------------------------")
    print(f"Results: {passed} passed, {failed} failed out of {len(scenarios)} test scenarios.")
    return 0 if failed == 0 else 1

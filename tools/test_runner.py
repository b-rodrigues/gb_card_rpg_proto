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
from emulator import (EmulatorSession, STORY_FLAG_ID_MAP, DIALOGUE_ID_MAP,
                      SCENARIO_IDS, ENTITY_ID_MAP, STATE_FLAG_ID_MAP,
                      VARIABLE_ID_MAP, ITEM_ID_MAP, ACTOR_ID_MAP,
                      ACTOR_STATE_NAME_MAP, CHARACTER_ID_MAP, SCENE_MAP,
                      CHARACTER_ID_TO_NAME, ITEM_ID_TO_NAME, ACTOR_ID_TO_NAME,
                      CURRENCY_ID_MAP, PROGRESSION_TARGET_MAP,
                      CURRENCY_ID_TO_NAME, PROG_TYPE_HERO,
                      ITEM_ATTACK_BONUS, HERO_BASE_ATTACK)

VALID_ASSERTION_TYPES = {
    "game_state", "player_position", "player_facing", "player_hp", "music_track",
    "enemy_hp", "battle_turn", "battle_result", "battle_player_hp", "battle_enemy_hp",
    "game_over_choice", "story_flag", "screen", "scene",
    "event_occurred", "event_not_occurred", "dialogue_active", "dialogue_line", "dialogue_id",
    "screen_row", "actor_at",
    "flag", "variable", "inventory", "party_hp", "party_level", "actor_state",
    "currency", "progression_level", "progression_progress", "attack"
}

VALID_ENTITY_IDS = set(ENTITY_ID_MAP.values())

VALID_STORY_FLAGS = set(STORY_FLAG_ID_MAP.values())
VALID_DIALOGUE_IDS = set(DIALOGUE_ID_MAP.values())
VALID_FLAG_NAMES = set(STATE_FLAG_ID_MAP)
VALID_VARIABLE_NAMES = set(VARIABLE_ID_MAP)
VALID_ITEM_NAMES = set(ITEM_ID_MAP) - {"NONE"}
VALID_ACTOR_NAMES = set(ACTOR_ID_MAP)
VALID_ACTOR_STATES = set(ACTOR_STATE_NAME_MAP)
VALID_CURRENCY_NAMES = set(CURRENCY_ID_MAP)
VALID_PROGRESSION_NAMES = set(PROGRESSION_TARGET_MAP)

def validate_scenario(data, filepath):
    scen_id = data.get("scenario_id")
    if scen_id and scen_id not in SCENARIO_IDS:
        raise ValueError(f"SCENARIO ERROR in {filepath}: Unknown scenario_id '{scen_id}'. Valid IDs: {list(SCENARIO_IDS.keys())}")

    for act in data.get("actions", []):
        if act.get("type") == "interact":
            actor = act.get("actor")
            if actor and actor not in VALID_ENTITY_IDS:
                raise ValueError(f"SCENARIO ERROR in {filepath}: Unknown interact actor '{actor}'. Valid entities: {sorted(list(VALID_ENTITY_IDS))}")

    for a in data.get("assertions", []):
        a_type = a.get("type")
        if a_type not in VALID_ASSERTION_TYPES:
            raise ValueError(f"SCENARIO ERROR in {filepath}: Unknown assertion type '{a_type}'. Valid types: {sorted(list(VALID_ASSERTION_TYPES))}")

        flag = a.get("flag")
        if flag and flag not in VALID_STORY_FLAGS:
            raise ValueError(f"SCENARIO ERROR in {filepath}: Unknown story flag '{flag}'. Valid flags: {sorted(list(VALID_STORY_FLAGS))}")

        dlg = a.get("dialogue_id")
        if dlg and dlg not in VALID_DIALOGUE_IDS:
            raise ValueError(f"SCENARIO ERROR in {filepath}: Unknown dialogue_id '{dlg}'. Valid IDs: {sorted(list(VALID_DIALOGUE_IDS))}")

        flag = a.get("flag")
        if flag and flag not in VALID_FLAG_NAMES:
            raise ValueError(f"SCENARIO ERROR in {filepath}: Unknown flag '{flag}'. Valid flags: {sorted(list(VALID_FLAG_NAMES))}")

        var = a.get("variable")
        if var and var not in VALID_VARIABLE_NAMES:
            raise ValueError(f"SCENARIO ERROR in {filepath}: Unknown variable '{var}'. Valid variables: {sorted(list(VALID_VARIABLE_NAMES))}")

        item = a.get("item")
        if item and item not in VALID_ITEM_NAMES:
            raise ValueError(f"SCENARIO ERROR in {filepath}: Unknown item '{item}'. Valid items: {sorted(list(VALID_ITEM_NAMES))}")

        actor = a.get("actor")
        if actor and actor not in VALID_ACTOR_NAMES:
            raise ValueError(f"SCENARIO ERROR in {filepath}: Unknown actor '{actor}'. Valid actors: {sorted(list(VALID_ACTOR_NAMES))}")

        astate = a.get("expected_state")
        if astate and astate not in VALID_ACTOR_STATES:
            raise ValueError(f"SCENARIO ERROR in {filepath}: Unknown actor state '{astate}'. Valid states: {sorted(list(VALID_ACTOR_STATES))}")

        cname = a.get("currency")
        if cname and cname not in VALID_CURRENCY_NAMES:
            raise ValueError(f"SCENARIO ERROR in {filepath}: Unknown currency '{cname}'. Valid currencies: {sorted(list(VALID_CURRENCY_NAMES))}")

        pname = a.get("target")
        if pname and pname not in VALID_PROGRESSION_NAMES:
            raise ValueError(f"SCENARIO ERROR in {filepath}: Unknown progression target '{pname}'. Valid targets: {sorted(list(VALID_PROGRESSION_NAMES))}")

        init = data.get("initial_state") or {}
        for fname, fval in (init.get("flags") or {}).items():
            if fname not in VALID_FLAG_NAMES:
                raise ValueError(f"SCENARIO ERROR in {filepath}: Unknown flag '{fname}' in initial_state. Valid flags: {sorted(list(VALID_FLAG_NAMES))}")
        for vname, vval in (init.get("variables") or {}).items():
            if vname not in VALID_VARIABLE_NAMES:
                raise ValueError(f"SCENARIO ERROR in {filepath}: Unknown variable '{vname}' in initial_state. Valid variables: {sorted(list(VALID_VARIABLE_NAMES))}")
        for cname, cval in (init.get("currency") or {}).items():
            if cname not in VALID_CURRENCY_NAMES:
                raise ValueError(f"SCENARIO ERROR in {filepath}: Unknown currency '{cname}' in initial_state. Valid currencies: {sorted(list(VALID_CURRENCY_NAMES))}")
        for iname, iqty in (init.get("inventory") or {}).items():
            if iname not in VALID_ITEM_NAMES:
                raise ValueError(f"SCENARIO ERROR in {filepath}: Unknown item '{iname}' in initial_state. Valid items: {sorted(list(VALID_ITEM_NAMES))}")
        for aname, aval in (init.get("world") or {}).items():
            if aname not in VALID_ACTOR_NAMES:
                raise ValueError(f"SCENARIO ERROR in {filepath}: Unknown actor '{aname}' in initial_state. Valid actors: {sorted(list(VALID_ACTOR_NAMES))}")
            if aval not in VALID_ACTOR_STATES:
                raise ValueError(f"SCENARIO ERROR in {filepath}: Unknown actor state '{aval}' for '{aname}'. Valid states: {sorted(list(VALID_ACTOR_STATES))}")
        for pname, pstats in (init.get("party") or {}).items():
            if pname not in set(CHARACTER_ID_MAP):
                raise ValueError(f"SCENARIO ERROR in {filepath}: Unknown party member '{pname}'. Valid members: {sorted(list(CHARACTER_ID_MAP))}")
        for pname, pstats in (init.get("progression") or {}).items():
            if pname not in VALID_PROGRESSION_NAMES:
                raise ValueError(f"SCENARIO ERROR in {filepath}: Unknown progression target '{pname}'. Valid targets: {sorted(list(VALID_PROGRESSION_NAMES))}")
        equip = init.get("equipment") or {}
        if "weapon" in equip and equip["weapon"] not in VALID_ITEM_NAMES:
            raise ValueError(f"SCENARIO ERROR in {filepath}: Unknown equipment weapon '{equip.get('weapon')}'. Valid items: {sorted(list(VALID_ITEM_NAMES))}")
        scene = init.get("scene")
        if scene and scene not in set(SCENE_MAP.values()):
            raise ValueError(f"SCENARIO ERROR in {filepath}: Unknown scene '{scene}'. Valid scenes: {sorted(set(SCENE_MAP.values()))}")

def load_scenarios(scenarios_dir="tools/scenarios"):
    """Load all JSON scenario files from directory and subdirectories with strict validation."""
    scenarios = []
    pattern = os.path.join(scenarios_dir, "**", "*.json")
    for filepath in sorted(glob.glob(pattern, recursive=True)):
        try:
            with open(filepath, 'r') as f:
                data = json.load(f)
                data['_filepath'] = filepath
                validate_scenario(data, filepath)
                scenarios.append(data)
        except Exception as e:
            print(f"Error loading scenario {filepath}: {e}", file=sys.stderr)
            raise
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

        # Load requested scenario from its declarative initial_state
        session.load_scenario(scenario)

        # Execute action sequence
        for act in actions:
            act_type = act.get("type")
            if act_type == "press":
                session.press(act.get("button", "A"))
            elif act_type == "wait":
                session.wait(act.get("frames", 1))
            elif act_type == "interact":
                # Semantic interaction: the player must already be adjacent
                # and facing the target (set via initial_state); press A.
                session.press("A")
                session.wait(act.get("frames", 5))
            elif act_type == "use_item":
                # Open the quick screen (START is the universal open key in
                # both the overworld and battle) and use the cursor item (A).
                # ITEM is the default tab; cursor starts at the first item.
                session.press("START")
                session.wait(4)
                session.press("A")
                session.wait(act.get("frames", 4))
            elif act_type == "equip_item":
                session.debug_action(session.DBG_ACT_EQUIP_ITEM,
                                     ITEM_ID_MAP[act.get("item")], 0, 0)
            elif act_type == "add_item":
                session.debug_action(session.DBG_ACT_ADD_ITEM,
                                     ITEM_ID_MAP[act.get("item")],
                                     0, act.get("quantity", 1))
            elif act_type == "remove_item":
                session.debug_action(session.DBG_ACT_REMOVE_ITEM,
                                     ITEM_ID_MAP[act.get("item")],
                                     0, act.get("quantity", 1))
            elif act_type == "add_currency":
                session.debug_action(session.DBG_ACT_ADD_CURRENCY,
                                     CURRENCY_ID_MAP[act.get("currency")],
                                     act.get("amount", 0), 0)
            elif act_type == "add_progress":
                ttype, tid = PROGRESSION_TARGET_MAP[act.get("target")]
                session.debug_action(session.DBG_ACT_ADD_PROGRESS,
                                     ttype, tid, act.get("amount", 0))
            elif act_type == "buy_item":
                session.debug_action(session.DBG_ACT_BUY_ITEM,
                                     ITEM_ID_MAP[act.get("item")], 0, 0)
            elif act_type == "use_item_direct":
                session.debug_action(session.DBG_ACT_USE_ITEM,
                                     ITEM_ID_MAP[act.get("item")],
                                     0, act.get("member", 1))

        # Read final snapshot, canonical state buffer and telemetry
        snap = session.snapshot()
        state_snap = session.state_snapshot()
        telemetry = session.get_telemetry()

        # Check if any assertion needs logical screen buffer
        has_screen_assert = any(a.get("type") == "screen_row" for a in scenario.get("assertions", []))
        screen_lines = []
        if has_screen_assert:
            screen_buf = session.get_screen_buf()
            screen_lines = screen_buf.split('\n')
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

        elif a_type == "screen":
            actual = snap.get("screen", "UNKNOWN")
            passed = (actual == expected)

        elif a_type == "scene":
            actual = snap.get("scene", "UNKNOWN")
            passed = (actual == expected)

        elif a_type == "actor_at":
            exp_entity = a.get("entity")
            exp_x = a.get("x")
            exp_y = a.get("y")
            actors = snap.get("actors", [])
            found = [ac for ac in actors
                     if ac.get("id_name") == exp_entity and
                     ac.get("x") == exp_x and ac.get("y") == exp_y]
            expected = f"{exp_entity} at ({exp_x},{exp_y})"
            actual = "; ".join(
                f"{ac.get('id_name')} at ({ac.get('x')},{ac.get('y')})" for ac in actors
            ) or "NO ACTORS"
            passed = len(found) > 0

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

        elif a_type == "player_facing":
            actual = snap.get("player_facing", "UNKNOWN")
            passed = (actual == expected)

        elif a_type == "music_track":
            actual = snap.get("music_track", "UNKNOWN")
            passed = (actual == expected)

        elif a_type == "enemy_hp":
            actual = snap.get("enemy_hp", 0)
            passed = (actual == int(expected))

        elif a_type == "battle_turn":
            actual = snap.get("battle_turn", "UNKNOWN")
            passed = (actual == expected)

        elif a_type == "battle_player_hp":
            actual = snap.get("battle_player_hp", 0)
            passed = (actual == int(expected))

        elif a_type == "battle_enemy_hp":
            actual = snap.get("battle_enemy_hp", 0)
            passed = (actual == int(expected))

        elif a_type == "game_over_choice":
            actual = snap.get("game_over_choice", 0)
            passed = (actual == int(expected))

        elif a_type == "battle_result":
            actual = snap.get("battle_result", "UNKNOWN")
            passed = (actual == expected)

        elif a_type == "story_flag":
            exp_flag = a.get("flag")
            exp_val = a.get("expected", True)
            expected = f"{exp_flag}={exp_val}"
            active_flags = snap.get("story_flags_active", [])
            has_flag = exp_flag in active_flags
            passed = (has_flag == exp_val)
            actual = f"{exp_flag}={has_flag}"

        elif a_type == "dialogue_active":
            actual = snap.get("dialogue_active", False)
            passed = (actual == bool(expected))

        elif a_type == "dialogue_line":
            actual = snap.get("dialogue_line", 0)
            passed = (actual == int(expected))

        elif a_type == "dialogue_id":
            actual = snap.get("dialogue_id_name", "NONE")
            passed = (actual == expected)

        elif a_type == "screen_row":
            row_idx = a.get("row", 0)
            contains_str = a.get("contains", expected)
            expected = f"row {row_idx} contains '{contains_str}'"
            actual_row = screen_lines[row_idx] if row_idx < len(screen_lines) else ""
            passed = contains_str in actual_row
            actual = f"row {row_idx}: '{actual_row.strip()}'"

        elif a_type == "event_occurred":
            exp_event = a.get("event", expected)
            exp_flag = a.get("flag")
            if exp_flag:
                expected = f"{exp_event}({exp_flag})"
            else:
                expected = exp_event

            matching_events = [ev for ev in telemetry if ev.get("type") == exp_event]
            if exp_flag:
                matching_events = [ev for ev in matching_events if ev.get("flag_name") == exp_flag or ev.get("flag_id") == exp_flag]

            passed = len(matching_events) > 0
            actual = f"EMITTED ({len(matching_events)} time(s))" if passed else "NOT_EMITTED"

        elif a_type == "event_not_occurred":
            exp_event = a.get("event", expected)
            exp_flag = a.get("flag")
            if exp_flag:
                expected = f"NOT {exp_event}({exp_flag})"
            else:
                expected = f"NOT {exp_event}"

            matching_events = [ev for ev in telemetry if ev.get("type") == exp_event]
            if exp_flag:
                matching_events = [ev for ev in matching_events if ev.get("flag_name") == exp_flag or ev.get("flag_id") == exp_flag]

            passed = len(matching_events) == 0
            actual = "NOT_EMITTED" if passed else f"EMITTED ({len(matching_events)} time(s))"

        elif a_type == "flag":
            exp_flag = a.get("flag")
            expected = f"flag {exp_flag} == {expected}"
            active_flags = (state_snap or {}).get("flags", [])
            has_flag = exp_flag in active_flags
            actual = f"flag {exp_flag} == {has_flag}"
            passed = (has_flag == a.get("expected", True))

        elif a_type == "variable":
            exp_var = a.get("variable")
            expected = f"variable {exp_var} == {expected}"
            actual = (state_snap or {}).get("variables", {}).get(exp_var)
            passed = (actual == int(a.get("expected")))

        elif a_type == "inventory":
            exp_item = a.get("item")
            expected = f"item {exp_item} == {expected}"
            items = (state_snap or {}).get("inventory", [])
            qty = sum(it["quantity"] for it in items
                      if it["item_id"] == ITEM_ID_MAP[exp_item])
            actual = f"item {exp_item} == {qty}"
            passed = (qty == int(a.get("expected")))

        elif a_type == "party_hp":
            expected = f"party member {a.get('member', 0)} hp == {expected}"
            party = (state_snap or {}).get("party", [])
            member = int(a.get("member", 0))
            actual = party[member]["hp"] if member < len(party) else None
            passed = (actual == int(a.get("expected")))

        elif a_type == "party_level":
            # Hero level is owned by the generic progression system.
            expected = f"party member {a.get('member', 0)} level == {expected}"
            hero_prog = next((p for p in (state_snap or {}).get("progression", [])
                              if p.get("name") == "HERO_1"), None)
            actual = hero_prog.get("level") if hero_prog else 1
            passed = (actual == int(a.get("expected")))

        elif a_type == "actor_state":
            exp_actor = a.get("actor")
            exp_state = a.get("expected_state", "ALIVE")
            expected = f"actor {exp_actor} == {exp_state}"
            world = (state_snap or {}).get("world", [])
            actor_id = ACTOR_ID_MAP[exp_actor]
            entry = next((w for w in world if w["actor_id"] == actor_id), None)
            actual = entry["state"] if entry else "ALIVE"
            passed = (actual == exp_state)

        elif a_type == "currency":
            exp_cur = a.get("currency")
            expected = f"currency {exp_cur} == {expected}"
            actual = (state_snap or {}).get("currency", {}).get(exp_cur)
            passed = (actual == int(a.get("expected")))

        elif a_type == "progression_level":
            exp_target = a.get("target")
            expected = f"progression {exp_target} level == {expected}"
            entry = next((p for p in (state_snap or {}).get("progression", [])
                          if p.get("name") == exp_target), None)
            actual = entry.get("level") if entry else None
            passed = (actual == int(a.get("expected")))

        elif a_type == "progression_progress":
            exp_target = a.get("target")
            expected = f"progression {exp_target} progress == {expected}"
            entry = next((p for p in (state_snap or {}).get("progression", [])
                          if p.get("name") == exp_target), None)
            actual = entry.get("progress") if entry else None
            passed = (actual == int(a.get("expected")))

        elif a_type == "attack":
            equipment = (state_snap or {}).get("equipment", {})
            weapon = equipment.get("weapon", "NONE")
            actual = HERO_BASE_ATTACK + ITEM_ATTACK_BONUS.get(weapon, 0)
            expected = f"hero attack == {expected}"
            passed = (actual == int(a.get("expected")))

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
        "state": state_snap,
        "telemetry": telemetry,
        "assertions": assertion_results
    }

def format_state(snap, state_snap):
    """Render the canonical GameState as LLM-readable semantic text.

    This is the intended LLM-facing representation.  The byte layouts in
    g_snap_buf / g_state_snap_buf are internal transports, not the API.
    """
    lines = []
    if not snap:
        return "\n".join(lines)
    scene = snap.get("scene", "UNKNOWN")
    lines.append(f"SCENE={scene}")
    lines.append("PLAYER=({},{}) FACING={}".format(
        snap.get("player_x"), snap.get("player_y"), snap.get("player_facing")))

    flags = sorted((state_snap or {}).get("flags", []))
    lines.append("FLAGS: " + (" ".join(flags) if flags else "(none)"))

    variables = (state_snap or {}).get("variables", {})
    vars_str = " ".join(f"{k}={v}" for k, v in sorted(variables.items()))
    lines.append("VARIABLES: " + (vars_str if vars_str else "(none)"))

    currency = (state_snap or {}).get("currency", {})
    cur_str = " ".join(f"{k}={v}" for k, v in sorted(currency.items()))
    lines.append("CURRENCY: " + (cur_str if cur_str else "(none)"))

    party = (state_snap or {}).get("party", [])
    for i, m in enumerate(party):
        name = CHARACTER_ID_TO_NAME.get(m.get("id"), f"ID{m.get('id')}")
        lines.append("PARTY[{}]: {} {}/{}".format(
            i, name, m.get("hp"), m.get("max_hp")))
    if not party:
        lines.append("PARTY: (none)")

    inventory = (state_snap or {}).get("inventory", [])
    items = {}
    for it in inventory:
        iname = ITEM_ID_TO_NAME.get(it.get("item_id"), f"ID{it.get('item_id')}")
        items[iname] = items.get(iname, 0) + it.get("quantity", 0)
    inv_str = " ".join(f"{k} x{v}" for k, v in sorted(items.items()))
    lines.append("INVENTORY: " + (inv_str if inv_str else "(none)"))

    world = (state_snap or {}).get("world", [])
    world_str = " ".join("{}={}".format(ACTOR_ID_TO_NAME.get(w.get("actor_id"),
                        f"ID{w.get('actor_id')}"), w.get("state")) for w in world)
    lines.append("WORLD: " + (world_str if world_str else "(none)"))

    progression = (state_snap or {}).get("progression", [])
    if progression:
        lines.append("PROGRESSION:")
        for p in progression:
            lines.append("  {}: level={} progress={}".format(
                p.get("name"), p.get("level"), p.get("progress")))

    equipment = (state_snap or {}).get("equipment", {})
    weapon = equipment.get("weapon", "NONE")
    lines.append(f"EQUIPMENT: {weapon}")
    lines.append(f"ATTACK: {HERO_BASE_ATTACK + ITEM_ATTACK_BONUS.get(weapon, 0)}")
    return "\n".join(lines)


def build_initial_state_from_snapshot(snap, state_snap):
    """Rebuild an initial_state dict from observed snapshot state.

    Used by the roundtrip check to prove GameState <-> descriptor is
    lossless (the save/load boundary design, roadmap section 20).
    """
    initial = {
        "scene": snap.get("scene", "FIELD"),
        "player": {
            "x": snap.get("player_x", 4),
            "y": snap.get("player_y", 4),
            "facing": snap.get("player_facing", "DOWN"),
        },
        "seed": 1,
    }
    if state_snap is None:
        return initial

    flags = {name: True for name in state_snap.get("flags", [])}
    if flags:
        initial["flags"] = flags

    variables = {k: v for k, v in state_snap.get("variables", {}).items()}
    if variables:
        initial["variables"] = variables

    currency = {k: v for k, v in state_snap.get("currency", {}).items()}
    if currency:
        initial["currency"] = currency

    party = {}
    for m in state_snap.get("party", []):
        name = CHARACTER_ID_TO_NAME.get(m.get("id"))
        if name:
            party[name] = {
                "hp": m.get("hp", 10),
                "max_hp": m.get("max_hp", 10),
            }
    if party:
        initial["party"] = party

    inventory = {}
    for it in state_snap.get("inventory", []):
        iname = ITEM_ID_TO_NAME.get(it.get("item_id"))
        if iname:
            inventory[iname] = inventory.get(iname, 0) + it.get("quantity", 0)
    if inventory:
        initial["inventory"] = inventory

    world = {}
    for w in state_snap.get("world", []):
        aname = ACTOR_ID_TO_NAME.get(w.get("actor_id"))
        if aname:
            world[aname] = w.get("state", "ALIVE")
    if world:
        initial["world"] = world

    progression = {}
    for p in state_snap.get("progression", []):
        pname = p.get("name")
        if pname:
            progression[pname] = {
                "level": p.get("level", 1),
                "progress": p.get("progress", 0),
            }
    if progression:
        initial["progression"] = progression

    equipment = state_snap.get("equipment", {})
    weapon = equipment.get("weapon", "NONE")
    if weapon != "NONE":
        initial["equipment"] = {"weapon": weapon}

    return initial


def normalize_semantic_state(snap, state_snap):
    """Canonical comparable form of the semantic state (for roundtrip)."""
    return {
        "scene": snap.get("scene"),
        "player": (snap.get("player_x"), snap.get("player_y"),
                   snap.get("player_facing")),
        "flags": sorted((state_snap or {}).get("flags", [])),
        "variables": (state_snap or {}).get("variables", {}),
        "currency": (state_snap or {}).get("currency", {}),
        "party": sorted((m.get("id"), m.get("hp"), m.get("max_hp"))
                        for m in (state_snap or {}).get("party", [])),
        "inventory": sorted((it.get("item_id"), it.get("quantity"))
                            for it in (state_snap or {}).get("inventory", [])),
        "world": sorted((w.get("actor_id"), w.get("state"))
                        for w in (state_snap or {}).get("world", [])),
        "progression": sorted((p.get("name"), p.get("level"), p.get("progress"))
                              for p in (state_snap or {}).get("progression", [])),
        "equipment": (state_snap or {}).get("equipment", {}),
    }


def run_roundtrip(scenario):
    """Load a scenario, dump its semantic state, re-inject it, and verify
    the observed state is unchanged (GameState <-> descriptor lossless)."""
    session = EmulatorSession()
    try:
        session.connect()
        session.load_scenario(scenario)
        session.step(2)
        snap1 = session.snapshot()
        st1 = session.state_snapshot()
        rebuilt = build_initial_state_from_snapshot(snap1, st1)
        session.load_scenario(rebuilt)
        session.step(2)
        snap2 = session.snapshot()
        st2 = session.state_snapshot()
    finally:
        session.disconnect()

    first = normalize_semantic_state(snap1, st1)
    second = normalize_semantic_state(snap2, st2)
    passed = (first == second)
    diff = {k: (first.get(k), second.get(k)) for k in first
            if first.get(k) != second.get(k)}
    return {
        "scenario": scenario.get("name", "unknown"),
        "status": "PASS" if passed else "FAIL",
        "diff": diff,
        "rebuilt_initial_state": rebuilt,
    }


def print_roundtrip(result):
    print(f"SCENARIO: {result['scenario']}")
    print(f"ROUNDTRIP STATUS: {result['status']}")
    if result["status"] == "FAIL":
        print("\nDIFF:")
        for key, (a, b) in result["diff"].items():
            print(f"  {key}:")
            print(f"    first:  {a}")
            print(f"    second: {b}")
        print("\nREBUILT INITIAL_STATE:")
        print(json.dumps(result["rebuilt_initial_state"], indent=2))
    else:
        print("  state survived a load -> dump -> re-inject -> reload cycle")
    print()


def print_result(result, show_state=False):
    """Print structured scenario execution result per dev-harness.md spec."""
    print(f"SCENARIO: {result['scenario']}")
    print(f"STATUS:   {result['status']}")
    if result.get("description"):
        print(f"DESC:     {result['description']}")

    print("ASSERTIONS:")
    for a in result.get("assertions", []):
        mark = "✓" if a['status'] == "PASS" else "✗"
        print(f"  {mark} [{a['status']}] {a['type']}: expected={a['expected']}, actual={a['actual']}")

    if show_state:
        print("\nSEMANTIC STATE:")
        dump = format_state(result.get("snapshot"), result.get("state"))
        for line in dump.splitlines():
            print(f"  {line}")

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
                extra_str = f" {ev['flag_name']}" if "flag_name" in ev else ""
                print(f"  #{ev['seq']:03d} [f:{ev['frame']:05d}] {ev['type']}{extra_str} ({d_str})")

    print()

def run_all(scenarios_dir="tools/scenarios", show_state=False):
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
        print_result(res, show_state=show_state)
        if res["status"] == "PASS":
            passed += 1
        else:
            failed += 1

    print("----------------------------------------")
    print(f"Results: {passed} passed, {failed} failed out of {len(scenarios)} test scenarios.")
    return 0 if failed == 0 else 1

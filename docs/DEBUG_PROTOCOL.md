# Debug Protocol

This document defines the machine-readable development and debugging interface for the Game Boy RPG.

The protocol exists to make the game **fully testable and inspectable by an LLM** without requiring a human to play through the game or interpret screenshots.

The player-facing Game Boy interface is optimized for gameplay.

The debug interface is optimized for deterministic automation, inspection, testing, and diagnosis.

---

# 1. Goals

The debug protocol must allow an external development agent to:

1. Start the game in a known state.
2. Load a predefined scenario.
3. Inspect the complete relevant game state.
4. Control player input.
5. Advance the game by deterministic numbers of frames.
6. Inspect recent gameplay events.
7. Inspect the world and entities.
8. Inspect story flags.
9. Inspect battle state.
10. Inspect audio state.
11. Control and inspect RNG.
12. Assert expected conditions.
13. Detect state transitions.
14. Reproduce bugs deterministically.
15. Produce machine-readable PASS/FAIL results.

The protocol must not depend on the visual appearance of the game.

---

# 2. Design Principles

## 2.1 Semantic State Over Pixels

The debug interface is authoritative for gameplay state.

A screenshot may show:

```text
@     E
```

The debug interface must instead identify:

```text
PLAYER:
  id: player
  position: (8,5)

ENEMY:
  id: slime_01
  type: enemy
  position: (14,5)
```

The fact that the enemy is currently rendered as `E` is irrelevant to the protocol.

---

## 2.2 Deterministic By Default

Given:

* the same ROM;
* the same scenario;
* the same RNG seed;
* the same input sequence;
* the same frame advancement;

the game should produce the same observable result.

---

## 2.3 The Harness Must Test Real Gameplay

Scenarios may establish state.

They must not bypass the behavior being tested.

For example, a town-event scenario may teleport the player into town and establish story flags.

It must then allow normal collision/event/gameplay logic to trigger the event.

---

## 2.4 Stable Semantic Names

Names exposed through the protocol are part of the development API.

Prefer:

```text
BATTLE_STARTED
PLAYER_MOVED
TOWN_ATTACK_STARTED
slime_01
town_guard_01
```

Avoid names based on implementation details or visual representations.

---

# 3. Protocol Layers

The development system consists of four layers:

```text
┌──────────────────────────────┐
│            LLM               │
│     Agent / Coding Tool      │
└──────────────┬───────────────┘
               │
        Host Debug API
               │
┌──────────────▼───────────────┐
│       tools/dev.py           │
│       Test Runner            │
└──────────────┬───────────────┘
               │
        Emulator Transport
               │
┌──────────────▼───────────────┐
│         SameBoy               │
│       Debug ROM               │
└──────────────┬───────────────┘
               │
┌──────────────▼───────────────┐
│        Game Systems           │
│ world / battle / story / etc. │
└──────────────────────────────┘
```

Game systems must not depend on the LLM or host-side Python tooling.

---

# 4. Debug Build

The protocol is available only in the debug ROM.

Build with:

```bash
make debug
```

Result:

```text
build/rpg_card_proto_debug.gb
```

The release ROM:

```bash
make release
```

must not require the debug protocol.

Debug functionality must be conditionally compiled or otherwise excluded from the release build where appropriate.

---

# 5. Transport

The exact transport implementation is emulator-specific.

The protocol must therefore distinguish between:

### Transport

How bytes/messages physically move between host and emulator.

### Protocol

What those messages mean.

The gameplay code must not contain SameBoy-specific assumptions.

Conceptually:

```text
DebugProtocol
      │
      ▼
DebugTransport
      │
      ├── SameBoy
      └── future implementation
```

The initial implementation targets SameBoy.

The exact SameBoy integration must be verified against the version available through the Nix development environment.

---

# 6. Command Model

The host may send commands to the debug ROM.

The initial command vocabulary is:

```text
HELP
RESET
LOAD_SCENARIO
PRESS
RELEASE
WAIT
STEP
INSPECT
INSPECT_AREA
SNAPSHOT
EVENTS
EVENTS_SINCE
SET_RNG
GET_RNG
SET_FLAG
CLEAR_FLAG
TELEPORT
SET_HP
ASSERT
```

Commands must return a structured response.

Every response must contain enough information to determine:

* whether the command succeeded;
* what changed;
* what the current relevant state is;
* whether an error occurred.

---

# 7. Command Responses

Successful commands should use a structure conceptually equivalent to:

```json
{
  "ok": true,
  "command": "INSPECT",
  "result": {}
}
```

Errors:

```json
{
  "ok": false,
  "command": "LOAD_SCENARIO",
  "error": {
    "code": "SCENARIO_NOT_FOUND",
    "message": "Unknown scenario: town_event_99"
  }
}
```

Error codes must be stable.

Do not rely on parsing human prose to determine whether a command succeeded.

---

# 8. HELP

Command:

```text
HELP
```

Returns the available commands and concise descriptions.

Example:

```json
{
  "ok": true,
  "command": "HELP",
  "commands": [
    "RESET",
    "LOAD_SCENARIO",
    "PRESS",
    "WAIT",
    "STEP",
    "INSPECT",
    "INSPECT_AREA",
    "SNAPSHOT",
    "EVENTS",
    "EVENTS_SINCE",
    "SET_RNG",
    "GET_RNG"
  ]
}
```

---

# 9. RESET

Command:

```text
RESET
```

Returns the game to its normal initial state.

It should clear:

* gameplay state;
* story flags;
* entities;
* battle state;
* telemetry;
* debug state.

It should establish a deterministic default RNG seed.

The default seed should be documented and stable.

---

# 10. LOAD_SCENARIO

Command:

```text
LOAD_SCENARIO <name>
```

Example:

```text
LOAD_SCENARIO first_encounter
```

The scenario establishes a known state.

Loading a scenario must:

1. reset relevant game state;
2. apply scenario configuration;
3. set the RNG seed;
4. initialize entities;
5. initialize story state;
6. initialize battle state if applicable;
7. initialize audio state if specified;
8. clear old telemetry;
9. emit a `SCENARIO_LOADED` event.

The scenario must then run through normal gameplay logic.

### Declarative loading (`initial_state`)

Scenarios specify their starting state declaratively in the JSON
`initial_state` field.  The host serializes it into a fixed-size byte
descriptor in `g_scen_state_buf` (layout documented in
`src/debug/telemetry.h`, `STATE_LOAD_DESC_*`), sets `g_scen_load_state`,
and the ROM applies it through a single general loader
(`scenario_load_state()`) that constructs the canonical `GameState`,
loads the world through the normal `world_init`/`world_load_map` paths
(so persistent actor defeats are respected), and starts the game in the
requested screen.

Supported `initial_state` fields:

```json
{
  "scene": "TOWN",
  "player": { "x": 8, "y": 6, "facing": "DOWN" },
  "seed": 42,
  "flags": { "ARRIVED_TOWN": true, "MET_MAYOR": false },
  "variables": { "GOLD": 150, "CHAPTER": 2 },
  "party": { "HERO": { "level": 3, "hp": 24, "max_hp": 30 } },
  "inventory": { "POTION": 2, "BOMB": 1 },
  "world": { "SLIME_FIELD": "DEFEATED" },
  "screen": "BATTLE",
  "dialogue": "MAYOR_GREETING",
  "start_battle": true,
  "game_over_choice": 1,
  "font_test": true
}
```

Rules:

* Each variable-length section carries a count; only the listed entries
  are applied, so unspecified sections keep the game's default state
  (e.g. leaving `variables` out keeps `CHAPTER == 1`).
* A `DEFEATED` actor in `world` is honoured by `actor_load_scene()`: the
  actor is not spawned into the scene.
* Scenario setup must never emit gameplay telemetry; all descriptor state
  is written directly into `GameState`.
* `screen` starts a non-overworld screen directly (DIALOGUE/BATTLE/
  GAME_OVER/THANKS); `dialogue` + `start_battle` configure the active
  runtime screens.

### Semantic state dump (LLM-facing)

The byte buffers (`g_snap_buf`, `g_state_snap_buf`) are internal transports.
The LLM-facing representation is semantic text rendered host-side from the
parsed snapshot:

```text
SCENE=FOREST
PLAYER=(10,8) FACING=RIGHT
FLAGS: ARRIVED_TOWN MET_MAYOR
VARIABLES: CHAPTER=1 GOLD=150
PARTY[0]: HERO lvl=3 24/30
INVENTORY: POTION x2
WORLD: SLIME_FOREST=DEFEATED
```

It is opt-in via `dev.py`:

```text
python3 tools/dev.py scenario <name> --state   # run + dump semantic state
python3 tools/dev.py state <name>              # run + dump semantic state
python3 tools/dev.py test --state              # every scenario
```

### Save/load boundary check

`GameState` is the potential save unit; `Battle`, `DialogueState`,
`RenderCache`, input, and `World.actors` HP/facing are runtime state and are
excluded.  The roundtrip check proves the boundary is lossless:

```text
python3 tools/dev.py roundtrip <scenario>
```

It loads an `initial_state`, dumps the canonical state, rebuilds a descriptor
from the dump, reloads, and asserts the observed state is unchanged.

---

# 11. Scenario Format

Scenario files live in:

```text
tools/scenarios/
```

They use JSON.

Example:

```json
{
  "name": "first_encounter",
  "description": "Player walks into the first enemy.",
  "rng_seed": 12345,

  "world": {
    "map": "field",
    "player": {
      "x": 8,
      "y": 5,
      "facing": "EAST"
    }
  },

  "entities": [
    {
      "id": "slime_01",
      "type": "enemy",
      "x": 10,
      "y": 5
    }
  ],

  "story": {
    "flags": {}
  },

  "audio": {
    "track": "FIELD"
  }
}
```

The scenario format may evolve.

New fields must remain backwards compatible where practical.

---

# 12. Scenario Philosophy

A scenario establishes **preconditions**, not outcomes.

Correct:

```json
{
  "world": {
    "map": "town",
    "player": {
      "x": 12,
      "y": 8
    }
  }
}
```

Then the test walks into the relevant NPC or trigger.

Incorrect:

```json
{
  "game_state": "TOWN_EVENT_COMPLETE"
}
```

when the scenario is supposed to test whether the town event triggers.

The latter tests state injection rather than gameplay.

---

# 13. PRESS

Command:

```text
PRESS <button>
```

Supported buttons:

```text
UP
DOWN
LEFT
RIGHT
A
B
START
SELECT
```

A press represents a normal gameplay button action.

The input should pass through the same input system used by the player.

---

# 14. RELEASE

Command:

```text
RELEASE <button>
```

Releases a previously held button.

This is primarily useful for future systems requiring held input.

The initial game may treat `PRESS` as a single logical button action.

---

# 15. WAIT

Command:

```text
WAIT <frames>
```

Advances the game by the specified number of frames.

Example:

```text
WAIT 60
```

represents approximately one second at 60 Hz.

The exact timing must use the Game Boy/emulator frame rate rather than host wall-clock timing.

---

# 16. STEP

Command:

```text
STEP <frames>
```

`STEP` is equivalent to deterministic frame advancement but is intended primarily for debugging.

Example:

```text
STEP 1
```

advances exactly one frame.

This is useful for investigating:

* input timing;
* collisions;
* state transitions;
* animations;
* battle logic;
* VBlank behavior;
* audio transitions.

---

# 17. INSPECT

Command:

```text
INSPECT
```

Returns the current semantic game state.

The response must be concise enough for an LLM to consume.

Minimum fields:

```text
game
player
map
entities
story
battle
audio
rng
frame
```

Example:

```json
{
  "ok": true,
  "result": {
    "game": {
      "state": "OVERWORLD"
    },

    "player": {
      "id": "player",
      "map": "field",
      "x": 8,
      "y": 5,
      "facing": "EAST",
      "hp": 20,
      "max_hp": 20
    },

    "entities": [
      {
        "id": "slime_01",
        "type": "enemy",
        "x": 10,
        "y": 5,
        "hp": 10,
        "max_hp": 10
      }
    ],

    "story": {
      "flags": {}
    },

    "battle": null,

    "audio": {
      "track": "FIELD",
      "playing": true
    },

    "rng": {
      "seed": 12345
    },

    "frame": 1832
  }
}
```

---

# 18. INSPECT_AREA

Command:

```text
INSPECT_AREA <radius>
```

Returns the semantic and spatial information around the player.

Example:

```text
INSPECT_AREA 4
```

The response should include a compact ASCII representation where useful.

Example:

```text
012345678
.........
....@....
......E..
.........
```

It must additionally provide semantic entities.

Example:

```json
{
  "entities": [
    {
      "id": "slime_01",
      "type": "enemy",
      "x": 10,
      "y": 5
    }
  ]
}
```

ASCII is convenience information.

Entity data is authoritative.

---

# 19. SNAPSHOT

Command:

```text
SNAPSHOT
```

Returns a complete machine-readable state snapshot.

Unlike `INSPECT`, which may be optimized for concise repeated inspection, `SNAPSHOT` should include all state relevant to debugging.

Potential sections:

```text
game
world
player
party
entities
story
inventory
battle
audio
rng
input
frame
telemetry
debug
```

Do not include raw memory dumps by default.

---

# 20. EVENTS

Command:

```text
EVENTS
```

Returns the recent telemetry events in the ring buffer.

Example:

```json
{
  "events": [
    {
      "sequence": 120,
      "frame": 1001,
      "type": "PLAYER_MOVED",
      "data": {
        "from": [8, 5],
        "to": [9, 5]
      }
    },
    {
      "sequence": 121,
      "frame": 1002,
      "type": "COLLISION",
      "data": {
        "entity_a": "player",
        "entity_b": "slime_01"
      }
    },
    {
      "sequence": 122,
      "frame": 1002,
      "type": "ENCOUNTER_STARTED",
      "data": {
        "enemy": "slime_01"
      }
    }
  ]
}
```

---

# 21. EVENTS_SINCE

Command:

```text
EVENTS_SINCE <sequence>
```

Example:

```text
EVENTS_SINCE 120
```

Returns all available events after sequence `120`.

This is the preferred mechanism for incremental inspection.

An agent should not need to repeatedly download the entire telemetry buffer.

---

# 22. Telemetry Sequence Numbers

Every event receives a monotonically increasing sequence number.

Example:

```text
120
121
122
123
```

Sequence numbers allow the host to determine exactly what happened between two inspections.

If the ring buffer overwrites old events, the response must explicitly indicate that events were lost.

Example:

```json
{
  "events_lost": true,
  "oldest_available_sequence": 150
}
```

The harness must never silently pretend that missing events did not occur.

---

# 23. Telemetry Event Schema

Every event contains:

```json
{
  "sequence": 123,
  "frame": 1002,
  "type": "PLAYER_MOVED",
  "data": {}
}
```

Required fields:

```text
sequence  (uint32_t, little-endian)
frame     (uint32_t, little-endian)
type      (uint8_t, mapped via EVENT_TYPE_MAP)
data      (uint8_t[4])
```

Binary Memory ABI Layout (`GameEvent` = 13 bytes):
- Offset 0..3: `uint32_t seq`
- Offset 4..7: `uint32_t frame`
- Offset 8:    `uint8_t type`
- Offset 9..12: `uint8_t data[4]`

Event types must use stable uppercase identifiers.

---

# 24. Required Initial Event Types

The initial protocol defines:

```text
GAME_STARTED
GAME_STATE_CHANGED

SCENARIO_LOADED

PLAYER_MOVED
PLAYER_FACING_CHANGED
COLLISION

ACTOR_COLLISION
ACTOR_INTERACTION
ACTOR_COMBAT_START

ENCOUNTER_STARTED
BATTLE_STARTED
BATTLE_ENDED

DAMAGE_DEALT
HEALING_APPLIED
ENTITY_DEFEATED

TURN_STARTED
TURN_ENDED

STORY_FLAG_SET
STORY_FLAG_CLEARED
STORY_EVENT_STARTED
STORY_EVENT_ENDED

MUSIC_CHANGED

RNG_SEEDED
RNG_USED
```

## World Actors

**World Actor** is the canonical term for any character-like entity that
exists in an overworld scene (Mayor, Guard, Shopkeeper, Slime, Bat, Boss,
Villager, Monster). Friendly NPCs and hostile enemies share one data-driven
structure; hostility, interaction type, dialogue ID, and battle ID decide
what happens when the player engages an actor.

Observe actors with:

```text
ACTOR_COLLISION      actor collided with the player (id, x, y)
ACTOR_INTERACTION    actor engaged by the player (id, interaction)
ACTOR_COMBAT_START   hostile actor engagement started combat (id)
```

The SNAPSHOT `actors` section lists the active scene's actors with their
semantic `id`, `position`, `facing`, `visual`, `hostile`, `interaction`,
`dialogue`, and `battle` fields. Do not maintain separate NPC/enemy debug
formats: every overworld character is a World Actor.

Not every game system needs every event immediately.

As systems are implemented, their important transitions must become observable.

---

# 25. GAME_STATE_CHANGED

Example:

```json
{
  "type": "GAME_STATE_CHANGED",
  "data": {
    "from": "OVERWORLD",
    "to": "BATTLE"
  }
}
```

Initial game states may include:

```text
BOOT
TITLE
OVERWORLD
BATTLE
MENU
DIALOGUE
GAME_OVER
```

The enum may expand as the game grows.

---

# 26. PLAYER_MOVED

Example:

```json
{
  "type": "PLAYER_MOVED",
  "data": {
    "from": [8, 5],
    "to": [9, 5]
  }
}
```

Blocked movement should also be observable when useful.

Example:

```json
{
  "type": "PLAYER_MOVEMENT_BLOCKED",
  "data": {
    "position": [9, 5],
    "direction": "EAST",
    "reason": "WALL"
  }
}
```

---

# 27. COLLISION

Example:

```json
{
  "type": "COLLISION",
  "data": {
    "entity_a": "player",
    "entity_b": "slime_01"
  }
}
```

Collision must be emitted before encounter/battle transitions where applicable.

This allows the harness to distinguish:

```text
movement
→ collision
→ encounter
→ battle
```

---

# 28. ENCOUNTER_STARTED

Example:

```json
{
  "type": "ENCOUNTER_STARTED",
  "data": {
    "enemy": "slime_01"
  }
}
```

This is distinct from `BATTLE_STARTED`.

This distinction allows future encounter systems to support:

* surprise attacks;
* scripted encounters;
* escape opportunities;
* pre-battle dialogue;
* encounter animations.

---

# 29. BATTLE_STARTED

Example:

```json
{
  "type": "BATTLE_STARTED",
  "data": {
    "enemy_ids": [
      "slime_01"
    ]
  }
}
```

Battle state must become inspectable immediately after this event.

---

# 30. BATTLE_ENDED

Example:

```json
{
  "type": "BATTLE_ENDED",
  "data": {
    "result": "VICTORY"
  }
}
```

Possible results:

```text
VICTORY
DEFEAT
ESCAPE
SCRIPTED
```

---

# 31. DAMAGE_DEALT

Example:

```json
{
  "type": "DAMAGE_DEALT",
  "data": {
    "source": "player",
    "target": "slime_01",
    "amount": 4,
    "remaining_hp": 6
  }
}
```

The event should expose the final applied damage, not merely the pre-randomized damage value.

---

# 32. STORY FLAGS

Story flags are semantic boolean state.

Example:

```text
MET_MAYOR
TOWN_ATTACK_STARTED
TOWN_ATTACK_COMPLETE
BOSS_DEFEATED
```

When a flag changes:

```json
{
  "type": "STORY_FLAG_SET",
  "data": {
    "flag": "TOWN_ATTACK_STARTED"
  }
}
```

Story flags must be inspectable.

---

# 33. SET_FLAG

Debug command:

```text
SET_FLAG <flag>
```

Example:

```text
SET_FLAG MET_MAYOR
```

This is a state-construction tool.

It must emit an appropriate debug event.

Scenarios should prefer setting initial flags through scenario configuration rather than issuing a sequence of debug commands.

---

# 34. CLEAR_FLAG

Command:

```text
CLEAR_FLAG <flag>
```

Example:

```text
CLEAR_FLAG TOWN_ATTACK_STARTED
```

---

# 35. TELEPORT

Command:

```text
TELEPORT <map> <x> <y>
```

Example:

```text
TELEPORT town 12 8
```

Teleportation is a debug-only state-construction operation.

It must not simulate player movement.

Therefore it must not emit `PLAYER_MOVED`.

Instead, it should emit a debug-specific event such as:

```text
DEBUG_TELEPORTED
```

This distinction prevents tests from confusing state setup with actual gameplay.

---

# 36. SET_HP

Command:

```text
SET_HP <entity> <value>
```

Example:

```text
SET_HP player 1
```

This is primarily a scenario/debug setup tool.

It must not emit `DAMAGE_DEALT` or `HEALING_APPLIED`, because no actual combat action occurred.

---

# 37. RNG

All gameplay randomness must pass through the game's RNG abstraction.

The debug protocol provides:

```text
SET_RNG <seed>
GET_RNG
```

Example:

```text
SET_RNG 12345
```

Inspect:

```text
GET_RNG
```

Result:

```json
{
  "seed": 12345
}
```

If random consumption is important for debugging, the harness may emit:

```text
RNG_USED
```

with enough information to reproduce the relevant result.

---

# 38. SET_RNG

`SET_RNG` must reset the deterministic RNG state.

Loading a scenario with:

```json
{
  "rng_seed": 12345
}
```

must have the same effect as:

```text
RESET
SET_RNG 12345
```

before the scenario state is established.

---

# 39. Audio State

The current audio state must be inspectable.

Example:

```json
{
  "audio": {
    "track": "BATTLE",
    "playing": true
  }
}
```

Track identifiers must be semantic:

```text
TITLE
FIELD
TOWN
BATTLE
VICTORY
DEFEAT
```

The actual musical implementation is irrelevant to the debug protocol.

---

# 40. MUSIC_CHANGED

When the music changes:

```json
{
  "type": "MUSIC_CHANGED",
  "data": {
    "from": "FIELD",
    "to": "BATTLE"
  }
}
```

This allows a scenario to verify that an encounter correctly changes the music without analyzing audio output.

---

# 41. Assertions

The protocol supports machine-readable assertions.

Examples:

```text
game.state == "BATTLE"
player.hp == 20
player.x == 12
player.y == 8
story.TOWN_ATTACK_STARTED == true
audio.track == "BATTLE"
```

The host-side test runner should perform most assertions rather than requiring complex expression evaluation inside the Game Boy.

---

# 42. Assertion Types

The initial assertion system should support:

```text
equals
not_equals
greater_than
less_than
greater_or_equal
less_or_equal
contains
exists
event_occurred
event_not_occurred
```

Example:

```json
{
  "assert": {
    "path": "game.state",
    "equals": "BATTLE"
  }
}
```

---

# 43. Scenario Actions

Scenario files should support an action sequence.

Example:

```json
{
  "actions": [
    {
      "press": "RIGHT"
    },
    {
      "wait": 5
    },
    {
      "inspect": true
    }
  ]
}
```

The exact schema may evolve.

Actions should be deterministic.

---

# 44. Scenario Assertions

Example:

```json
{
  "assertions": [
    {
      "path": "game.state",
      "equals": "BATTLE"
    },
    {
      "path": "audio.track",
      "equals": "BATTLE"
    },
    {
      "event_occurred": "ENCOUNTER_STARTED"
    },
    {
      "event_occurred": "BATTLE_STARTED"
    }
  ]
}
```

---

# 45. Scenario Test Result

A completed scenario returns:

```json
{
  "scenario": "first_encounter",
  "status": "PASS"
}
```

A failure must include diagnostics:

```json
{
  "scenario": "first_encounter",
  "status": "FAIL",

  "failure": {
    "assertion": "game.state == BATTLE",
    "expected": "BATTLE",
    "actual": "OVERWORLD"
  },

  "snapshot": {},
  "recent_events": []
}
```

---

# 46. LLM-Friendly Failure Output

The human-readable test runner should produce output such as:

```text
SCENARIO: first_encounter
STATUS: FAIL

ASSERTION:
  game.state == BATTLE

EXPECTED:
  BATTLE

ACTUAL:
  OVERWORLD

PLAYER:
  map: field
  position: (8,5)
  facing: EAST

ENEMY:
  slime_01
  position: (9,5)

RECENT EVENTS:
  120 PLAYER_MOVED
  121 PLAYER_MOVEMENT_BLOCKED

MISSING EVENTS:
  COLLISION
  ENCOUNTER_STARTED
  BATTLE_STARTED
```

An LLM should be able to diagnose the likely area of failure from this information.

---

# 47. Example: First Encounter Scenario

Scenario:

```json
{
  "name": "first_encounter",

  "rng_seed": 12345,

  "world": {
    "map": "field",
    "player": {
      "x": 8,
      "y": 5,
      "facing": "EAST"
    }
  },

  "entities": [
    {
      "id": "slime_01",
      "type": "enemy",
      "x": 9,
      "y": 5
    }
  ],

  "audio": {
    "track": "FIELD"
  },

  "actions": [
    {
      "press": "RIGHT"
    },
    {
      "wait": 10
    }
  ],

  "assertions": [
    {
      "path": "game.state",
      "equals": "BATTLE"
    },
    {
      "event_occurred": "COLLISION"
    },
    {
      "event_occurred": "ENCOUNTER_STARTED"
    },
    {
      "event_occurred": "BATTLE_STARTED"
    },
    {
      "path": "audio.track",
      "equals": "BATTLE"
    }
  ]
}
```

This verifies the entire chain:

```text
RIGHT
  ↓
movement
  ↓
collision
  ↓
encounter
  ↓
battle
  ↓
battle music
```

---

# 48. Example: Town Event Scenario

The scenario should place the player immediately before the trigger.

Example:

```json
{
  "name": "town_event_01",

  "rng_seed": 1001,

  "world": {
    "map": "town",
    "player": {
      "x": 12,
      "y": 8,
      "facing": "EAST"
    }
  },

  "story": {
    "flags": {
      "MET_MAYOR": true,
      "TOWN_ATTACK_STARTED": false
    }
  },

  "actions": [
    {
      "press": "RIGHT"
    },
    {
      "wait": 20
    }
  ],

  "assertions": [
    {
      "path": "story.TOWN_ATTACK_STARTED",
      "equals": true
    },
    {
      "event_occurred": "STORY_EVENT_STARTED"
    }
  ]
}
```

The test does not directly start the event.

It creates the conditions under which normal game logic must start it.

---

# 49. Event Ordering

Event ordering is significant.

For an encounter, the expected ordering might be:

```text
PLAYER_MOVED
COLLISION
ENCOUNTER_STARTED
GAME_STATE_CHANGED
BATTLE_STARTED
MUSIC_CHANGED
```

The exact order may evolve as implementation changes, but important ordering dependencies must be documented and tested.

The host harness must preserve event sequence numbers.

---

# 50. State vs Events

The harness must distinguish between:

### State

What is true now.

Example:

```text
game.state = BATTLE
```

### Event

What happened.

Example:

```text
BATTLE_STARTED
```

A current state does not prove how the game got there.

Events provide historical evidence.

Tests should use both where useful.

---

# 51. Debug-Only Events

Debug operations may emit events prefixed with:

```text
DEBUG_
```

Examples:

```text
DEBUG_TELEPORTED
DEBUG_HP_CHANGED
DEBUG_FLAG_CHANGED
DEBUG_SCENARIO_LOADED
```

These must never be confused with real gameplay events.

For example:

```text
DEBUG_TELEPORTED
```

does not mean:

```text
PLAYER_MOVED
```

---

# 52. Telemetry Ring Buffer

The Game Boy debug implementation must use a bounded ring buffer.

The initial implementation should target approximately 32 recent events.

Each event should be compact.

The ring buffer must never allocate memory dynamically for every event.

When old events are overwritten, the protocol must expose that information.

---

# 53. Telemetry Performance

Telemetry must not materially interfere with gameplay timing.

Avoid:

* expensive string formatting every frame;
* large allocations;
* full state serialization every frame;
* writing giant debug messages continuously.

Emit events at meaningful state boundaries.

For example:

Good:

```text
PLAYER_MOVED
```

Bad:

```text
PLAYER_POSITION_CHECKED
```

every frame.

---

# 54. Debug State Must Not Change Gameplay Semantics

The debug harness may:

* inspect;
* seed RNG;
* inject input;
* construct state;
* advance frames.

It must not silently alter normal gameplay rules.

For example, loading a scenario should use the same battle initialization code as a naturally occurring encounter.

---

# 55. Input Recording

The host runner should eventually support recording:

```text
scenario
rng seed
input sequence
frame timing
```

This creates a reproducible bug report.

Example:

```text
REPLAY:

ROM: rpg_card_proto_debug
SCENARIO: first_encounter
RNG: 12345

1. RIGHT
2. STEP 1
3. RIGHT
4. WAIT 20
```

A future replay system should be able to reproduce the same execution.

---

# 56. Future Save-State Integration

The protocol should be designed so that future emulator save-state support can be added without changing the semantic protocol.

Potential future commands:

```text
SAVE_STATE <name>
LOAD_STATE <name>
```

These should be considered transport/debug features, not gameplay features.

Scenario loading remains the preferred portable mechanism.

---

# 57. Future Card-System Integration

When the card battle system is implemented, the same protocol must expose:

```text
deck
hand
discard
draw_pile
turn
active_card
combo
damage
enemy_intent
```

Example:

```json
{
  "battle": {
    "turn": 3,
    "player_hp": 18,
    "enemy_hp": 12,

    "hand": [
      "fire_slash",
      "water_guard",
      "heal"
    ],

    "deck_count": 14,
    "discard_count": 6
  }
}
```

Card events should include:

```text
CARD_DRAWN
CARD_SELECTED
CARD_PLAYED
CARD_DISCARDED
COMBO_STARTED
COMBO_RESOLVED
```

The card system must remain testable without screenshot interpretation.

---

# 58. Future Story/Cutscene Integration

Future story systems should expose:

```text
dialogue_id
speaker
dialogue_state
cutscene_id
cutscene_step
story_flags
```

Events may include:

```text
DIALOGUE_STARTED
DIALOGUE_ADVANCED
DIALOGUE_ENDED

CUTSCENE_STARTED
CUTSCENE_STEP
CUTSCENE_ENDED
```

This allows an agent to test long narrative sequences without manually watching them.

---

# 59. Future NPC/AI Integration

NPC state should eventually expose:

```text
id
type
position
facing
behavior
current_target
state
```

AI transitions may emit:

```text
NPC_STATE_CHANGED
NPC_TARGET_CHANGED
NPC_MOVED
```

This allows bugs in NPC behavior to be tested deterministically.

---

# 60. Protocol Compatibility

Once a field, command, event name, or semantic identifier is used by automated scenarios, changing it may break the development API.

Prefer additive changes.

For breaking changes:

1. update the protocol version;
2. update affected scenarios;
3. update the host runner;
4. update `AGENTS.md`;
5. document the migration.

---

# 61. Protocol Version

The debug protocol should expose a version.

Example:

```json
{
  "protocol": {
    "name": "gameboy-rpg-debug",
    "version": 1
  }
}
```

The host should verify compatibility when connecting.

---

# 62. Connection Handshake

On connection, the debug target should identify itself.

Conceptual response:

```json
{
  "protocol": {
    "name": "gameboy-rpg-debug",
    "version": 1
  },

  "game": {
    "name": "rpg_card_proto",
    "build": "debug"
  }
}
```

This prevents the host from accidentally communicating with an incompatible ROM.

---

# 63. Error Codes

Errors must use stable identifiers.

Initial error codes:

```text
UNKNOWN_COMMAND
INVALID_ARGUMENT
SCENARIO_NOT_FOUND
SCENARIO_INVALID
ENTITY_NOT_FOUND
MAP_NOT_FOUND
INVALID_STATE
INVALID_BUTTON
INVALID_FRAME_COUNT
ASSERTION_FAILED
PROTOCOL_VERSION_MISMATCH
TRANSPORT_ERROR
TIMEOUT
```

Human-readable messages may accompany them.

The host should branch on error codes, not message strings.

---

# 64. Timeouts

The host runner must never wait indefinitely for the ROM.

Commands must have a timeout.

On timeout, report:

```text
TIMEOUT

COMMAND:
  INSPECT

LAST KNOWN FRAME:
  1832

LAST KNOWN STATE:
  OVERWORLD
```

The harness should preserve enough diagnostic information to determine whether the emulator, transport, or game stopped responding.

---

# 65. PASS/FAIL Contract

The canonical test result is:

```text
PASS
```

or:

```text
FAIL
```

The process exit code must be:

```text
0 = all tests passed
non-zero = at least one test failed
```

This allows:

```bash
make test-harness
```

to be used by both humans and automated coding agents.

---

# 66. Definition of Done

The debug protocol is considered implemented when an LLM can execute the following workflow:

```text
BUILD DEBUG ROM
      ↓
START EMULATOR
      ↓
LOAD SCENARIO
      ↓
INSPECT
      ↓
PRESS INPUT
      ↓
STEP / WAIT
      ↓
INSPECT
      ↓
EVENTS_SINCE
      ↓
ASSERT
      ↓
PASS / FAIL
```

For the first encounter:

```text
LOAD_SCENARIO first_encounter
        ↓
PRESS RIGHT
        ↓
WAIT
        ↓
INSPECT
        ↓
ENCOUNTER_STARTED
        ↓
BATTLE_STARTED
        ↓
MUSIC_CHANGED
        ↓
ASSERT
        ↓
PASS
```

For a future town event:

```text
LOAD_SCENARIO town_event_01
        ↓
PRESS movement
        ↓
WAIT
        ↓
INSPECT
        ↓
STORY_FLAG_SET
        ↓
STORY_EVENT_STARTED
        ↓
ASSERT
        ↓
PASS
```

---

# 67. Golden Rule

The development harness must always answer these five questions:

```text
WHERE AM I?
WHAT STATE IS THE GAME IN?
WHAT JUST HAPPENED?
WHAT HAPPENS NEXT?
DID THE EXPECTED BEHAVIOR OCCUR?
```

If an LLM cannot answer those questions from the harness, the harness is not sufficiently observable.

The long-term objective is therefore:

> **Every important gameplay system must expose enough deterministic semantic state and telemetry that an LLM can operate, test, diagnose, and reproduce the game without relying on human gameplay or visual interpretation.**

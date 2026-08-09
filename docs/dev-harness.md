# Game Boy RPG — LLM-Native Development & Testing Harness

## Implementation Specification — Harness Alpha 1

### 1. Purpose

Build a development harness around the existing Game Boy RPG prototype that allows an LLM-based development agent to:

1. Start the ROM in a known scenario.
2. Control the game programmatically.
3. Inspect semantic game state.
4. Inspect the spatial state of the current map.
5. Read a rolling event log.
6. Inspect audio/game-state transitions.
7. Control deterministic randomness.
8. Advance the game by controlled numbers of frames.
9. Run predefined scenarios.
10. Assert expected outcomes.
11. Receive machine-readable PASS/FAIL results.
12. Diagnose failures without needing to visually inspect the Game Boy screen.
13. Reproduce bugs deterministically.
14. Eventually support more sophisticated RPG systems such as towns, story events, NPCs, card combat, inventory, party members, bosses, and scripted sequences.

The harness must be designed so that the **LLM can understand what happened without relying on screenshots**.

Screenshots/visual inspection may be added later as a supplementary capability, but structured semantic state is the primary source of truth.

---

# 2. Current Game Assumptions

The existing prototype already has:

* GBDK-based Game Boy build.
* Working Nix development environment.
* Working ROM build.
* SameBoy available for emulation.
* Basic overworld.
* Basic player movement.
* Basic enemy.
* Collision/encounter.
* Basic battle loop.
* Basic music.
* No card system yet.

The harness must be added without requiring the existing gameplay to be rewritten from scratch.

Where the existing architecture does not have clean boundaries, refactor only what is necessary to create the interfaces described below.

---

# 3. Core Design Principle

The fundamental principle is:

> Every important gameplay system must be observable and controllable through semantic interfaces independent of its visual representation.

The LLM should never need to infer important state from pixels.

For example, this is insufficient:

```text
SCREENSHOT:
@       E
```

The harness should additionally expose:

```text
PLAYER:
  map: field
  position: (8,5)
  facing: EAST

ENEMY:
  id: slime_01
  position: (16,5)
  hp: 5/5

GAME:
  state: OVERWORLD
```

The visual representation is presentation.

The semantic state is authoritative.

---

# 4. Architecture

The system consists of two sides.

## 4.1 Game-side harness

Add:

```text
src/
├── debug/
│   ├── debug.c
│   ├── debug.h
│   ├── debug_protocol.c
│   ├── debug_protocol.h
│   ├── telemetry.c
│   ├── telemetry.h
│   ├── scenarios.c
│   ├── scenarios.h
│   ├── assertions.c
│   └── assertions.h
```

Existing systems should remain separate:

```text
src/
├── core/
├── world/
├── battle/
├── input/
├── audio/
├── ui/
└── debug/
```

## 4.2 Host-side harness

Add:

```text
tools/
├── dev.py
├── emulator.py
├── protocol.py
├── test_runner.py
├── scenario.py
└── scenarios/
    ├── new_game.json
    ├── first_encounter.json
    └── town_event_01.json
```

The host-side system is responsible for:

* building the ROM;
* launching SameBoy;
* communicating with the debug build;
* sending input;
* collecting telemetry;
* running scenarios;
* evaluating assertions;
* producing machine-readable results.

---

# 5. Debug Build vs Release Build

The project must support two configurations.

## Debug

```text
game-debug.gb
```

Includes:

* telemetry;
* scenario loader;
* debug commands;
* state inspection;
* event log;
* deterministic RNG;
* debug assertions;
* scenario test infrastructure;
* optional diagnostics.

## Release

```text
game.gb
```

Contains none of the development-only protocol/debug UI unless explicitly required.

The game must not depend on debug functionality to operate correctly.

Use a compile-time symbol such as:

```c
#ifdef DEBUG_BUILD
```

for code that genuinely must not exist in release builds.

Do not scatter `#ifdef DEBUG_BUILD` throughout gameplay code unnecessarily.

Prefer:

```text
game systems
    ↓
normal APIs

debug system
    ↓
uses normal APIs
```

rather than:

```text
game system
    ↓
if debug
    do special thing
```

---

# 6. Game State

Create a central state representation.

The exact structure may evolve, but the initial model should contain:

```c
typedef struct {
    GameState state;

    MapId map;

    Position player_position;
    Direction player_facing;

    uint8_t player_hp;
    uint8_t player_max_hp;

    uint32_t story_flags;

    uint32_t rng_seed;

    MusicTrack music;

    uint32_t frame;
} GameState;
```

Do not duplicate authoritative state inside the debug subsystem.

The debug subsystem reads and manipulates the real game state through controlled APIs.

---

# 7. Game State Machine

The initial states are:

```c
typedef enum {
    GAME_STATE_OVERWORLD,
    GAME_STATE_BATTLE
} GameState;
```

The architecture must allow future states:

```text
TITLE
OVERWORLD
DIALOGUE
MENU
BATTLE
CUTSCENE
GAME_OVER
PAUSE
```

The harness must report state transitions.

Example:

```text
GAME_STATE_CHANGED
  from: OVERWORLD
  to: BATTLE
```

---

# 8. Semantic Entity Model

Entities must have stable IDs.

Example:

```text
player
slime_01
mayor
town_guard_01
```

Do not identify entities only by their rendered character.

The fact that an enemy is currently rendered as:

```text
E
```

is presentation.

The semantic identity might be:

```text
entity_id: town_guard_01
entity_type: enemy
```

This becomes essential once real sprites replace ASCII rendering.

---

# 9. Position Model

Use a consistent world coordinate system.

```c
typedef struct {
    uint8_t x;
    uint8_t y;
} Position;
```

The debug representation must always report coordinates.

Example:

```text
PLAYER:
  map: town
  x: 12
  y: 8
  facing: EAST
```

Coordinates must not depend on screen coordinates.

---

# 10. World Inspection

Implement:

```text
INSPECT
```

The result should contain:

```text
GAME
  state
  frame
  map

PLAYER
  position
  facing
  hp

ENTITIES
  id
  type
  position
  visible
  relevant properties

WORLD
  nearby tiles
  nearby entities

STORY
  active flags

AUDIO
  current track
  playing state

EVENTS
  recent events
```

Example:

```text
GAME
  state: OVERWORLD
  frame: 18342
  map: town

PLAYER
  position: (12,8)
  facing: EAST
  hp: 20/20

ENTITIES
  mayor
    type: npc
    position: (16,8)
    visible: true

WORLD
  north: WALL
  south: FLOOR
  east: FLOOR
  west: FLOOR

STORY
  MET_MAYOR: true
  TOWN_ATTACK_STARTED: false

AUDIO
  track: TOWN
  playing: true
```

The format must be deterministic and stable.

---

# 11. Spatial Inspection

Implement a map representation suitable for an LLM.

Example:

```text
MAP: TOWN

    01234567890123456789
00  ####################
01  #..................#
02  #..................#
03  #.......@..........#
04  #..................#
05  #.............M....#
06  #..................#
07  ####################

LEGEND:
  @ = player
  M = mayor
  # = wall
  . = floor
```

The map output should be optional because full maps may eventually become large.

Implement:

```text
INSPECT AREA radius=5
```

as the preferred future interface.

Example:

```text
AREA CENTER=(12,8) RADIUS=5

   78901234567
7  ...........
8  .......@...
9  ...........
10 ...........
11 ........M..
```

---

# 12. Event Telemetry

Every important gameplay transition must produce an event.

Create:

```c
typedef enum {
    EVENT_PLAYER_MOVED,
    EVENT_COLLISION,
    EVENT_ENCOUNTER_STARTED,
    EVENT_BATTLE_STARTED,
    EVENT_BATTLE_ACTION,
    EVENT_DAMAGE_DEALT,
    EVENT_DAMAGE_RECEIVED,
    EVENT_ENTITY_DEFEATED,
    EVENT_BATTLE_WON,
    EVENT_BATTLE_LOST,
    EVENT_GAME_STATE_CHANGED,
    EVENT_STORY_FLAG_SET,
    EVENT_STORY_FLAG_CLEARED,
    EVENT_MAP_CHANGED,
    EVENT_MUSIC_CHANGED,
    EVENT_DIALOGUE_STARTED,
    EVENT_DIALOGUE_FINISHED
} GameEventType;
```

This list must be extensible.

Each event should contain:

* sequence number;
* frame number;
* event type;
* relevant IDs;
* relevant values.

Example:

```text
[00124] frame=18342
PLAYER_MOVED
  entity=player
  from=(12,8)
  to=(13,8)

[00125] frame=18342
COLLISION
  entity_a=player
  entity_b=town_guard_01

[00126] frame=18343
ENCOUNTER_STARTED
  enemy=town_guard_01

[00127] frame=18344
GAME_STATE_CHANGED
  from=OVERWORLD
  to=BATTLE

[00128] frame=18344
MUSIC_CHANGED
  from=TOWN
  to=BATTLE
```

---

# 13. Event Ring Buffer

Because Game Boy memory is limited, maintain a fixed-size ring buffer.

Initial target:

```text
32 events
```

The number should be configurable.

Old events may be discarded.

Expose:

```text
EVENTS
```

to retrieve the current buffer.

Also expose:

```text
EVENTS SINCE <sequence>
```

if practical.

This allows the host test runner to ask:

```text
What happened since my last action?
```

without repeatedly retrieving the entire game state.

---

# 14. Input API

The host must be able to send:

```text
PRESS UP
PRESS DOWN
PRESS LEFT
PRESS RIGHT
PRESS A
PRESS B
PRESS START
PRESS SELECT
```

Also:

```text
WAIT 1
WAIT 10
WAIT 60
```

The input interface must distinguish:

* pressed;
* held;
* released.

Initially the test runner only needs `PRESS` and `WAIT`.

---

# 15. Frame Control

Implement deterministic frame stepping in debug mode.

Commands:

```text
STEP 1
STEP 10
STEP 60
```

The emulator/test runner should be able to advance the game by exact numbers of frames.

This is required for diagnosing timing-dependent bugs.

Example:

```text
PRESS RIGHT
STEP 5
INSPECT
```

---

# 16. Scenario System

Scenarios are named deterministic starting states.

Initial scenarios:

```text
NEW_GAME
FIRST_ENCOUNTER
TOWN_ARRIVAL
TOWN_EVENT_01
```

The scenario API should be:

```c
void scenario_load(ScenarioId id);
```

Each scenario must:

1. reset game state;
2. establish required map;
3. establish player position;
4. establish entity state;
5. establish story flags;
6. establish party state where applicable;
7. establish HP;
8. establish deterministic RNG seed;
9. establish required audio state;
10. return control to normal game logic.

A scenario must not directly execute the behavior it is intended to test.

For example:

```text
BAD:

scenario_town_event_01()
    start_town_event();
```

Correct:

```text
scenario_town_event_01()
    map = TOWN
    player_position = trigger_position
    MET_MAYOR = true
    TOWN_ATTACK_STARTED = false
```

Then the normal game logic must detect the trigger.

This ensures the test actually tests the event.

---

# 17. Scenario Files

Initially scenarios may be implemented in C.

Once the scenario count grows, migrate definitions to data files.

Target format:

```json
{
  "name": "town_event_01",

  "initial_state": {
    "map": "town",

    "player": {
      "x": 12,
      "y": 8,
      "facing": "EAST",
      "hp": 20
    },

    "flags": {
      "MET_MAYOR": true,
      "TOWN_ATTACK_STARTED": false,
      "TOWN_ATTACK_COMPLETE": false
    },

    "rng_seed": 12345
  },

  "actions": [
    {
      "input": "RIGHT"
    },
    {
      "wait": 5
    }
  ],

  "assertions": [
    {
      "path": "game.state",
      "equals": "BATTLE"
    },

    {
      "path": "story.TOWN_ATTACK_STARTED",
      "equals": true
    },

    {
      "path": "audio.track",
      "equals": "BATTLE"
    }
  ]
}
```

Do not implement a complex scenario parser until the C-based system works.

---

# 18. Assertions

Assertions are the bridge between gameplay and automated testing.

Support:

```text
equals
not_equals
greater_than
less_than
contains
exists
event_occurred
event_not_occurred
```

Examples:

```text
game.state == BATTLE

player.hp == 20

story.TOWN_ATTACK_STARTED == true

audio.track == BATTLE

event_occurred("ENCOUNTER_STARTED")

event_not_occurred("BATTLE_WON")
```

---

# 19. Test Result Format

Every scenario test must return a structured result.

Example:

```json
{
  "scenario": "town_event_01",
  "status": "PASS",

  "assertions": [
    {
      "description": "battle started",
      "status": "PASS"
    },
    {
      "description": "town attack flag set",
      "status": "PASS"
    },
    {
      "description": "battle music started",
      "status": "PASS"
    }
  ],

  "events": [
    "PLAYER_MOVED",
    "COLLISION",
    "ENCOUNTER_STARTED",
    "GAME_STATE_CHANGED",
    "MUSIC_CHANGED"
  ]
}
```

On failure:

```json
{
  "scenario": "town_event_01",
  "status": "FAIL",

  "failure": {
    "assertion": "game.state == BATTLE",
    "expected": "BATTLE",
    "actual": "OVERWORLD"
  },

  "player": {
    "position": [16, 8]
  },

  "events": [
    "PLAYER_MOVED",
    "COLLISION"
  ],

  "diagnostic": "Collision occurred but no encounter transition occurred."
}
```

The diagnostic should be generated from known facts where possible rather than invented speculation.

---

# 20. Deterministic RNG

Implement a game RNG abstraction.

Do not call random functions directly from gameplay code.

Use:

```c
uint16_t game_random(void);
```

or equivalent.

The RNG must have:

```c
void rng_set_seed(uint32_t seed);
uint32_t rng_get_seed(void);
```

Every scenario must be able to specify a seed.

The current seed/state must be included in debug snapshots.

This makes future card draws, enemy AI, damage rolls, random encounters, and procedural behavior reproducible.

---

# 21. Snapshot

Implement:

```text
SNAPSHOT
```

which returns a complete semantic description of the current game.

Minimum fields:

```text
GAME
PLAYER
PARTY
MAP
ENTITIES
STORY FLAGS
BATTLE
AUDIO
RNG
FRAME
RECENT EVENTS
```

The snapshot should be sufficient to understand the game without seeing the screen.

---

# 22. Debug Commands

The first debug command set should be:

```text
HELP

LOAD_SCENARIO <name>

RESET

PRESS <button>

WAIT <frames>

STEP <frames>

INSPECT

INSPECT AREA <radius>

SNAPSHOT

EVENTS

EVENTS SINCE <sequence>

ASSERT <expression>

SET_FLAG <flag>

CLEAR_FLAG <flag>

TELEPORT <map> <x> <y>

SET_HP <entity> <value>

SET_RNG <seed>
```

Not all of these need to be exposed to the player-facing debug UI.

They are primarily an LLM/host API.

---

# 23. Debug UI

The ROM should have a minimal human-accessible debug menu for situations where an LLM is not driving it.

Example:

```text
DEBUG MENU

> SCENARIO
  TELEPORT
  FLAGS
  STATE
  EVENTS
  AUDIO
  RNG
```

However, this menu is secondary.

The machine-readable debug protocol is the authoritative development interface.

---

# 24. Host-Side Test Runner

Implement:

```bash
python tools/dev.py scenario town_event_01
```

Expected flow:

```text
1. Build debug ROM
2. Launch emulator
3. Wait for debug interface
4. Load scenario
5. Execute actions
6. Collect events/state
7. Evaluate assertions
8. Produce result
9. Shut down emulator
```

Also support:

```bash
python tools/dev.py test
```

to run all scenarios.

Output:

```text
Running 8 scenarios

✓ new_game
✓ first_encounter
✓ battle_victory
✓ battle_defeat
✓ town_arrival
✓ town_event_01
✓ town_event_repeat
✓ music_transition

8 passed
0 failed
```

---

# 25. Emulator Integration

The host-side harness must use the existing SameBoy development environment.

Do not hard-code machine-specific paths.

The Nix environment should provide the emulator.

The harness should launch the emulator through a configurable command.

Example conceptual configuration:

```text
SAMEBOY=...
ROM=build/game-debug.gb
```

The implementation should determine the exact SameBoy executable/interface available in the current environment rather than assuming a particular binary name.

---

# 26. Debug Communication Transport

The Game Boy ROM itself has no ordinary command-line interface.

Therefore the implementation must establish a host↔ROM debug transport.

The preferred implementation should use an emulator-supported mechanism available in the existing SameBoy environment.

Before implementing the transport, inspect the installed SameBoy version and determine the most reliable way to:

1. send controller input;
2. retrieve debug/telemetry output;
3. synchronize frame advancement;
4. detect ROM startup;
5. terminate the emulator.

Do not invent a protocol that cannot actually be supported by the chosen emulator.

If direct bidirectional communication is impractical through SameBoy, implement the first version around deterministic emulator control plus a host-visible telemetry mechanism, while keeping the `DebugProtocol` abstraction independent of the transport.

The transport must therefore be abstracted:

```text
DebugProtocol
      │
      ▼
Transport
      │
      ├── SameBoy implementation
      └── future hardware implementation
```

---

# 27. Debug Protocol Abstraction

Define a conceptual interface:

```text
connect()
disconnect()

send_command(command)

press(button)

wait(frames)

load_scenario(name)

inspect()

snapshot()

events()

step(frames)
```

The test runner must not know whether the underlying transport is:

* emulator;
* serial hardware;
* development cartridge;
* future custom tooling.

---

# 28. LLM-Friendly Output Rules

All machine-readable output must follow these rules:

### Stable names

Use:

```text
GAME_STATE_BATTLE
```

rather than:

```text
battle mode
```

### Stable IDs

Use:

```text
town_guard_01
```

rather than:

```text
enemy
```

### Explicit booleans

Use:

```text
true
false
```

rather than:

```text
yes
no
```

### Explicit coordinates

Always provide:

```text
x
y
map
```

### Explicit event names

Prefer:

```text
ENCOUNTER_STARTED
```

over:

```text
Encounter!
```

### No ambiguous output

Avoid:

```text
HP: okay
```

Prefer:

```text
HP: 18/20
```

---

# 29. LLM-Oriented Diagnostics

The harness should expose enough information for an agent to diagnose common failure classes.

Examples:

## Collision without transition

```text
COLLISION detected
ENCOUNTER_STARTED not detected
GAME_STATE remains OVERWORLD
```

## Event fired twice

```text
ENCOUNTER_STARTED count: 2
expected: 1
```

## Wrong music

```text
GAME_STATE_CHANGED: OVERWORLD → BATTLE
MUSIC_CHANGED: TOWN → TOWN
```

## Incorrect story flag

```text
event occurred: TOWN_ATTACK_STARTED
flag TOWN_ATTACK_STARTED: false
```

## Wrong location

```text
expected player position: (12,8)
actual player position: (12,9)
```

The harness should expose facts. It should not make unsupported claims about the cause.

---

# 30. First Required Scenario

Implement:

```text
town_event_01
```

Scenario setup:

```text
MAP = town

PLAYER:
  x = event trigger - appropriate position
  y = event trigger - appropriate position

FLAGS:
  MET_MAYOR = true
  TOWN_ATTACK_STARTED = false

RNG:
  seed = 12345
```

The scenario then performs the real movement necessary to trigger the event.

Expected sequence:

```text
PLAYER_MOVED
COLLISION
EVENT_TRIGGERED
STORY_FLAG_SET
ENCOUNTER_STARTED
GAME_STATE_CHANGED
MUSIC_CHANGED
```

Expected final state:

```text
GAME_STATE = BATTLE

TOWN_ATTACK_STARTED = true

AUDIO = BATTLE

ENEMY = expected enemy
```

This becomes the reference implementation for the harness.

---

# 31. Second Required Scenario

Implement:

```text
town_event_repeat
```

Setup:

```text
TOWN_ATTACK_STARTED = true
```

Attempt to trigger the same event again.

Expected:

```text
event does not restart
```

This establishes that scenarios can test negative behavior, not only happy paths.

---

# 32. Third Required Scenario

Implement:

```text
first_encounter
```

Expected:

```text
OVERWORLD
    ↓
player movement
    ↓
enemy collision
    ↓
ENCOUNTER_STARTED
    ↓
BATTLE
    ↓
BATTLE MUSIC
```

This validates the existing prototype against the new harness.

---

# 33. Test Categories

As the project grows, organize tests into:

```text
tests/
├── world/
├── movement/
├── encounters/
├── story/
├── battle/
├── cards/
├── audio/
└── integration/
```

Examples:

```text
movement_wall_collision
movement_enemy_collision
first_encounter
town_arrival
town_event_01
town_event_repeat
battle_basic_attack
battle_victory
battle_defeat
battle_music_transition
```

Every significant new system should eventually have at least one scenario.

---

# 34. No Visual Dependency

A test must not fail merely because the visual rendering changes.

For example:

```text
OLD:
E

NEW:
[SLIME]
```

must not affect:

```text
enemy.id == "slime_01"
```

The test should operate on semantics.

Likewise:

```text
@ → hero sprite
```

must not break movement tests.

---

# 35. Debugging Information Should Be Layered

Provide three levels.

## Level 1 — Summary

```text
STATE: BATTLE
PLAYER: 18/20 HP
ENEMY: 7/10 HP
```

## Level 2 — State

Full semantic snapshot.

## Level 3 — Trace

Frame-by-frame/event-by-event information.

This prevents every LLM interaction from producing enormous output.

---

# 36. Debug Trace

Add optional trace categories:

```text
TRACE_INPUT
TRACE_WORLD
TRACE_COLLISION
TRACE_STORY
TRACE_BATTLE
TRACE_AUDIO
TRACE_STATE
```

The host can request:

```text
TRACE collision
```

rather than dumping everything.

---

# 37. Performance and Memory Requirements

The debug harness must respect Game Boy constraints.

Avoid:

* dynamic allocation;
* unbounded logs;
* large strings;
* large JSON structures inside Game Boy RAM;
* expensive formatting on every frame.

The Game Boy-side representation should be compact.

Serialization should happen only when requested.

The host-side tool can do the expensive formatting.

---

# 38. Serialization Strategy

Do not construct giant JSON strings in Game Boy RAM.

Prefer a compact debug message protocol.

For example:

```text
EVENT
TYPE=COLLISION
A=player
B=town_guard_01
```

or a compact binary protocol if necessary.

The host can convert this into:

```json
{
  "type": "COLLISION",
  "entity_a": "player",
  "entity_b": "town_guard_01"
}
```

The exact transport format is less important than the semantic contract.

---

# 39. Development Commands

Add Make targets:

```text
make
make debug
make release
make test
make test-scenario SCENARIO=town_event_01
make test-scenario SCENARIO=first_encounter
```

Potentially:

```text
make run-debug
```

to launch the debug ROM manually.

---

# 40. Acceptance Criteria

The harness is considered complete when an LLM-driven development process can perform this sequence without human interaction:

```text
1. Build the debug ROM.

2. Launch the emulator.

3. Load scenario:
   town_event_01

4. Inspect initial state.

5. Issue movement commands.

6. Observe player coordinates.

7. Observe collision.

8. Observe event telemetry.

9. Observe game-state transition.

10. Observe story flag change.

11. Observe battle initialization.

12. Observe music transition.

13. Execute battle actions.

14. Observe HP changes.

15. Observe battle victory.

16. Observe return to overworld.

17. Verify music returned to overworld music.

18. Produce PASS/FAIL result.
```

The LLM should never need to ask a human:

> "What happened on the screen?"

---

# 41. Required Final Developer Experience

The intended workflow is:

```bash
make test-scenario SCENARIO=town_event_01
```

Result:

```text
SCENARIO: town_event_01
STATUS: PASS

INITIAL STATE
  map: town
  player: (12,8)
  flags:
    MET_MAYOR=true
    TOWN_ATTACK_STARTED=false

ACTIONS
  RIGHT
  RIGHT
  RIGHT
  RIGHT

EVENTS
  PLAYER_MOVED
  PLAYER_MOVED
  PLAYER_MOVED
  PLAYER_MOVED
  COLLISION
  TOWN_EVENT_TRIGGERED
  STORY_FLAG_SET
  ENCOUNTER_STARTED
  GAME_STATE_CHANGED
  MUSIC_CHANGED

FINAL STATE
  game_state: BATTLE
  player_hp: 20/20
  enemy: town_guard_01
  enemy_hp: 10/10
  town_attack_started: true
  music: BATTLE

RESULT
  PASS
```

If it fails:

```text
SCENARIO: town_event_01
STATUS: FAIL

FAILED ASSERTION
  game_state == BATTLE

EXPECTED
  BATTLE

ACTUAL
  OVERWORLD

LAST EVENTS
  PLAYER_MOVED
  PLAYER_MOVED
  COLLISION

STATE
  player: town(16,8)
  enemy: town_guard_01
  flags:
    MET_MAYOR=true
    TOWN_ATTACK_STARTED=false

DIAGNOSTIC FACTS
  collision occurred
  encounter event did not occur
  game state did not change
  battle music did not start
```

This is the minimum quality bar for the harness.

---

# 42. Implementation Order

Implement in this exact order.

## Phase 1 — State observability

1. Centralize game state.
2. Add semantic entity IDs.
3. Add position/map information.
4. Add game-state reporting.
5. Add player/enemy reporting.

**Deliverable:** `SNAPSHOT` works.

---

## Phase 2 — Event telemetry

1. Create event types.
2. Create ring buffer.
3. Emit movement events.
4. Emit collision events.
5. Emit encounter events.
6. Emit state-change events.
7. Emit music-change events.
8. Expose `EVENTS`.

**Deliverable:** LLM can explain what happened.

---

## Phase 3 — Deterministic control

1. Abstract input.
2. Add frame stepping.
3. Add deterministic RNG.
4. Add `PRESS`.
5. Add `WAIT`.
6. Add `STEP`.

**Deliverable:** Same sequence produces same result.

---

## Phase 4 — Scenarios

1. Create scenario abstraction.
2. Implement scenario reset.
3. Implement `NEW_GAME`.
4. Implement `FIRST_ENCOUNTER`.
5. Implement `TOWN_ARRIVAL`.
6. Implement `TOWN_EVENT_01`.

**Deliverable:** Game can start from known states.

---

## Phase 5 — Assertions

1. Implement state assertions.
2. Implement event assertions.
3. Implement flag assertions.
4. Implement audio assertions.
5. Implement positional assertions.
6. Produce structured PASS/FAIL results.

**Deliverable:** Scenarios become automated tests.

---

## Phase 6 — Host runner

1. Launch SameBoy.
2. Establish debug transport.
3. Send input.
4. Synchronize frames.
5. Retrieve telemetry.
6. Execute scenario files.
7. Evaluate assertions.
8. Print results.
9. Exit with appropriate process status.

**Deliverable:**

```bash
make test-scenario SCENARIO=town_event_01
```

actually works end-to-end.

---

## Phase 7 — LLM interface

1. Define stable command vocabulary.
2. Define stable output format.
3. Add `INSPECT`.
4. Add `SNAPSHOT`.
5. Add `EVENTS`.
6. Add scenario loading.
7. Add failure diagnostics.
8. Document protocol.

**Deliverable:** An LLM can operate the game without human interpretation.

---

# 43. Future Extensions

The architecture must leave room for:

```text
DEBUG
├── Scenarios
├── Story Flags
├── Teleport
├── Party
├── Inventory
├── Equipment
├── Cards
├── Decks
├── Battle
├── RNG
├── Audio
├── Map
├── NPCs
├── Dialogue
├── Save State
└── Performance
```

Eventually the LLM should be able to request:

```text
load scenario card_combo_fire_slash

inspect

press A
inspect

press RIGHT
press A
inspect

events
```

and understand:

```text
CARD_SELECTED
CARD_ADDED_TO_SEQUENCE
COMBO_RESOLVED
DAMAGE_DEALT
STATUS_EFFECT_APPLIED
```

without requiring special-purpose test code.

---

# 44. Non-Goals

Do not implement yet:

* card testing;
* sophisticated AI;
* visual screenshot analysis;
* full save-state support;
* network multiplayer;
* hardware serial debugging;
* complex scripting language;
* elaborate debug UI;
* production-quality JSON serialization inside the ROM.

The immediate objective is:

> **A deterministic, observable, controllable, scenario-driven development environment around the existing RPG prototype.**

---

# 45. Definition of Success

The harness is successful when the development agent can encounter a failure such as:

```text
town_event_01 FAILED
```

and, without playing the game manually, determine:

```text
The player reached the trigger.
Collision was detected.
The expected story event did not fire.
The story flag remains false.
The game remained in OVERWORLD.
Battle music did not start.
```

From that information, the LLM should be able to locate the relevant gameplay code, make a change, rebuild the ROM, rerun the exact same scenario, and determine whether the fix worked.

That is the core objective of the entire harness.

**The game screen is for the player.
The semantic state/event interface is for the development agent.**

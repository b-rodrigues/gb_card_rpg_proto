# Agent Operating Instructions (AGENTS.md)

This file defines the operational contract and constraints for AI coding agents working on this project.

The project is an experimental Game Boy RPG. The development workflow is intentionally **LLM-first**: AI coding agents must be able to build, execute, inspect, test, and debug gameplay through deterministic machine-readable interfaces without relying on a human to play the game or interpret the screen.

---

# 1. Environment

This repository uses Nix flakes for complete, reproducible environment management.

Do not install dependencies manually using `apt`, `brew`, `npm`, `pip`, or other host package managers.

Enter the development environment with:

```bash
nix develop
```

All development commands must work inside the Nix development environment.

Do not introduce dependencies that are unavailable from the project's Nix environment without first updating the flake appropriately.

---

# 2. Primary Commands

All common operations are exposed through standard `make` targets.

## Build Release ROM

```bash
make release
```

Produces:

```text
build/rpg_card_proto.gb
```

The release ROM must not depend on debug-only functionality.

---

## Build Debug ROM

```bash
make debug
```

Produces:

```text
build/rpg_card_proto_debug.gb
```

The debug ROM includes development harness functionality such as:

* telemetry;
* semantic state inspection;
* event logging;
* deterministic RNG;
* scenario loading;
* assertions;
* debug input;
* development diagnostics.

---

## Run All Harness Scenario Tests

```bash
make test-harness
```

This must:

1. build the debug ROM;
2. execute the host-side harness;
3. run all scenarios in `tools/scenarios/`;
4. evaluate their assertions;
5. return a non-zero exit code if any scenario fails.

Equivalent underlying command:

```bash
python3 tools/dev.py test
```

---

## Run One Harness Scenario

```bash
make test-scenario SCENARIO=first_encounter
```

Example:

```bash
make test-scenario SCENARIO=town_event_01
```

The scenario must be deterministic and reproducible.

---

## Automated ROM Validation

```bash
make test
```

This validates the release build, including compilation, linking, ROM header, and checksum integrity.

---

## Run in Emulator

```bash
make run
```

Builds the ROM if necessary and launches it in the configured emulator.

---

## Capture Visual Screenshot

```bash
make screenshot
```

Produces:

```text
build/screenshot.png
```

Screenshots are useful for visual validation, but they are **not the primary debugging interface**.

Agents must inspect semantic state and telemetry first.

---

## Clean Build Artifacts

```bash
make clean
```

Removes generated artifacts in:

```text
build/
```

## Debug Harness & Protocol

The development harness is a first-class part of the project and must be used for gameplay development and validation.

The authoritative specification for the debug interface is:

```text
docs/DEBUG_PROTOCOL.md
```

Before modifying or extending the debug harness, telemetry system, scenario system, debug commands, or automated gameplay testing, read `docs/DEBUG_PROTOCOL.md`.

The protocol defines:

* Debug ROM behavior
* Scenario loading and state initialization
* Deterministic input injection
* Frame stepping
* Game-state inspection
* Entity/world inspection
* Telemetry events and sequence numbers
* Story flag inspection
* Battle-state inspection
* Audio-state inspection
* RNG control
* Assertions
* Scenario PASS/FAIL behavior
* LLM-oriented diagnostic output

### Agent Requirements

1. **Use the harness instead of manually playing through the game whenever a scenario can reproduce the behavior being tested.**
2. **Create or update a scenario when implementing important gameplay behavior that should be regression-tested.**
3. **Ensure important gameplay state transitions emit semantic telemetry events.**
4. **Ensure important gameplay state is exposed through `INSPECT`/`SNAPSHOT` where an automated agent needs it to diagnose behavior.**
5. **Use deterministic RNG seeds for scenarios involving randomness.**
6. **Do not make tests depend on screenshots when semantic debug state can provide the required information.**
7. **When a scenario fails, inspect the state and telemetry before modifying gameplay code.**
8. **Keep debug operations separate from real gameplay events.** For example, a debug teleport must not emit `PLAYER_MOVED`.
9. **Treat `docs/DEBUG_PROTOCOL.md` as the protocol contract.** If implementation and documentation disagree, resolve the discrepancy rather than silently inventing a new behavior.
10. **When adding a new gameplay system, consider its observability requirements as part of the implementation, not as a later debugging task.**

The goal is for an LLM to be able to reproduce, execute, inspect, and diagnose gameplay scenarios without playing through the entire game manually.

---

# 3. Target Platform & Toolchain

* Target Hardware: Nintendo Game Boy (DMG) / Game Boy Color (CGB)
* C Toolchain: GBDK-4 (`lcc`)
* Assembly Toolchain: RGBDS (`rgbasm`, `rgblink`, `rgbfix`)
* Development Emulator: SameBoy
* Build Environment: Nix

The code must remain compatible with the actual Game Boy target.

Do not accidentally introduce desktop-only assumptions into gameplay code.

---

# 4. Project Structure

The project is organized by gameplay responsibility.

```text
src/
├── main.c
│
├── core/
│   ├── game.c
│   └── state.c
│
├── world/
│   ├── world.c
│   └── entity.c
│
├── battle/
│   ├── battle.c
│   └── combatant.c
│
├── input/
│   └── input.c
│
├── audio/
│   └── audio.c
│
├── ui/
│   └── ui.c
│
└── debug/
    ├── telemetry.c
    ├── rng.c
    ├── scenarios.c
    └── assertions.c

tools/
├── dev.py
├── test_runner.py
├── emulator.py
└── scenarios/
    └── *.json

build/
└── generated artifacts
```

The exact file structure may evolve, but responsibilities must remain separated.

---

# 5. Core Code Philosophy

## 5.1 Simple C

Prefer clear, explicit C over complex macro systems, excessive indirection, or clever abstractions.

Game Boy code should be understandable to another developer or coding agent.

---

## 5.2 Small Functions

Functions should have one clear responsibility.

Prefer:

```c
battle_start();
battle_update();
battle_end();
```

over giant functions containing an entire gameplay system.

---

## 5.3 Explicit State

Represent gameplay state using plain C structs and enums.

Prefer:

```c
typedef enum {
    GAME_STATE_OVERWORLD,
    GAME_STATE_BATTLE
} GameState;
```

over implicit state encoded through unrelated booleans.

---

## 5.4 Game Boy Native

Be conscious of:

* 8-bit arithmetic;
* RAM limits;
* VRAM access;
* VBlank timing;
* tile memory;
* stack usage;
* ROM/RAM layout.

Do not write desktop-style code and assume the compiler will make it appropriate for Game Boy hardware.

---

## 5.5 No Unnecessary Dependencies

Do not introduce modern game engines, scripting runtimes, JavaScript environments, Python runtime dependencies, or external libraries into the ROM.

Host-side tooling may use Python standard-library functionality where appropriate.

---

# 6. LLM-First Development Principle

The development harness is a **first-class game subsystem**.

The goal is not merely to provide convenient debugging tools for humans.

The goal is:

> An LLM must be able to start the game in a known state, control it, inspect what happened, determine whether behavior was correct, and reproduce failures without needing a human to interpret the screen.

The game screen is the player-facing interface.

The semantic debug interface is the development-agent-facing interface.

---

# 7. Semantic Observability Is Authoritative

When debugging or testing, prefer:

1. structured game state;
2. telemetry events;
3. scenario results;
4. semantic map/entity information;
5. screenshots.

Do not infer gameplay state from pixels when authoritative semantic information is available.

For example, this is insufficient:

```text
SCREEN:
@     E
```

The harness should expose information such as:

```text
GAME:
  state: OVERWORLD

PLAYER:
  map: field
  position: (8,5)
  facing: EAST

ENTITY:
  id: slime_01
  type: enemy
  position: (9,5)
```

Visual rendering is presentation.

Semantic state is authoritative.

---

# 8. Every Important Gameplay Event Must Be Observable

The following kinds of behavior MUST emit telemetry:

* game-state transitions;
* map transitions;
* player movement;
* collisions;
* encounter detection;
* encounter start;
* battle start;
* battle actions;
* damage;
* healing;
* entity defeat;
* battle victory;
* battle defeat;
* story event triggers;
* story flag changes;
* scripted event triggers (`SCRIPT_TRIGGERED` with the stable `EventId`);
* dialogue start/end;
* card actions;
* deck changes;
* random outcomes when relevant;
* audio track changes.

Use:

```c
telemetry_emit(...)
```

or the project's equivalent telemetry API.

Do not create important gameplay behavior that is invisible to the harness.

---

# 9. Telemetry Events

Events must have stable semantic names.

Prefer:

```text
PLAYER_MOVED
COLLISION
ENCOUNTER_STARTED
BATTLE_STARTED
DAMAGE_DEALT
STORY_FLAG_SET
GAME_STATE_CHANGED
MUSIC_CHANGED
```

Do not use vague messages such as:

```text
"something happened"
"battle!"
"hit"
```

Events should contain relevant structured information.

Example:

```text
PLAYER_MOVED
  entity: player
  from: (12,8)
  to: (13,8)
```

Example:

```text
COLLISION
  entity_a: player
  entity_b: town_guard_01
```

Example:

```text
GAME_STATE_CHANGED
  from: OVERWORLD
  to: BATTLE
```

---

# 10. Telemetry Ring Buffer

Game Boy memory is limited.

Telemetry must use a bounded ring buffer.

The initial target is approximately 32 recent events, unless the implementation demonstrates that another size is more appropriate.

Telemetry must never grow without bound.

The harness should expose recent events and, where supported:

```text
EVENTS SINCE <sequence>
```

This allows an LLM to inspect only what happened after its previous action.

---

# 11. Stable Entity IDs

Gameplay entities must have semantic IDs.

Examples:

```text
player
slime_01
town_guard_01
mayor
boss_01
```

Do not identify entities by their visual representation.

An enemy being rendered as:

```text
E
```

does not make `"E"` its identity.

The semantic entity may be:

```text
id: town_guard_01
type: enemy
```

This ensures tests continue working when ASCII graphics are eventually replaced by sprites.

---

# 12. Explicit Coordinates

World entities must have inspectable world coordinates.

Always report:

```text
map
x
y
```

and, where relevant:

```text
facing
```

Example:

```text
PLAYER
  map: town
  x: 12
  y: 8
  facing: EAST
```

Coordinates must represent world/game coordinates, not screen-pixel coordinates.

---

# 13. Machine-Readable Game Inspection

The debug harness must support semantic inspection.

Conceptual commands include:

```text
INSPECT
INSPECT AREA <radius>
SNAPSHOT
EVENTS
```

A snapshot should expose enough information for an LLM to understand the current gameplay state without seeing the screen.

Minimum information:

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

Do not add large irrelevant data to every response.

Prefer concise, layered information.

---

# 14. Spatial Inspection

The harness must support a machine-readable representation of the current map/area.

For the current ASCII prototype, an ASCII representation is acceptable and useful.

Example:

```text
    01234567890123456789
00  ####################
01  #..................#
02  #..................#
03  #.......@..........#
04  #..................#
05  #.............E....#
06  #..................#
07  ####################
```

The harness should additionally provide semantic entity information.

Do not make spatial understanding dependent solely on ASCII.

---

# 15. Input Control

The development harness must support programmatic input.

At minimum:

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

It must also support:

```text
WAIT <frames>
STEP <frames>
```

The test runner must be able to reproduce the exact same sequence of inputs.

---

# 16. Frame Control

Debug builds must support deterministic frame advancement.

Examples:

```text
STEP 1
STEP 10
STEP 60
```

This is required for investigating:

* timing bugs;
* state transitions;
* animation logic;
* audio transitions;
* input problems;
* race-like behavior between systems.

---

# 17. Deterministic Randomness

All gameplay randomness must go through a game RNG abstraction.

Do not directly use uncontrolled random functions throughout gameplay code.

The debug harness must support:

```text
SET_RNG <seed>
```

and scenarios must be able to define an initial seed.

The current RNG state/seed must be inspectable.

The same scenario, seed, and input sequence must produce the same behavior.

This becomes particularly important for:

* enemy behavior;
* damage;
* card draws;
* deck shuffling;
* random encounters;
* loot;
* critical hits;
* future procedural systems.

---

# 18. Scenarios Are First-Class Test Fixtures

A scenario represents a deterministic, named gameplay situation.

Examples:

```text
new_game
first_encounter
town_arrival
town_event_01
town_event_repeat
battle_basic
battle_victory
```

A scenario must establish state rather than bypass the behavior being tested.

Bad:

```text
scenario_town_event_01()
{
    start_town_event();
}
```

Good:

```text
scenario_town_event_01()
{
    set_map(TOWN);
    set_player_position(...);
    set_flag(MET_MAYOR);
    clear_flag(TOWN_ATTACK_STARTED);
}
```

Then normal game logic must detect the trigger and start the event.

This distinction is mandatory.

---

# 19. Every New Gameplay Feature Should Have a Scenario

When adding a significant gameplay feature, agents should add at least one reproducible scenario demonstrating it.

Examples:

```text
movement_wall_collision
first_encounter
town_arrival
town_event_01
town_event_repeat
battle_basic_attack
battle_victory
battle_defeat
card_draw
card_play
card_combo
boss_phase_02
```

The scenario becomes executable documentation of the feature.

---

# 20. Scenario Determinism

A scenario must specify enough state to reproduce its behavior.

This may include:

```text
map
player position
player facing
HP
party
inventory
story flags
enemy state
battle state
RNG seed
audio state
```

Do not rely on whatever state happened to remain in RAM from a previous test.

Every scenario must begin from a known state.

---

# 21. Assertions

Scenarios must support machine-checkable assertions.

Examples:

```text
game.state == BATTLE

player.hp == 20

story.TOWN_ATTACK_STARTED == true

audio.track == BATTLE

player.position == (12,8)

event_occurred("ENCOUNTER_STARTED")
```

Supported assertion categories should include:

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

Assertions must produce clear expected/actual information.

---

# 22. Scenario Test Results

A scenario must return one of:

```text
PASS
FAIL
```

Failure output must contain enough information for an LLM to diagnose the problem.

Example:

```text
SCENARIO: town_event_01
STATUS: FAIL

FAILED ASSERTION:
  game.state == BATTLE

EXPECTED:
  BATTLE

ACTUAL:
  OVERWORLD

PLAYER:
  map: town
  position: (16,8)

RECENT EVENTS:
  PLAYER_MOVED
  COLLISION

STORY:
  TOWN_ATTACK_STARTED: false

AUDIO:
  track: TOWN
```

The harness should report facts.

Do not fabricate explanations that cannot be supported by telemetry.

---

# 23. Host-Side Test Runner

The host-side harness in `tools/` is responsible for:

1. building the debug ROM;
2. launching SameBoy;
3. establishing the debug transport;
4. loading scenarios;
5. sending inputs;
6. advancing frames;
7. retrieving semantic state;
8. retrieving telemetry;
9. evaluating assertions;
10. producing machine-readable and human-readable results;
11. returning a meaningful process exit code.

The test runner must be independent of the game's visual presentation.

---

# 24. Emulator Transport

The Game Boy does not have a normal command-line interface.

Therefore the host-side harness must use an emulator-supported communication/control mechanism.

The transport must be isolated behind an abstraction.

Conceptually:

```text
DebugProtocol
      |
      v
Transport
      |
      +-- SameBoy implementation
      |
      +-- future hardware implementation
```

Do not spread SameBoy-specific details throughout gameplay or test logic.

Before changing the transport, inspect the actual SameBoy version/configuration available in the Nix environment.

Never assume an emulator feature exists without verifying it.

---

# 25. Debug Protocol

The development protocol should expose operations equivalent to:

```text
connect()
disconnect()

load_scenario(name)

press(button)

wait(frames)

step(frames)

inspect()

snapshot()

events()

assert(expression)
```

The exact wire format may change.

The semantic contract must remain stable.

---

# 26. Release/Debug Separation

Debug functionality must not alter the normal game architecture.

Prefer:

```text
GAME SYSTEMS
    |
    +---- normal gameplay APIs
    |
DEBUG HARNESS
    |
    +---- observes and controls normal systems
```

Avoid embedding large amounts of test-specific behavior directly into gameplay logic.

Debug code may construct state and invoke normal APIs.

It should not create an entirely separate implementation of gameplay.

---

# 27. Debug Commands

The initial debug command vocabulary should include:

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

Commands should have stable names and predictable results.

---

# 28. Debug UI

A minimal in-ROM debug UI may exist for human developers.

It may expose:

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

However, this UI is secondary.

The machine-readable harness is the primary development interface.

---

# 29. Layered Diagnostics

Diagnostics should have three levels.

## Summary

```text
STATE: BATTLE
PLAYER: 18/20 HP
ENEMY: 7/10 HP
```

## Full semantic state

All relevant state.

## Trace

Frame/event-level information.

Do not dump enormous amounts of information when a concise answer is sufficient.

Agents should be able to request more detail when necessary.

---

# 30. Trace Categories

Where useful, support selectively enabled trace categories:

```text
TRACE_INPUT
TRACE_WORLD
TRACE_COLLISION
TRACE_STORY
TRACE_BATTLE
TRACE_AUDIO
TRACE_STATE
```

This allows targeted investigation without overwhelming the test output.

---

# 31. Game Boy Memory Constraints

The harness must respect the target hardware.

Avoid:

* dynamic allocation where unnecessary;
* unbounded logs;
* giant strings;
* giant JSON buffers in Game Boy RAM;
* formatting large diagnostic reports every frame;
* unnecessary per-frame telemetry.

The Game Boy-side representation should remain compact.

The host-side tools should perform expensive formatting and aggregation.

---

# 32. Serialization

Do not build large JSON documents inside Game Boy RAM unless there is a demonstrated need.

Prefer compact debug messages/events.

The host-side tool may transform compact messages into structured JSON.

For example, Game Boy-side:

```text
EVENT
TYPE=COLLISION
A=player
B=town_guard_01
```

Host-side:

```json
{
  "type": "COLLISION",
  "entity_a": "player",
  "entity_b": "town_guard_01"
}
```

The semantic meaning is more important than the exact serialization format.

---

# 33. Story State

Story progression should be represented explicitly using stable flags or equivalent state.

Examples:

```text
MET_MAYOR
HAS_MAGIC_STONE
TOWN_ATTACK_STARTED
TOWN_ATTACK_COMPLETE
BOSS_DEFEATED
```

Story state must be inspectable.

Story state must be scenario-configurable.

Story transitions must emit telemetry.

---

# 34. Audio Observability

Audio state must be represented semantically.

For example:

```text
AUDIO
  track: TOWN
  playing: true
```

When music changes:

```text
MUSIC_CHANGED
  from: TOWN
  to: BATTLE
```

Tests should assert audio state semantically rather than attempting to analyze recorded audio.

---

# 35. Hardware VBlank Sound Timing

Never update music step timers directly inside the main `while(1)` loop.

Main-loop CPU variations can cause music to play at variable tempos between menus and gameplay.

Music must run on the **hardware VBlank interrupt**, driven by a dedicated
RAM-resident ISR installed by the custom CRT0:

* `src/crt0.s` rewrites the VBlank vector (`0x0040`) to `JP 0xC900` (WRAM,
  always mapped regardless of ROM bank).
* At boot `init` copies a small ISR (`vbl_isr`) from ROM to WRAM `0xC900`
  and enables VBlank IE (`IE |= 0x01`).  The ISR calls `_audio_update`
  directly (a baked-in `call`, so no function pointers / banked-call
  helpers) and `reti`s.
* `audio_update`/`play_note` and the note tables must stay in the **fixed
  bank 0** (`< 0x4000`) so the ISR's `call` target and table reads are
  always mapped.
* `main.c` calls `audio_init()` then `enable_interrupts()` (enables IME);
  the ISR and IE are already set up by CRT0.

Do NOT use `add_VBL()`: the custom CRT0 stubs it (the GBDK interrupt
manager is RAM-resident and the harness skips CRT0), so it is a no-op.

Always call:

```c
enable_interrupts();
```

after boot init.

Audio transitions must emit telemetry.

---

# 36. Targeted Redrawing

Avoid calling full-screen clears:

```c
ui_clear_screen();
```

during frequent interactive events such as:

* menu navigation;
* cursor movement;
* player movement;
* UI updates.

Full clears are appropriate for major screen transitions such as:

```text
TITLE -> OVERWORLD
OVERWORLD -> BATTLE
BATTLE -> OVERWORLD
```

Use incremental tile updates wherever practical.

Visual performance should not compromise semantic game state.

---

# 37. Joypad Startup State

In:

```text
input_init()
```

initialize both:

```text
pad_state
prev_pad_state
```

to the current hardware:

```c
joypad()
```

Leaving:

```text
prev_pad_state = 0
```

can cause `input_pressed()` to incorrectly report a button press during boot.

Debug input injection must use the same input abstractions as normal input wherever practical.

---

# 38. Game Boy Color Palette & Attribute Mapping

Use:

```text
-Wm-yc
```

and appropriate:

```text
rgbfix -C
```

header flags for CGB compatibility.

Header byte `0x143` should indicate dual compatibility appropriately.

Before initializing CGB palettes:

```c
if (_cpu == CGB_TYPE)
```

Always reset:

```c
VBK_REG = 0;
```

after writing tile attributes to VRAM Bank 1.

Do not assume CGB hardware when running on DMG.

---

# 39. SDCC / GBDK C89 Rules

GBDK-4 uses an SDCC C89-style language environment.

Declare variables at the beginning of function blocks.

Avoid:

```c
if (condition) {
    uint8_t value = ...;
}
```

Prefer:

```c
uint8_t value;

if (condition) {
    value = ...;
}
```

Do not use C99/C11 features unless verified to compile correctly with the project's exact toolchain.

Avoid non-constant array initializers unsupported by the target compiler.

---

# 40. Screenshot Capture

Automated screenshot capture should allow sufficient startup time for the Game Boy boot sequence.

Allow at least:

```text
sleep 4
```

before capturing screenshots unless the capture system has a more reliable readiness signal.

Screenshots are for visual verification only.

Do not use screenshots as the primary automated gameplay assertion mechanism when semantic telemetry is available.

---

# 41. Agent Workflow

When modifying gameplay code, agents should follow this process:

## Step 1 — Understand

Inspect:

* relevant source files;
* current game state;
* existing telemetry;
* relevant scenarios;
* related tests.

Do not immediately modify code.

---

## Step 2 — Reproduce

Run the smallest relevant scenario.

Example:

```bash
make test-scenario SCENARIO=town_event_01
```

If no scenario exists, create one before implementing complex behavior when practical.

---

## Step 3 — Observe

Inspect:

* snapshot;
* telemetry;
* event sequence;
* assertion failure;
* RNG state;
* relevant game state.

Do not rely on visual inspection unless the problem is specifically visual.

---

## Step 4 — Implement

Make the smallest architectural change that fixes the behavior.

Preserve existing semantic interfaces.

Do not bypass normal game logic merely to make a scenario pass.

---

## Step 5 — Add/Update Tests

Every bug fix involving gameplay behavior should add or update a scenario or assertion that would have caught the bug.

The test should fail before the fix and pass after it whenever practical.

---

## Step 6 — Validate

Run:

```bash
make test-harness
```

then:

```bash
make test
```

If rendering or UI changed, also run:

```bash
make screenshot
```

or:

```bash
make run
```

---

# 42. Scenario-Driven Development

For substantial gameplay features, follow this development pattern:

```text
Scenario
    ↓
Expected behavior
    ↓
Implementation
    ↓
Telemetry
    ↓
Assertions
    ↓
Automated test
```

Do not wait until the end of a feature to think about testability.

The scenario is part of the feature.

---

# 43. Bug Reproduction

When an agent discovers a bug that depends on a particular state:

1. Create a deterministic scenario reproducing it.
2. Record the relevant RNG seed.
3. Record relevant story flags.
4. Record map/player/entity state.
5. Record the input sequence.
6. Record the expected result.
7. Add the scenario to the test suite.
8. Fix the bug.
9. Confirm the scenario passes.

A bug that cannot be reproduced should be treated as a development problem in its own right.

---

# 44. No "Magic" Test Fixes

Never make a scenario pass by adding test-only shortcuts to gameplay behavior.

For example, do not:

```text
if (debug && scenario == TOWN_EVENT_01)
    start_event();
```

Instead, construct the appropriate initial state and exercise the real gameplay path.

The harness must test the same logic used by the player.

---

# 45. Scenario Naming

Use stable, descriptive names.

Prefer:

```text
first_encounter
town_arrival
town_event_01
town_event_repeat
battle_basic_attack
battle_victory
```

Avoid:

```text
test1
foo
debugtest
newtest
```

Scenario names become part of the project's development API.

---

# 46. Test Output Must Be LLM-Friendly

Test output should:

* use stable names;
* explicitly state expected values;
* explicitly state actual values;
* include relevant recent events;
* include relevant state;
* avoid unnecessary noise;
* avoid ambiguous natural-language descriptions;
* preserve deterministic ordering.

A good failure should allow an LLM to answer:

> What happened, what was expected, and what should I inspect next?

without asking a human.

---

# 47. Example Good Failure

```text
SCENARIO: town_event_01
STATUS: FAIL

ASSERTION:
  game.state == BATTLE

EXPECTED:
  BATTLE

ACTUAL:
  OVERWORLD

PLAYER:
  map: town
  position: (16,8)

ENTITIES:
  town_guard_01:
    type: enemy
    position: (16,8)

STORY:
  MET_MAYOR: true
  TOWN_ATTACK_STARTED: false

AUDIO:
  track: TOWN

RECENT EVENTS:
  120 PLAYER_MOVED
  121 COLLISION

MISSING EVENTS:
  ENCOUNTER_STARTED
  GAME_STATE_CHANGED
  MUSIC_CHANGED
```

This is significantly more useful than:

```text
FAIL: town event didn't work
```

---

# 48. Current Harness Milestone

The current harness milestone is complete when an LLM can perform the following workflow without human intervention:

```text
1. Build debug ROM.

2. Launch emulator.

3. Load town_event_01.

4. Inspect initial state.

5. Send movement commands.

6. Advance frames.

7. Inspect player position.

8. Detect collision.

9. Inspect telemetry.

10. Confirm story event.

11. Confirm story flag.

12. Confirm battle state.

13. Confirm enemy initialization.

14. Confirm battle music.

15. Execute battle actions.

16. Confirm HP changes.

17. Complete battle.

18. Confirm return to overworld.

19. Confirm overworld music.

20. Produce PASS/FAIL.
```

No human should need to say:

> "Yes, the event happened on screen."

---

# 49. Future Harness Expansion

The architecture must support future systems including:

```text
Scenarios
Story Flags
Teleportation
Party State
Inventory
Equipment
Cards
Decks
Battle State
Enemy AI
Dialogue
NPCs
Cutscenes
Boss Phases
Audio
Save/Load
RNG
Map State
World State
```

For every new subsystem, expose semantic state and relevant telemetry.

For example, when the card system is added, the harness should eventually be able to report:

```text
BATTLE
  turn: 3
  player_hp: 18/20
  enemy_hp: 12/30

DECK
  draw_count: 14
  discard_count: 6

HAND
  fire_slash
  water_guard
  heal

EVENTS
  CARD_DRAWN
  CARD_SELECTED
  CARD_PLAYED
  DAMAGE_DEALT
  COMBO_RESOLVED
```

This should not require screenshot interpretation.

---

# 50. Definition of Done for Gameplay Work

A gameplay feature is not considered complete merely because it works when manually played.

For significant gameplay systems, completion requires:

* implementation;
* semantic state;
* telemetry;
* deterministic behavior where appropriate;
* at least one scenario;
* assertions;
* harness test passing;
* release ROM compiling.

For example, a town event is not complete merely because a human can walk into town and see it.

It is complete when:

```bash
make test-scenario SCENARIO=town_event_01
```

can prove that the event works.

---

# 51. Final Development Principle

The project should continuously move toward this development loop:

```text
                 ┌───────────────┐
                 │      LLM      │
                 └───────┬───────┘
                         │
                  write / inspect
                         │
                 ┌───────▼───────┐
                 │ Test Harness  │
                 └───────┬───────┘
                         │
                  scenario/input
                         │
                 ┌───────▼───────┐
                 │     ROM       │
                 └───────┬───────┘
                         │
              state/events/telemetry
                         │
                 ┌───────▼───────┐
                 │ Test Harness  │
                 └───────┬───────┘
                         │
                    PASS / FAIL
                         │
                 ┌───────▼───────┐
                 │      LLM      │
                 └───────────────┘
```

The long-term objective is:

> **An AI coding agent should be able to develop and test the RPG by interacting with its semantic development interface rather than by manually playing the game.**

The human-facing Game Boy UI remains important for the final player experience.

The development-facing semantic interface is equally important for building the game.

---

# 52. Verified GBDK, CRT0 & Harness Gotchas

These findings were discovered and verified during development. Treat them as
operating constraints. They are the most common sources of "it worked before,
now it hangs / renders wrong" regressions.

## 52.1 No function pointers in harness-exercised gameplay code

GBDK compiles any indirect call (function pointer) through `___sdcc_call_hl`
/ `___sdcc_banked_call`. Those helpers are **RAM-resident code copied to RAM
by CRT0 at boot**. The debug harness jumps straight to `main()` and skips
CRT0, so the helpers are not in RAM and any function-pointer call hangs the
ROM under the harness.

Do not use function pointers for gameplay dispatch (e.g. per-scene loader
functions). Use direct `switch` statements instead. Function pointers are
only safe in code that always runs through the full CRT0 boot.

## 52.2 The Makefile has no header dependencies

Changing a `.h` does not rebuild dependent `.c` files. Stale object files
compiled against an older header produce silent struct-layout mismatches
(e.g. `Entity` field offsets) that manifest as wrong HP values, wrong
positions, or crashes. After editing any shared header, always do a full
clean rebuild:

```bash
make clean && make debug && make release
```

## 52.3 Custom CRT0 `_DATA` layout is ABI-critical

GBDK expects its WRAM variables (`__cpu`, `_cpu`, `__current_bank`, `.mode`,
`.int`, `__shadow_OAM_base`, `.sys_time`) at specific addresses. In this
project `.mode` lives at `0xC0A4` — it was shifted from `0xC0A2` when `_cpu`
was added to the custom CRT0's `_DATA`. Never hardcode these addresses in C
code; expose them as C-visible symbols from `src/crt0.s`:

```asm
.mode:
        .ds     1
        .globl  _console_mode
_console_mode = .mode
```

and declare `extern uint8_t console_mode;` in C.

## 52.4 Custom CRT0 init order

The CRT0 WRAM clear (0xC000-0xDFFF) wipes anything stored before it. Set
`__cpu`, `_cpu`, `__is_GBA`, `__current_bank` **after** the clear. Detect the
CPU type from register A (set by the boot ROM: `0x11` CGB, `0x01` DMG) saved
before the clear, and store it into **both** `__cpu` and `_cpu` so
`_cpu == CGB_TYPE` works (this drives the CGB palette path).

## 52.5 `_HOME` area ordering in crt0.s

Declare areas in the order `_CODE`, `_HOME`, then `_DATA` (matching GBDK's
crt0.o). Declaring `_DATA` first makes sdldgb place gb.lib's `_HOME` code in
WRAM instead of ROM, breaking `joypad`, fonts, and rendering.

## 52.6 The mGBA CLI debugger does not advance VBlank/LY

While paused at a breakpoint, the mGBA debugger does not advance VBlank, so
any code that waits on VBlank (`vsync()`, `display_off()`, GBDK
`set_bkg_data()`) hangs under the harness. Keep VBlank waits out of
harness-reachable boot paths: the harness sets `g_harness_mode` to skip
vsync, and `ui_init()` turns the LCD off before `font_load()` so
`display_off()` returns immediately.

## 52.7 SDCC enums are 1 byte when all values fit

In this toolchain an `enum` whose values fit in a byte is 1 byte wide. Struct
layouts using small enums are compact. Mismatches come from stale objects
(52.2), not from enum sizing.

## 52.8 Host-side telemetry read contract

`g_telemetry_count` is a `uint8_t`; the byte after it is `g_telemetry_head`,
NOT a count high byte. Read it as a single byte, cap at
`TELEMETRY_CAPACITY`, and loop over `min(count, capacity)`. A 16-bit misread
caused ~3084 redundant reads and ~40s per scenario.

## 52.9 Emulator process teardown

`disconnect()` must kill the whole process group (SIGTERM then SIGKILL).
Killing only the `xvfb-run` wrapper orphans Xvfb/mgba processes, which
accumulate, exhaust display numbers, and slow or hang later connects. SIGKILL
also leaves stale `/tmp/.X11-unix/X*` sockets; clean them up if connects
start failing.

## 52.10 Debug input is edge-triggered

`g_inp_mask` is consumed once per `input_update()`. `input_pressed()` fires
only on a 0→1 edge. Consecutive presses of the same button therefore need a
reset frame (`WAIT 1`) between them; otherwise only the first press registers.
Scenario JSONs that walk multiple tiles must interleave `wait` actions.

## 52.11 GBDK RAM-resident library sections

Sections such as `.jpad`, `.hiramcpy`, and the banked-call helpers are
RAM-resident and copied by CRT0 at boot. The harness never runs that copy.
Keep `_HOME` in ROM (52.5) and avoid harness-exercised paths that depend on
RAM-resident library code.

The one RAM-resident section the custom CRT0 itself copies is the VBlank
ISR (see §35): `crt0.s` `init` copies `vbl_isr` from ROM to WRAM `0xC900`
and enables VBlank IE.  The harness skips this (it never enables interrupts),
so the ISR never runs under the harness.

## 52.12 Scenario state ordering

`scenario_begin()` (called by the declarative loader) resets
`g_game.state.flags.bytes[]`. The general loader re-applies descriptor flags
AFTER `scenario_begin()`. Any scenario code that pokes state directly must do
the same ordering, or its flags are silently wiped and story-dependent
scenarios fail in confusing ways.

## 52.13 Full clean build after structural changes

Whenever the `World`/`Game` struct layout, actor tables, or the CRT0 `_DATA`
area changes, the snapshot bytes and symbol addresses shift. The harness
resolves symbols via `get_symbol()`, but the ROM must be fully rebuilt; a
partial build leaves a stale `.sym`/`.gb` pair that reads garbage.

---

# 53. State Ownership

`GameState` (`src/rpg/state.h`) is the **canonical persistent game state**:
scene, party, inventory, flags, variables, and persistent world actor state.
Treat it as the single source of truth for anything a save file would
contain.

## 53.1 Canonical state vs runtime engine copies

* `g_game.state` is authoritative.
* `g_game.world` holds the runtime engine copy of the current scene: terrain,
  exits, the player entity, and spawned hostile actors.  `scene_sync_from_world()`
  copies the scene + player position back into `GameState` once per frame.
* `Battle`, `DialogueState`, `RenderCache`, and input are temporary runtime
  state, never persistent.

## 53.2 Single-writer rules

* Scene/position: `scene_load()`, `scene_update_from_map()`, and
  `scene_sync_from_world()` write `state.scene`.  Do not set
  `state.scene.*` from gameplay code directly.
* Party HP: `battle_start` reads the hero HP from `state.party.members[0]`;
  victory writes the post-battle HP back to both the party and the world copy.
* Flags/variables: use `game_flag_set/clear`, `game_variable_set/add`.
  Story flags are a sub-set of `GameState.flags`; `story.c` operates on
  `GameState*`.
* Persistent actor defeat: `world_on_battle_end()` records
  `ACTOR_STATE_DEFEATED` into `state.world` keyed by the stable ActorId.
  `actor_load_scene()` skips spawning defeated actors.
* Currency: `currency_add/currency_set` in `src/rpg/currency.{h,c}` mutate
  `state.currency` (dense slots indexed by `CurrencyId`).  Gold is
  `CURRENCY_ID_GOLD`, not a generic variable.
* Items: ownership via `inventory_add/remove`; effects via `item_use` /
  `item_purchase` in `src/rpg/items.{h,c}`.  `item_use` consumes only on a
  successful use; `item_purchase` is atomic (failed purchases leave state
  unchanged).
* Progression: `progression_add` / `progression_ensure` in
  `src/rpg/progression.{h,c}` mutate `state.progression`.  The engine is
  generic and contains no per-target-type gameplay logic; the game-specific
  consequence of a level-up lives in `game_on_level_up()` (called by the
  progress-granting caller when a level was crossed).
* Static actor definitions (`WorldActorDefinition`) must never hold mutable
  state; lifecycle lives in `state.world` + `World.actors`.

## 53.3 Debug injection must not emit gameplay telemetry

Scenario/`initial_state` setup writes state directly into `GameState`
(flags via `state.flags.bytes[]`, variables via `state.variables.values[]`,
currency via `state.currency.amount[]`, progression via `progression_ensure`,
etc.) and must NOT go through `game_flag_set` / `game_variable_set` /
`currency_add` / `progression_add`, which emit telemetry.  Setup that emits
gameplay telemetry breaks scenarios that assert `event_not_occurred`
(e.g. `town_reentry`).

Runtime **debug actions** (the `g_debug_action` channel) are different: they
are mid-scenario gameplay exercised through the real mechanic functions
(`add_item`, `buy_item`, `add_currency`, `add_progress`, `use_item_direct`),
so they legitimately emit telemetry.  Do not route scenario *setup* through
them.

## 53.4 Descriptor layout is a wire contract

The host serializes `initial_state` JSON into the fixed-size descriptor
`g_scen_state_buf` (`STATE_LOAD_DESC_*` in `src/debug/telemetry.h`), and the
ROM applies it in `scenario_load_state()`.  The extended snapshot
`g_state_snap_buf` (`STATE_SNAP_*`) mirrors the same sections for
host-side assertion.  Keep the host (`tools/emulator.py`) and ROM constants
in sync; changing one without the other silently breaks every scenario.

## 53.5 Snapshot / telemetry observability contract

* Core snapshot (`g_snap_buf`, 36 bytes): byte 12 is `state.flags.bytes[0]`,
  byte 19 is `state.scene.scene_id`.  Existing scenarios depend on these.
* Extended snapshot (`g_state_snap_buf`, 183 bytes, version 0x03): version
  byte 0, flags 1..8, variables 9..24, currency 25..37, party 38..50,
  inventory 51..83, world 84..132, progression 133..181, equipment 182.
* Every important gameplay transition must emit a telemetry event; state
  assertions must be possible without screenshots.

## 53.6 Save/load boundary (design, not implementation)

`GameState` is the potential save unit.  Rule:

> **If a piece of state is part of `GameState`, it is potentially saveable;
> if it is temporary runtime state, it is not.**

Persistent state: scene, party (id/hp/max), inventory, flags, variables,
currency, world actor lifecycle, progression.  Runtime state — `Battle`,
`DialogueState`, `RenderCache`, input state, `World.actors` HP/facing (the
engine copy), `g_game.screen` — must never become part of the save format.
`World.actors` is rebuilt from scene definitions + `GameState.world` on every
scene load.

The wire descriptor `g_scen_state_buf` and the extended snapshot
`g_state_snap_buf` are the save-boundary probes: the host roundtrip check
(`python3 tools/dev.py roundtrip <scenario>`) loads an `initial_state`,
dumps the canonical state, rebuilds a descriptor from the dump, reloads,
and asserts the state is unchanged.  Any section that serializes in but not
out (or vice versa) breaks the roundtrip.

## 53.7 Semantic layers — never expose byte layouts to the LLM

Keep the layers separate:

```text
GameState
    ↓
semantic state representation (tools/test_runner.format_state)
    ↓
debug snapshot / protocol transport (g_snap_buf / g_state_snap_buf)
    ↓
LLM
```

The LLM-facing output is text such as:

```text
SCENE=FOREST
PLAYER=(10,8) FACING=RIGHT
FLAGS: ARRIVED_TOWN MET_MAYOR
VARIABLES: CHAPTER=1 GOLD=150
PARTY[0]: HERO lvl=3 24/30
INVENTORY: POTION x2
WORLD: SLIME_FOREST=DEFEATED
```

The byte offsets in `g_snap_buf` / `g_state_snap_buf` are an internal
transport contract and may change; scenarios must not depend on them.
Semantic assertions (`flag`, `variable`, `inventory`, `party_hp`,
`party_level`, `actor_state`, `screen_row`, ...) are the stable API.

---

# 54. State Ownership, Screens & Actor Lifecycle (audit)

## 54.1 State ownership

For every piece of persistent state, one clear owner:

| State             | Owner                        | Mutators                          | Persistent |
|-------------------|------------------------------|-----------------------------------|------------|
| Player HP/max     | `state.party.members[0]`     | battle, item_use, level-up        | yes        |
| Current scene/pos | `state.scene`                | scene_load / scene_sync_from_world| yes        |
| Quest state       | `state.variables[QUEST_MONSTER_HUNT]` | event actions             | yes        |
| Quest objective   | `state.variables[MONSTERS_REMAINING]` | MONSTER_DEFEATED event      | yes        |
| Slime defeated    | `state.world` (ActorId)      | world_on_battle_end               | yes        |
| Gold              | `state.currency[GOLD]`       | currency_add                      | yes        |
| Equipped weapon   | `state.equipment.weapon`     | item_equip                        | yes        |
| Hero attack       | derived (`game_hero_attack`) | from equipment                    | derived    |
| Screen            | `g_game.screen`              | screen_change                     | no         |

The quest/objective lives in **generic variables** owned by the event table
(`src/core/event.c`) — never in the Mayor actor or the dialogue screens.

## 54.2 Screen transition contract

Every screen implements `update()` + `render()` (+ shared `screen_change`
enter/exit).  Screens hold **no persistent state**: everything that matters
lives in `GameState`.  Transient UI state (`game_over_choice`,
`item_menu_index`, `item_menu_tab`, `shop_message`) lives in `Game` and is
reset on exit.  A screen must never become the home of gameplay state
(quest progress, HP, etc.).

The quick screen (`SCREEN_ITEM`) is a tabbed menu: ITEM (use consumables),
EQUIP (equip weapons), QUEST (ongoing quests), STATUS (hero HP/gold/level).
START is the universal open key (overworld and battle player-turn); SELECT
in the overworld does nothing.  Inside, SELECT focuses the tab row,
LEFT/RIGHT moves tabs, A confirms, B closes.

## 54.3 Actor lifecycle

* `WorldActorDefinition` (static): scene-owned configuration, never mutable.
* `World.actors` (runtime): the engine's current-scene copy, rebuilt on every
  scene load; spawned hostiles live here.
* `GameState.world` (persistent): defeats/lifecycle keyed by stable `ActorId`.

`world_spawn_actor()` creates a runtime-only hostile (actor_id 0) that is NOT
persistent — used for training enemies and test fixtures.

## 54.4 Warnings / lint

The normal build cannot enable `-Wall` (sdcc's `--use-stdout` pipeline leaks
warnings into the assembly stream).  Use `make lint` to run a
compile-to-assembly `-Wf-Wall` pass over every source; it must report no
warnings.  This has caught real bugs (e.g. `uint8_t` progression thresholds
truncating values > 255).

## 54.5 Input bit layout is a wire contract

`InputButton` bits (`1 << InputButton`) must equal GBDK `joypad()`'s `J_*`
bits.  They diverge only for the real game, never for the harness (the
harness injects `g_inp_mask`, which uses the same `1 << InputButton` bits) —
so a mismatch silently breaks only hardware controls.  Two guards keep the
paths in sync:

* `src/input/input.c` contains a compile-time check (`g_input_bit_layout_ok`)
  that makes any `InputButton`/`J_*` mismatch a hard compile error.
* The harness reads `g_input_button_bits[]` from the ROM at connect and
  derives its injection masks from it (no hand-synced masks).

Never reorder `InputButton` without updating the `J_*` expectations, and
never hand-code button masks in `tools/emulator.py`.

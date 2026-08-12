# Game Boy RPG Prototype & Foundation (`gb_card_rpg_proto`)

A deterministic, Nix-based Game Boy RPG development project targeting authentic Nintendo Game Boy (DMG) and Game Boy Color (CGB) constraints.

The project began as a prototype for a **Baten Kaitos-inspired, card-based RPG**, but is deliberately evolving into something broader: a small, reusable **Game Boy RPG foundation** that can eventually serve as the starting point for future RPG projects.

This is **not intended to become a general-purpose game engine**. The architecture is developed pragmatically: when the game exposes a reusable RPG problem, we build a clean foundation for it.

> **Build the game first. Generalize only where the game demonstrates a reusable problem.**

## Current State

The project has a complete, playable vertical slice — a small but complete
RPG:

```text
Town → Mayor → Monster Hunt quest → kill 3 monsters
   → return → SWORD → equip → Castle → Lord of Slimes → Ending
```

plus a second, structurally different quest (the Lost Merchant:
fetch/deliver/unlock his shop) that proves the quest/event abstraction is
reusable.  The full state/flow is deterministic and driven by the debug
harness.

Current status (Foundation 1.0, non-graphical):

- GameState + persistent world actor lifecycle + progression;
- overworld scenes, actors, movement, collision, interaction;
- scripted events, dialogue, quests (registered engine modules);
- items, inventory, equipment, per-shop stock;
- battle flow + results back into GameState;
- battery-backed SRAM save/load (versioned format);
- game/content separation (`src/game/` registered against a generic engine);
- deterministic debug harness (scenarios, telemetry, assertions, RNG);
- memory budget (`make memmap`), lint, release validation.

The **card system is not yet implemented**.  The current battle
implementation proves the battle lifecycle and keeps the combat boundary
clean so the card mechanics can plug in later.

Graphics are still the ASCII console-font prototype; a graphics pipeline is a
specified but deferred milestone (see `docs/graphics.md`).

## Architectural Direction

The repository is being designed around the common denominator of small RPGs rather than around the rules of one specific game.

The emerging foundation includes:

### World

- scenes/maps;
- stable scene IDs;
- actors/entities;
- movement;
- collision;
- scene transitions;
- actor interaction.

### Story

- dialogue;
- scripted events/scenes;
- event triggers;
- game flags;
- game variables.

### RPG State

- party state;
- character stats;
- progression;
- items;
- inventory;
- equipment;
- persistent world state.

### Gameplay

- encounter lifecycle;
- combatants;
- battle entry/exit;
- rewards;
- game-specific combat rules.

### Persistence

Save/load is intended to serialize persistent game state rather than individual screens.

### Development Infrastructure

- deterministic state injection;
- scenario testing;
- telemetry;
- assertions;
- LLM-readable debug state;
- emulator automation;
- reproducible builds.

See [`docs/rpg-foundation.md`](docs/rpg-foundation.md) for the architecture and [`docs/DEBUG_PROTOCOL.md`](docs/DEBUG_PROTOCOL.md) for the debug contract.

## Development Philosophy

The repository deliberately separates **foundation mechanisms** from **game-specific content**.

For example:

```text
Foundation                  Game
────────────────────────────────────────
Actor                       Mayor
Actor                       Guard
Actor                       Slime

Scene                       Town
Scene                       Forest

Dialogue                    Mayor greeting
Dialogue                    Guard warning

Item                        Potion
Item                        Phoenix Sword

Battle lifecycle            Baten Kaitos-style rules
Progression                 Game-specific XP formula
```

The foundation should provide reusable primitives without hard-coding the story, characters, items, maps, or combat rules of this particular RPG.

Generalization follows demonstrated need rather than speculation.

## Hardware & Toolchain

- **Target Hardware**: Nintendo Game Boy (DMG) / Game Boy Color (CGB)
- **C Toolchain**: GBDK-4 (`lcc`)
- **Assembly Toolchain**: RGBDS (`rgbasm`, `rgblink`, `rgbfix`)
- **Environment**: Nix flakes
- **Primary development/debug emulator**: mGBA
- **Compatibility testing**: Gambatte and other available Game Boy emulators

The project is developed against **mGBA** for debugging and automated development workflows. Compatibility is also checked against other emulators because emulator behavior can differ, particularly around low-level Game Boy behavior and error reporting.

The Nix environment also provides tools such as SameBoy where available.

## Requirements

- Nix with flakes enabled.

No host-level package installation should be necessary.

## Development Setup

Enter the reproducible development environment:

```bash
nix develop
```

## Commands

Common development tasks are exposed through `make` targets.

### Build Release ROM

```bash
make release
```

Produces `build/rpg_card_proto.gb`.

### Build Debug ROM

```bash
make debug
```

Produces `build/rpg_card_proto_debug.gb` with the development harness, telemetry, assertions, deterministic scenario support, and debug metadata enabled.

### Run the ROM

```bash
make run
```

The Makefile detects an available emulator from the development environment.

### Run the Debug ROM

```bash
make run-debug
```

### Run the Full Harness

```bash
make test-harness
```

Builds the debug ROM and executes the scenario tests through the host-side development harness.

### Run One Scenario

```bash
make test-scenario SCENARIO=first_encounter
```

The scenario name corresponds to a scenario definition under `tools/scenarios/`.

### Validate the Release ROM

```bash
make test
```

Builds the release ROM and validates its Game Boy header/checksum integrity.

### Memory Budget

```bash
make memmap
```

Prints the reproducible ROM/WRAM budget and fails on a `_HOME` >= 0x8000
violation.

### Save/Load Roundtrip (host descriptor check)

```bash
make roundtrip SCENARIO=save_load_roundtrip
```

### Capture a Screenshot

```bash
make screenshot
```

Produces `build/screenshot.png` for visual inspection.

### Clean Build Artifacts

```bash
make clean
```

## Development Harness

A major architectural goal is that an LLM should be able to test the game without manually playing through the entire RPG to reach a scenario.

Instead, scenarios can establish a deterministic initial state such as:

```text
Scene: TOWN
Player: (8, 6)
Flag: MET_MAYOR = false
```

and then execute actions such as:

```text
UP
A
```

The harness can inspect telemetry, assertions, state snapshots, and scenario results to determine what happened.

This allows agents to test situations such as:

- talking to a specific NPC;
- entering combat with a specific enemy;
- triggering a scripted scene;
- testing a scene transition;
- starting from a particular progression state;
- testing game-over behavior;
- reproducing a bug without replaying the entire game.

The debug protocol is documented in [`docs/DEBUG_PROTOCOL.md`](docs/DEBUG_PROTOCOL.md), with the broader harness design in [`docs/dev-harness.md`](docs/dev-harness.md).

## Project Structure

```text
src/
├── core/       Boot glue, events, dialogue, story, quests
├── rpg/        GameState, items, inventory, currency, party, progression, save
├── world/      Scenes, actors, movement and collision
├── battle/     Battle lifecycle and combatants
├── input/      Joypad handling and debug input
├── audio/      Game Boy audio/music infrastructure
├── ui/         Screen and UI rendering
├── screens/    Individual game screens
├── debug/      Telemetry, scenarios, assertions and deterministic debug support
└── game/       Game-specific content (events, dialogue, actors, items, quests, shops)

build/          Generated ROMs and build artifacts
docs/           Architecture and development documentation
tools/          Host-side development and emulator harness
```

## Documentation

- [`docs/architecture.md`](docs/architecture.md) — module layout, dependency
  direction, state ownership, content registration.
- [`docs/FOUNDATION_CONTRACT.md`](docs/FOUNDATION_CONTRACT.md) — what the
  foundation provides, what it does not, and the golden rule.
- [`docs/game-vs-engine.md`](docs/game-vs-engine.md) — how to decide where a
  feature belongs.
- [`docs/save-format.md`](docs/save-format.md) — the SRAM save format and
  versioning policy.
- [`docs/memory-budget.md`](docs/memory-budget.md) — the memory budget.
- [`docs/graphics.md`](docs/graphics.md) — the deferred graphics pipeline spec.
- [`docs/testing.md`](docs/testing.md) — how the foundation is validated.
- [`docs/roadmap.md`](docs/roadmap.md) — status (DONE / NEXT / LATER) and the
  detailed historical plan.
- [`docs/DEBUG_PROTOCOL.md`](docs/DEBUG_PROTOCOL.md) — debug protocol and
  LLM-readable game state contract.
- [`docs/dev-harness.md`](docs/dev-harness.md) — deterministic scenario and
  development harness design.
- [`AGENTS.md`](AGENTS.md) — operational instructions for AI coding agents
  working on the repository.

## Long-Term Direction

The eventual goal is not to turn this repository into a large standalone engine.

Instead, once the common RPG infrastructure becomes sufficiently stable, this repository may become a **Game Boy RPG template/foundation** from which future games can be created:

```text
             Game Boy RPG Foundation
                       │
          ┌────────────┼────────────┐
          ▼            ▼            ▼
        RPG #1        RPG #2       RPG #3
```

A future game should be able to replace:

- maps;
- characters;
- enemies;
- items;
- dialogue;
- story;
- quests;
- progression rules;
- combat rules;

while retaining reusable infrastructure such as screens, scenes, actors, input, audio, persistence, debugging, telemetry, and the scenario harness.

The repository has reached **Foundation 1.0 (non-graphical)**: the generic
runtime is cleanly separated from game content, the vertical slice is complete
and deterministically testable, save/load works, and the memory budget is
understood.  The next milestones are the graphics pipeline (spec'd in
`docs/graphics.md`), then the card battle system — at which point this
repository is the template a real game is forked from.

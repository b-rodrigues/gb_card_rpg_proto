# Game Boy RPG Foundation & LLM-First Development Template

A deterministic, Nix-based Game Boy RPG development template targeting authentic Nintendo Game Boy (DMG) and Game Boy Color (CGB) hardware constraints.

This repository serves as a **reusable template and foundation for Game Boy RPG development**, designed from the ground up to demonstrate and enable **LLM-first game development feasibility**. AI coding agents can build, execute, inspect, test, and debug gameplay loops through deterministic, machine-readable interfaces without requiring a human to manually play through the game or interpret the screen.

---

## What This Repository Is

1. **An LLM-First Retro Development Template**: A full Game Boy RPG codebase engineered for automated agent development. Every subsystem provides semantic observability, deterministic state injection, frame-level control, and automated assertion testing.
2. **A Reusable Game Boy RPG Foundation**: A generic, modular RPG engine cleanly separated from game content. When creating a new RPG, developers or AI agents can define new maps, actors, dialogue, items, quests, and combat mechanics in `src/game/` without rewriting engine primitives.
3. **A Hardware-Accurate, Production-Grade Toolchain**: Zero host dependency drift via Nix flakes, targeting GBDK-4 (`lcc`), RGBDS (`rgbasm`, `rgblink`, `rgbfix`), and verified across SameBoy and mGBA.

---

## LLM-First Development Architecture

Developing Game Boy games with AI agents requires solving retro hardware opacity: LLMs cannot reliably play real-time games with a joypad or interpret low-resolution pixels.

This repository solves that by making **semantic observability a first-class subsystem**:

```text
Host-Side AI Agent / Test Runner (Python)
                  │
                  ▼  PTY / Debug Protocol
     mGBA / SameBoy Emulator Session
                  │
                  ▼  Memory & Registers
     Game Boy Debug ROM (rpg_card_proto_debug.gb)
   ┌──────────────────────────────────────────────┐
   │ • Declarative State Injection (Scene/Player) │
   │ • Bounded Telemetry Ring Buffer (Events)     │
   │ • Semantic State Snapshots (Game/World/Party)│
   │ • Hardware Timer Sound Clock (TIMA ISR)      │
   │ • Deterministic RNG Seeding                  │
   └──────────────────────────────────────────────┘
```

### Key LLM-First Capabilities

- **Declarative Scenario Fixtures**: Over 100 deterministic scenarios in `tools/scenarios/*.json` that configure map coordinates, story flags, party stats, inventory, and enemy states before running scripted inputs.
- **Parallel Test Harness**: Runs test scenarios concurrently across host CPU cores (`make test-harness JOBS=16`), achieving over 7x speedup compared to serial execution.
- **Structured Telemetry**: Every state transition, map crossing, collision, dialogue event, quest update, and battle action emits structured telemetry into a bounded ring buffer.
- **Semantic State Snapshots**: Agents inspect exact world coordinates, active screens, dialogue trees, story flags, and combat stats rather than parsing pixel frames.
- **Deterministic Randomness**: RNG is fully seeded and reproducible across test runs.

See [`docs/DEBUG_PROTOCOL.md`](docs/DEBUG_PROTOCOL.md) and [`AGENTS.md`](AGENTS.md) for full protocol and operational contracts.

---

## Engine Features & Vertical Slice

The repository includes a complete, playable vertical slice proving all core RPG systems:

- **World & Overworld**:
  - Tile-based maps with smooth sub-tile hardware scrolling;
  - Collision detection, walkability allowlists, and warp gates;
  - Persistent actors, NPC patrolling, and proximity interaction.
- **Story & Events**:
  - Dialogue player with multi-step choices and branch conditions;
  - Scripted quest system with multi-stage state tracking;
  - Global story flags and variables.
- **RPG State & Progression**:
  - Party stats, leveling, and XP progression;
  - Inventory management, item usage, and equipment slots;
  - Shop system with per-shop stock and currency handling.
- **Turn-Based Battle Lifecycle**:
  - Transition into combat, turn management, damage calculation, and victory/defeat resolution;
  - Pluggable combat mechanics cleanly separated from the battle screen lifecycle.
- **Persistence**:
  - Battery-backed SRAM save/load with versioned formats and slot management.
- **Audio Architecture**:
  - Hardware Timer-driven music clock (`TIMA` overflow ISR) running at fixed 256 Hz tempo regardless of main loop load or LCD redraws.

---

## Engine vs. Game Content Separation

The codebase strictly enforces the separation of generic engine primitives from game-specific data:

```text
src/
├── core/       Generic boot glue, event engine, dialogue runner, story flags, quests
├── rpg/        Generic GameState, inventory, items, currency, party, progression, save
├── world/      Generic scene loader, actors, movement, collision, interactions
├── battle/     Generic battle lifecycle, turn controller, combatant stats
├── ui/         Generic screen renderer, menu frame layouts, font rendering
├── screens/    Overworld, battle, dialogue, item/status, shop, save/load screens
├── input/      Joypad abstraction and programmatic debug input injection
├── audio/      Timer-driven audio ISR and sound registers
├── debug/      Harness telemetry, scenario loader, assertions, RNG control
│
└── game/       GAME CONTENT (All game-specific tables live here)
    ├── game_ids.h          Named story flags, variables, currencies, and actors
    ├── content.c           New game initialization and victory hooks
    ├── events_content.c    Scripted events and quest triggers
    ├── dialogue_content.c  Dialogue scripts and NPC lines
    ├── actors_content.c    Per-map actor spawn tables and behaviors
    ├── items_content.c     Item catalog and equipment definitions
    ├── scenes_content.c    Scene definitions and map layouts
    ├── shops_content.c     Shop inventories and pricing
    └── tiles_content.c     World tilesets and graphical definitions
```

When creating a new game from this template, developers and agents modify `src/game/` while leaving the engine subsystems in `src/core/`, `src/world/`, `src/rpg/`, and `src/ui/` untouched.

---

## Hardware & Toolchain

- **Target Hardware**: Nintendo Game Boy (DMG) / Game Boy Color (CGB)
- **C Toolchain**: GBDK-4 (`lcc` / SDCC)
- **Assembly Toolchain**: RGBDS (`rgbasm`, `rgblink`, `rgbfix`)
- **Environment**: Nix flakes (100% reproducible, zero host dependencies)
- **Development Emulator**: SameBoy & mGBA

---

## Quick Start

### 1. Enter the Nix Development Shell

```bash
nix develop
```

All build tools, compilers, emulators, and test runners are automatically provided.

### 2. Primary Make Targets

| Target | Description | Output |
| :--- | :--- | :--- |
| `make release` | Build optimized release ROM | `build/rpg_card_proto.gb` |
| `make debug` | Build debug ROM with harness & telemetry | `build/rpg_card_proto_debug.gb` |
| `make test-harness` | Run all 100+ scenarios in parallel | Parallel test results (PASS/FAIL) |
| `make test` | Validate ROM header and checksums | ROM verification |
| `make memmap` | Check ROM and WRAM memory budget | Invariant check (`_HOME < 0x8000`) |
| `make run` | Launch release ROM in emulator | Game window |
| `make run-debug` | Launch debug ROM in emulator | Debug game window |
| `make screenshot` | Capture headless emulator screenshot | `build/screenshot.png` |
| `make clean` | Remove all generated build artifacts | Clean directory |

### 3. Running Scenario Tests

Run all scenarios in parallel (defaults to all host cores, or specify `JOBS`):

```bash
make test-harness JOBS=16
```

Run a single scenario with full diagnostic trace:

```bash
make test-scenario SCENARIO=town_arrival
```

---

## Documentation

- [`AGENTS.md`](AGENTS.md) — Operational contract and rules for AI coding agents.
- [`docs/DEBUG_PROTOCOL.md`](docs/DEBUG_PROTOCOL.md) — Authoritative debug protocol and LLM state inspection contract.
- [`docs/dev-harness.md`](docs/dev-harness.md) — Deterministic scenario design and emulator bridge.
- [`docs/architecture.md`](docs/architecture.md) — Subsystem architecture and dependency direction.
- [`docs/FOUNDATION_CONTRACT.md`](docs/FOUNDATION_CONTRACT.md) — Foundation boundaries and golden rules.
- [`docs/save-format.md`](docs/save-format.md) — SRAM save format and versioning specification.
- [`docs/memory-budget.md`](docs/memory-budget.md) — ROM and WRAM memory budgets.
- [`docs/graphics.md`](docs/graphics.md) — Graphics conversion pipeline (`png2gb.py`) and tile layout.

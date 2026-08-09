# Agent Operating Instructions (AGENTS.md)

This file defines the operational contract and constraints for AI coding agents working on this project.

## Environment

This repository uses Nix flakes for complete, reproducible environment management.

Do not install dependencies manually using `apt`, `brew`, `npm`, `pip`, or other host package managers.

Enter the development environment with:

```bash
nix develop
```

## Primary Commands

All operations are exposed via standard `make` targets:

* **Build Release ROM**:
  ```bash
  make release
  ```
  Produces `build/rpg_card_proto.gb`.

* **Build Debug ROM**:
  ```bash
  make debug
  ```
  Produces `build/rpg_card_proto_debug.gb` with telemetry, assertions, and scenario loading enabled.

* **Run Harness Scenario Tests**:
  ```bash
  make test-harness
  ```
  Builds the debug ROM and executes all JSON scenario tests in `tools/scenarios/` via `python3 tools/dev.py test`.

* **Run Specific Harness Scenario**:
  ```bash
  make test-scenario SCENARIO=first_encounter
  ```
  Runs a specific scenario test by name.

* **Automated ROM Validation**:
  ```bash
  make test
  ```
  Builds the release ROM and validates header/checksum integrity.

* **Run in Emulator**:
  ```bash
  make run
  ```
  Builds the ROM (if needed) and launches it in the emulator.

* **Capture Visual Screenshot**:
  ```bash
  make screenshot
  ```
  Produces `build/screenshot.png` for visual inspection of gameplay.

* **Clean Build Artifacts**:
  ```bash
  make clean
  ```
  Removes generated artifacts in `build/`.

## Target Platform & Toolchain

* **Target Hardware**: Nintendo Game Boy (DMG) / Game Boy Color (CGB)
* **C Toolchain**: GBDK-4 (`lcc`)
* **Assembly Toolchain**: RGBDS (`rgbasm`, `rgblink`, `rgbfix`)

## Project Structure

* `src/main.c`: Entry point and main game loop
* `src/core/`: Game state machine, core update/render loop (`game.c`, `state.c`)
* `src/world/`: Overworld map, entity positions, collision detection (`world.c`, `entity.c`)
* `src/battle/`: Turn-based card RPG battle engine and combatant stats (`battle.c`, `combatant.c`)
* `src/input/`: Joypad button reading, state tracking, and debug injection (`input.c`)
* `src/audio/`: Hardware APU sound engine & VBlank music player (`audio.c`)
* `src/ui/`: ASCII background tile rendering and font management (`ui.c`)
* `src/debug/`: Development harness — telemetry ring buffer, PRNG, scenario loader, assertions (`telemetry.c`, `rng.c`, `scenarios.c`, `assertions.c`)
* `tools/`: Host-side testing harness scripts (`dev.py`, `test_runner.py`, `emulator.py`, `scenarios/*.json`)
* `build/`: Generated artifacts (`build/rpg_card_proto.gb`, `build/rpg_card_proto_debug.gb`, `build/*.o`)

## Code Philosophy

1. **Simple C**: Prefer clear, explicit C code over complex macro magic or indirect callbacks.
2. **Small Functions**: Keep functions short and focused on a single responsibility.
3. **Explicit State**: Represent state using plain C structs and enums.
4. **Game Boy-Native**: Operate directly on 8-bit integers, tiles, and VRAM concepts.
5. **No External Dependencies**: Do not introduce modern engines, scripting runtimes, or non-Nix packages.

## Game Boy Engineering & Harness Insights & Rules

1. **Development Harness First**:
   - Every state transition, movement, collision, encounter, battle turn, damage roll, or audio track change MUST emit a telemetry event via `telemetry_emit(...)`.
   - Never rely exclusively on screen visual inspection to diagnose bugs — inspect telemetry events, game snapshots, and scenario test results first.

2. **Hardware VBlank Sound Timing (`add_VBL`)**:
   - Never update music step timers directly inside the main `while(1)` loop. Main loop CPU variations cause music to play at variable tempos between menus and gameplay.
   - Always hook the sound update function to the hardware VBlank interrupt vector (`add_VBL(audio_update)`). Always call `enable_interrupts()` after initializing VBL interrupt handlers.

3. **Targeted Redrawing vs. Full Screen Clears**:
   - Avoid calling full-screen clears (`ui_clear_screen()`) during frequent interactive events like menu navigation or UI cursor updates. Full clears cause visual screen flicker.
   - Perform full clears only on major screen transitions (e.g. Overworld -> Battle). Update specific tile coordinates for incremental UI updates.

4. **Joypad Startup State Initialization (`input_init`)**:
   - Initialize both `pad_state` and `prev_pad_state` to the hardware `joypad()` value in `input_init()` before entering the main loop.
   - Leaving `prev_pad_state = 0` causes `input_pressed()` to return `true` on boot, immediately skipping title screens.

5. **Game Boy Color (CGB) Palette & Attribute Mapping**:
   - Use `-Wm-yc` compiler flag and `rgbfix -C` header flags for CGB dual compatibility (Header 0x143 = 0x80).
   - Check `_cpu == CGB_TYPE` before initializing palettes (`set_bkg_palette`) or writing tile attributes to VRAM Bank 1 (`VBK_REG = 1`). Always reset `VBK_REG = 0` for tile indices.

6. **SDCC C89 Compiler Scope Rules**:
   - GBDK-4 uses SDCC (C89 dialect). Declare all variables at the beginning of function blocks. Avoid variable declarations inside nested `if`/`for` blocks or using non-constant array initializers.

7. **Automated Screenshot Capture**:
   - Allow at least 4 seconds (`sleep 4`) in automated screenshot capture scripts to let the Game Boy Color boot animation finish before taking screenshots.

## Validation Workflow

After modifying code or gameplay logic, always validate your changes using the harness:

1. Run `make test-harness` to run all scenario assertion tests.
2. Run `make test` to verify release compilation, linking, and ROM header integrity.
3. Run `make screenshot` or `make run` to visually inspect rendering and gameplay state if visual verification is needed.

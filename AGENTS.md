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

* **Build the ROM**:
  ```bash
  make
  ```
  Produces `build/game.gb`.

* **Run in Emulator**:
  ```bash
  make run
  ```
  Builds the ROM (if needed) and launches it in the emulator.

* **Automated Validation**:
  ```bash
  make test
  ```
  Builds the ROM and validates its header and checksum.

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
* `src/game.c`, `src/game.h`: Game loop, state machine, and Tetris game logic
* `src/board.c`, `src/board.h`: 10x16 playfield, occupancy check, and line clearing
* `src/piece.c`, `src/piece.h`: Tetromino shapes, rotation, collision, and locking
* `src/input.c`, `src/input.h`: Joypad button reading and state tracking
* `src/render.c`, `src/render.h`: Game Boy VRAM background tile & CGB palette rendering
* `src/sound.c`, `src/sound.h`: Hardware APU sound engine & VBlank melody player
* `asm/`: Assembly routines (e.g. `asm/example.asm`)
* `build/`: Generated artifacts (`build/game.gb`, `build/*.o`, `build/screenshot.png`)

## Code Philosophy

1. **Simple C**: Prefer clear, explicit C code over complex macro magic or indirect callbacks.
2. **Small Functions**: Keep functions short and focused on a single responsibility.
3. **Explicit State**: Represent state using plain C structs and enums.
4. **Game Boy-Native**: Operate directly on 8-bit integers, tiles, and VRAM concepts.
5. **No External Dependencies**: Do not introduce modern engines, scripting runtimes, or non-Nix packages.

## Game Boy Engineering Insights & Rules

1. **Hardware VBlank Sound Timing (`add_VBL`)**:
   - Never update music step timers directly inside the main `while(1)` loop. Main loop CPU variations cause music to play at variable tempos between menus and gameplay.
   - Always hook the sound update function to the hardware VBlank interrupt vector (`add_VBL(sound_update)`) inside `CRITICAL { ... }` so music plays at a constant hardware frequency (59.73 Hz).

2. **Targeted Redrawing vs. Full Screen Clears**:
   - Avoid calling full-screen clears (`render_clear_screen()`) during frequent interactive events like menu navigation or UI cursor updates. Full clears cause visual screen flicker.
   - Perform full clears only on major screen transitions (e.g. Title -> Game -> Options). Update specific tile coordinates for incremental UI updates.

3. **Joypad Startup State Initialization (`input_init`)**:
   - Initialize both `pad_state` and `prev_pad_state` to the hardware `joypad()` value in `input_init()` before entering the main loop.
   - Leaving `prev_pad_state = 0` causes `input_is_pressed()` to return `true` on boot, immediately skipping title screens.

4. **Game Boy Color (CGB) Palette & Attribute Mapping**:
   - Use `-Wm-yc` compiler flag and `rgbfix -C` header flags for CGB dual compatibility (Header 0x143 = 0x80).
   - Check `_cpu == CGB_TYPE` before initializing palettes (`set_bkg_palette`) or writing tile attributes to VRAM Bank 1 (`VBK_REG = 1`). Always reset `VBK_REG = 0` for tile indices.

5. **SDCC C89 Compiler Scope Rules**:
   - GBDK-4 uses SDCC (C89 dialect). Declare all variables at the beginning of function blocks. Avoid variable declarations inside nested `if`/`for` blocks or using non-constant array initializers.

6. **Automated Screenshot Capture**:
   - Allow at least 4 seconds (`sleep 4`) in automated screenshot capture scripts to let the Game Boy Color boot animation finish before taking screenshots.

## Validation Workflow

After modifying code or gameplay logic, always validate your changes:

1. Run `make test` to verify compilation, linking, and ROM header integrity.
2. Run `make screenshot` or `make run` to visually inspect rendering and gameplay state.

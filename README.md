# Game Boy Card-Based RPG Prototype (`gb_card_rpg_proto`)

A deterministic, Nix-based Game Boy development project focused on prototyping a card-based RPG for the Nintendo Game Boy (DMG) and Game Boy Color (CGB).

## Goal

The primary goal of this repository is to build a card-based role-playing game (RPG) prototype targeting authentic Game Boy hardware constraints. The project combines deck-building / card mechanics with classic Game Boy turn-based RPG exploration and battle systems using GBDK-4.

## Hardware & Toolchain

* **Target Hardware**: Nintendo Game Boy (DMG) / Game Boy Color (CGB)
* **C Toolchain**: GBDK-4 (`lcc`)
* **Assembly Toolchain**: RGBDS (`rgbasm`, `rgblink`, `rgbfix`)
* **Environment**: Nix flakes (reproducible build environment)

## Requirements

* Nix with flakes enabled.

## Development Setup

Enter the Nix development environment to make all toolchain dependencies (`gbdk`, `sameboy`, `rgbs`, etc.) available:

```bash
nix develop
```

## Commands

All common tasks are accessible via standard `make` targets:

* **Build ROM**:
  ```bash
  make
  ```
  Produces `build/rpg_card_proto.gb`.

* **Run in Emulator**:
  ```bash
  make run
  ```
  Builds the ROM and launches it in the emulator.

* **Automated Test & Validation**:
  ```bash
  make test
  ```
  Builds the ROM and verifies header integrity & checksums.

* **Capture Gameplay Screenshot**:
  ```bash
  make screenshot
  ```
  Generates `build/screenshot.png` for visual inspection.

* **Clean Build Artifacts**:
  ```bash
  make clean
  ```
  Removes all compiled files in `build/`.

## Project Structure

* `src/`: C source files and game logic built with GBDK-4.
* `asm/`: Assembly routines.
* `assets/`: Game Boy tilemaps, graphics, and card assets.
* `build/`: Target output binaries (`build/rpg_card_proto.gb`) and build artifacts.
* `tools/`: Helper scripts for asset processing and automated testing.


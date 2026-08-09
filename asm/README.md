# Assembly Integration

This directory contains Game Boy z80 (sm83) assembly source files.

## Overview

Assembly can be integrated into this GBDK C project when performance-critical operations or hardware manipulation are required.

## Tools

- `rgbasm`: RGBDS assembly compiler
- `rgblink`: RGBDS linker
- `rgbfix`: Game Boy ROM header validator/fixer

## Usage with C

To call an assembly function from C:

1. Declare the prototype in C:
   ```c
   extern uint8_t asm_add_numbers(uint8_t a, uint8_t b);
   ```

2. Assemble with `rgbasm` or `lcc`:
   ```bash
   rgbasm -o build/example.o asm/example.asm
   ```

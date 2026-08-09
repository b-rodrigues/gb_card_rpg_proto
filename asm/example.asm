; example.asm - Minimal Game Boy assembly example using RGBDS syntax
;
; This file demonstrates how to write Game Boy assembly routines
; that can be compiled with RGBDS (rgbasm) or integrated with C code.
;
; Function signature in C:
;   extern uint8_t asm_add_numbers(uint8_t a, uint8_t b);

SECTION "ExampleCode", ROM0

_asm_add_numbers::
    ; In standard calling convention:
    ; First argument is passed in register A or C
    ; Second argument is passed in register E or B
    ld a, c
    add a, e
    ret

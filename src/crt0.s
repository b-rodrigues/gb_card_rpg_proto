; Minimal CRT0: reuses GBDK vectors/dispatch/logo, minimal init.
        .module crt0
        .globl  _main
        .globl  _audio_update
        .globl  __cpu
        .globl  __is_GBA
        .globl  _cpu
        .globl  __current_bank
        .globl  .mode
        .globl  .int
        .globl  __shadow_OAM_base
        .globl  .sys_time

        .area   _HEADER (ABS)
        .org    0x0000
        .db     0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC3, 0xE5, 0x76, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
        .db     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
        .db     0xE9, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x22, 0x0D, 0x20, 0xFC, 0xC9, 0xFF, 0xFF, 0xFF
        .db     0x1A, 0x22, 0x13, 0x0D, 0x20, 0xFA, 0xC9, 0xFF, 0xF3, 0xC3, 0xCD, 0x77, 0xFF, 0xFF, 0xFF, 0xFF
        .db     0xC3, 0x00, 0xC9, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC3, 0x6E, 0x77, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
        .db     0xFB, 0xF5, 0xE5, 0xC3, 0xB8, 0x77, 0x80, 0x00, 0xF5, 0xE5, 0x21, 0x7F, 0xC3, 0xC3, 0x80, 0x00
        .db     0xF5, 0xE5, 0x21, 0xAE, 0xC3, 0xC3, 0x80, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
        .db     0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80
        .db     0xC5, 0xD5, 0x2A, 0xB6, 0x28, 0x0B, 0xE5, 0x3A, 0x6E, 0x67, 0xE7, 0xE1, 0x23, 0x18, 0xF3, 0xE8
        .db     0x04, 0xD1, 0xC1, 0xE1, 0xF0, 0x41, 0xE6, 0x02, 0x20, 0xFA, 0xF1, 0xD9, 0x21, 0xA3, 0xC0, 0x34
        .db     0x20, 0x02, 0x23, 0x34, 0x3E, 0x01, 0xE0, 0x91, 0xC3, 0x80, 0xFF, 0xF0, 0x41, 0xE6, 0x02, 0x20
        .db     0xFA, 0x3E, 0xC0, 0xC3, 0x84, 0xFF, 0xAF, 0x01, 0x70, 0x03, 0x21, 0xA0, 0xC0, 0xCD, 0x57, 0x4C
        .db     0x3E, 0xC0, 0xE0, 0x92, 0x67, 0xAF, 0x6F, 0x0E, 0xA0, 0xC3, 0x28, 0x00, 0xF3, 0xE0, 0xFF, 0xAF
        .db     0xFB, 0xE0, 0x0F, 0xC9, 0xF0, 0x92, 0xB7, 0xC8, 0xE0, 0x46, 0x3E, 0x28, 0x3D, 0x20, 0xFD, 0xC9
        .db     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
        .db     0xC9, 0xFF, 0xFF, 0xFF, 0xC3, 0x6E, 0x4E, 0xFF, 0xC3, 0x88, 0x76, 0xFF, 0xC3, 0x54, 0x6E, 0xFF
        .db     0x18, 0x55, 0xFF, 0xFF, 0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83
        .db     0x00, 0x0C, 0x00, 0x0D, 0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E, 0xE6
        .db     0xDD, 0xDD, 0xD9, 0x99, 0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F
        .db     0xBB, 0xB9, 0x33, 0x3E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        .db     0x00, 0x00, 0x00, 0x80, 0x30, 0x30, 0x00, 0x19, 0x01, 0x00, 0x00, 0x33, 0x01, 0xB9, 0xFC, 0x8B
        .db     0xFA, 0xA1, 0xC0, 0x47, 0xFA, 0xA0, 0xC0

        .org    0x0157
init:
        di
        ld      d, a
        ld      sp, #0xE000
        ld      hl, #0xC000
        ld      bc, #0x2000
clear_loop:
        xor     a
        ld      (hl), a
        inc     hl
        dec     bc
        ld      a, b
        or      c
        jr      nz, clear_loop
        ld      a, d
        ld      (__cpu), a
        ld      (_cpu), a
        xor     a
        ld      (__is_GBA), a
        inc     a
        ld      (__current_bank), a

        ; Shadow OAM lives at the base of WRAM; refresh_OAM() DMAs it to real
        ; OAM (0xFE00).  The WRAM clear above zeroed __shadow_OAM_base, so set
        ; it explicitly to 0xC000 (high byte = DMA source).
        xor     a
        ld      (__shadow_OAM_base), a       ; low byte 0x00
        ld      a, #0xC0
        ld      (__shadow_OAM_base + 1), a   ; high byte 0xC0

        ; Copy the RAM-resident VBlank ISR into WRAM (always mapped,
        ; independent of ROM banking).  IE is enabled here; IME is enabled
        ; later by main() -> enable_interrupts().
        ld      hl, #vbl_isr
        ld      de, #0xC900
        ld      bc, #(vbl_isr_end - vbl_isr)
copy_isr:
        ld      a, (hl)
        inc     hl
        ld      (de), a
        inc     de
        dec     bc
        ld      a, b
        or      c
        jr      nz, copy_isr
        ldh     a, (0xFFFF)
        or      #0x01
        ldh     (0xFFFF), a

        jp      _main

        .area   _CODE
        .globl  .display_off
.display_off:
        ldh     a, (0xFF40)
        and     #0x80
        ret     z
        ldh     a, (0xFF40)
        and     #0x7F
        ldh     (0xFF40), a
        ret

        .globl  .wait_vbl_done
        .globl  .vsync
.wait_vbl_done:
.vsync:
        ldh     a, (0xFF44)
        cp      #0x91
        jr      nz, vsync_wait
        ret
vsync_wait:
        ldh     a, (0xFF44)
        cp      #0x91
        jr      nz, vsync_wait
        ret

        .globl  .call_hl
.call_hl:
        jp      (hl)

        ; refresh_OAM() copies shadow OAM (WRAM base 0xC000) to real OAM
        ; (0xFE00) via OAM DMA.  GBDK's stock crt0 (which defines this) is
        ; not linked (-no-crt), so we provide the C API here.  Must stay in
        ; bank 0 so the RAM-resident ISR's baked-in call target is mapped.
        .globl  _refresh_OAM
_refresh_OAM:
        ld      a, #0xC0                    ; shadow OAM lives at 0xC000
        ldh     (0xFF46), a                  ; initiate OAM DMA
        ld      a, #0x28                     ; ~40 * 4 cycles
refresh_oam_wait:
        dec     a
        jr      nz, refresh_oam_wait
        ret

        .globl  .reset
        .globl  .remove_int
        .globl  .add_int
        .globl  .add_VBL
        .globl  .remove_VBL
        .globl  _add_VBL
        .globl  _vsync
        .globl  _wait_vbl_done
        .globl  _display_off
.reset:
        jp      init
.remove_int:
.remove_int_stub:
        ret
.add_int:
.add_VBL:
.remove_VBL:
_add_VBL:
        ret

; RAM-resident VBlank ISR: copied to WRAM 0xC900 by init and entered via
; the VBlank vector (0x0040 -> JP 0xC900).  Calls audio_update() directly;
; audio.c must remain in the fixed bank 0 so the baked-in call target is
; always mapped.  Saves af/bc/de/hl (SDCC clobber set).
vbl_isr:
        push    af
        push    bc
        push    de
        push    hl
        call    _audio_update
        pop     hl
        pop     de
        pop     bc
        pop     af
        reti
vbl_isr_end:

_display_off:
        jp      .display_off
_wait_vbl_done:
_vsync:
        jp      .vsync

; ── Banked-content copy trampoline ─────────────────────────────────
; Copies `n` bytes from a banked ROM source to a WRAM destination while
; running entirely from WRAM, so the MBC5 bank switch (rROMB0 at 0x2000)
; is safe regardless of which ROM bank the fixed-bank caller currently
; maps.  A bank switch executed from switchable ROM (0x4000+) unmaps the
; very instruction stream doing the switch; WRAM is always mapped.  The
; body is copied from ROM to the linker-allocated _g_banked_tramp buffer
; (WRAM) by _banked_copy_init (see §52.11/§35: like the VBlank ISR, it must
; be RAM-resident because the harness skips CRT0; game_init calls the init
; so it runs in both real boot and harness boot paths).
;
; No-arg entry: the C wrapper banked_copy() (src/core/banked.c) stores the
; four arguments into _DATA globals (g_bank_copy_bank/dst/src/n) before
; calling, so this trampoline never parses SDCC's stack layout.  The bank
; register is always restored to the project's home bank 1 before returning.
;
; Home bank is 1, NOT 0: the project links with -yo8, so the fixed-bank
; _CODE/_HOME area spans file 0x0000-0x7FFF and the second half (file
; 0x4000-0x7FFF, containing the SDCC runtime helpers and library code) is
; reached at CPU 0x4000-0x7FFF with ROMB=1 (crt0.s init stores
; __current_bank = 1).  Bank 0 (file 0x0000-0x3FFF) only covers the fixed
; region; with ROMB=0, CPU 0x4000+ reads bank 0's first half (file
; 0x0000-0x3FFF) and every fixed-bank call above 0x4000 executes garbage.
        .globl  _g_bank_copy_bank
        .globl  _g_bank_copy_dst
        .globl  _g_bank_copy_src
        .globl  _g_bank_copy_n
_banked_copy_tramp:
        xor     a
        ld      (0x3000), a          ; MBC5 ROM bank high byte = 0
        ld      a, (_g_bank_copy_bank)
        ld      (0x2000), a          ; select ROM bank
        ld      hl, #_g_bank_copy_src
        ld      a, (hl)
        inc     hl
        ld      h, (hl)
        ld      l, a                ; hl = g_bank_copy_src
        ld      d, h
        ld      e, l                ; de = src
        ld      hl, #_g_bank_copy_dst
        ld      a, (hl)
        inc     hl
        ld      h, (hl)
        ld      l, a                ; hl = dst
        ld      a, (_g_bank_copy_n)
        ld      c, a
 banked_copy_loop:
        ld      a, (de)             ; read src byte
        ld      (hl), a             ; write dst byte
        inc     de
        inc     hl
        dec     c
        jr      nz, banked_copy_loop
        ld      a, #0x01
        ld      (0x2000), a          ; restore home bank 1 (see below)
        ld      (__current_bank), a
        ret
 _banked_copy_tramp_end:

; ROM stub for the C wrapper (src/core/banked.c): jumps into the WRAM
; trampoline.  The trampoline's WRAM home is the linker-allocated C buffer
; _g_banked_tramp (src/core/banked.c), so its address is resolved by the
; linker rather than hardcoded.  Runs from the fixed bank and switches
; nothing, so it is safe regardless of the caller's address.
        .globl  _g_banked_tramp
        .globl  _banked_copy_run
_banked_copy_run:
        ld      hl, #_g_banked_tramp
        jp      (hl)

; Copies the trampoline body from ROM into the _g_banked_tramp buffer.
; Called once from game_init() so both real hardware and the harness have it
; resident before any banked content is read.
        .globl  _banked_copy_init
_banked_copy_init:
        ld      hl, #_banked_copy_tramp
        ld      de, #_g_banked_tramp
        ld      bc, #(_banked_copy_tramp_end - _banked_copy_tramp)
banked_copy_init_loop:
        ld      a, (hl)
        inc     hl
        ld      (de), a
        inc     de
        dec     bc
        ld      a, b
        or      c
        jr      nz, banked_copy_init_loop
        ret

        .area   _HOME

        .area   _DATA
__cpu:
        .ds     1
__is_GBA:
        .ds     1
_cpu:
        .ds     1
__current_bank:
        .ds     1
.mode:
        .ds     1
        .globl  _console_mode
_console_mode = .mode
.int:
        .ds     1
__shadow_OAM_base:
        .ds     2
.sys_time:
        .ds     4

        .area   _INITIALIZED
        .area   _INITIALIZER
        .area   _BSS
        .area   _GSINIT
        .area   _GSFINAL
        .area   _HEAP
        .area   _HEAP_END
        .area   _LIT

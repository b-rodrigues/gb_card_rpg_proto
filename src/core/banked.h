#ifndef BANKED_H
#define BANKED_H

#include <stdint.h>

/* RAM-resident MBC5 bank-switch + copy (src/crt0.s).  banked_copy() copies
 * `n` bytes from a banked ROM source into a WRAM destination, running the
 * MBC5 bank switch from a WRAM trampoline so it is safe regardless of the
 * caller's currently mapped ROM bank.  The trampoline must be resident
 * before any banked content is read: game_init() calls banked_copy_init()
 * once at boot (the harness skips CRT0, so the copy cannot live there).
 *
 * The bank register is always restored to the project's home bank 1 (the
 * -yo8 layout maps fixed-bank code above CPU 0x4000 through ROMB=1; see
 * crt0.s) before returning, so banked content must be fully copied out
 * before the call returns.  Never read banked data after the call.  `n` is
 * a uint8_t, so tables (rows or strings) must be smaller than 256 bytes.
 *
 * The trampoline itself lives in the linker-allocated WRAM buffer
 * g_banked_tramp[] (declared here so the engine and crt0.s agree on its
 * symbol): a hardcoded WRAM address (e.g. 0xC940) would collide with
 * linker-placed _INITIALIZED variables such as _g_boot_phase. */
extern uint8_t g_banked_tramp[64];

void banked_copy(uint8_t bank, void *dst, const void *src, uint8_t n);
void banked_copy_init(void);

#endif /* BANKED_H */

#include "banked.h"

/* Argument staging for the RAM-resident copy trampoline (src/crt0.s
 * _banked_copy_tramp, copied to WRAM 0xC940).  The C wrapper stores the
 * SDCC arguments here and then calls banked_copy_run() (asm) so the
 * trampoline never has to parse the compiler's stack layout.  Only the
 * switch + copy executes from WRAM; everything before/after runs in the
 * fixed bank. */

uint8_t g_bank_copy_bank;
void *g_bank_copy_dst;
const void *g_bank_copy_src;
uint8_t g_bank_copy_n;

/* Linker-allocated home for the RAM-resident copy trampoline.  crt0.s's
 * _banked_copy_init copies the ROM trampoline body here and
 * _banked_copy_run jumps to it, so the linker owns this address (a
 * hardcoded WRAM constant would collide with _INITIALIZED variables). */
uint8_t g_banked_tramp[64];

extern void banked_copy_run(void);

void banked_copy(uint8_t bank, void *dst, const void *src, uint8_t n)
{
    g_bank_copy_bank = bank;
    g_bank_copy_dst = dst;
    g_bank_copy_src = src;
    g_bank_copy_n = n;
    banked_copy_run();
}

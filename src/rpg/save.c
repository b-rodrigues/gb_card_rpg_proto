#include "save.h"
#include <gb/gb.h>

/* Battery-backed SRAM save, flat layout with no large stack frame.
 *
 *   SRAM 0xA000: magic (uint16 SAVE_MAGIC)
 *         0xA002: version (uint8 SAVE_VERSION)
 *         0xA003: checksum (uint8, sum over the state bytes)
 *         0xA004: GameState
 *
 * IMPORTANT: the state is copied directly from GameState to SRAM (and back)
 * without staging it in a big local.  The debug harness skips CRT0, so the
 * stack pointer stays at the boot value (0xFFFE) instead of CRT0's 0xE000;
 * a large local (e.g. a ~200 byte SaveSlot) overflows the stack into the
 * I/O region and crashes the ROM. */

#define SAVE_SRAM_MAGIC_OFF     0x0000
#define SAVE_SRAM_VERSION_OFF   0x0002
#define SAVE_SRAM_CHECKSUM_OFF  0x0003
#define SAVE_SRAM_STATE_OFF     0x0004
#define SAVE_SRAM_BASE          0xA000
#define SAVE_SLOT_STRIDE        0x0100

typedef char check_save_slot_size[(sizeof(GameState) <= (SAVE_SLOT_STRIDE - SAVE_SRAM_STATE_OFF)) ? 1 : -1];

static uint16_t save_slot_base(uint8_t slot)
{
    return (uint16_t)(SAVE_SRAM_BASE + (slot * SAVE_SLOT_STRIDE));
}

static uint8_t save_checksum(const GameState *state)
{
    uint8_t sum = 0;
    uint16_t i;
    const uint8_t *b = (const uint8_t *)state;
    for (i = 0; i < sizeof(GameState); i++) {
        sum = (uint8_t)(sum + b[i]);
    }
    return sum;
}

static bool save_valid_at_slot(uint8_t slot)
{
    uint8_t *sram = (uint8_t *)save_slot_base(slot);

    if (sram[SAVE_SRAM_MAGIC_OFF] != (uint8_t)(SAVE_MAGIC & 0xFF) ||
        sram[SAVE_SRAM_MAGIC_OFF + 1] != (uint8_t)(SAVE_MAGIC >> 8)) {
        return false;
    }
    if (sram[SAVE_SRAM_VERSION_OFF] != SAVE_VERSION) {
        return false;
    }
    return sram[SAVE_SRAM_CHECKSUM_OFF] == save_checksum((const GameState *)(sram + SAVE_SRAM_STATE_OFF));
}

bool save_present_slot(uint8_t slot)
{
    bool valid;
    if (slot >= SAVE_SLOT_COUNT) return false;
    ENABLE_RAM;
    valid = save_valid_at_slot(slot);
    DISABLE_RAM;
    return valid;
}

bool save_game_slot(uint8_t slot, const GameState *state)
{
    uint8_t *sram;
    uint8_t checksum;
    uint16_t i;
    const uint8_t *src;

    if (!state || slot >= SAVE_SLOT_COUNT) return false;
    checksum = save_checksum(state);
    sram = (uint8_t *)save_slot_base(slot);

    ENABLE_RAM;
    sram[SAVE_SRAM_MAGIC_OFF] = (uint8_t)(SAVE_MAGIC & 0xFF);
    sram[SAVE_SRAM_MAGIC_OFF + 1] = (uint8_t)(SAVE_MAGIC >> 8);
    sram[SAVE_SRAM_VERSION_OFF] = SAVE_VERSION;
    sram[SAVE_SRAM_CHECKSUM_OFF] = checksum;
    src = (const uint8_t *)state;
    for (i = 0; i < sizeof(GameState); i++) {
        sram[SAVE_SRAM_STATE_OFF + i] = src[i];
    }
    DISABLE_RAM;
    return true;
}

bool load_game_slot(uint8_t slot, GameState *state)
{
    uint8_t *sram;
    uint16_t i;
    uint8_t *dst;
    bool valid;

    if (!state || slot >= SAVE_SLOT_COUNT) return false;
    sram = (uint8_t *)save_slot_base(slot);

    ENABLE_RAM;
    valid = save_valid_at_slot(slot);
    if (valid) {
        dst = (uint8_t *)state;
        for (i = 0; i < sizeof(GameState); i++) {
            dst[i] = sram[SAVE_SRAM_STATE_OFF + i];
        }
    }
    DISABLE_RAM;
    return valid;
}

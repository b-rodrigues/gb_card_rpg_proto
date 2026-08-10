#include <gb/gb.h>
#include "game.h"
#include "audio.h"
#include "input.h"
#include "ui.h"

Game g_game;

/*
 * Set to 1 by the external test harness (via memory write) before
 * jumping to main.  When active, audio, VBlank interrupt handlers,
 * and vsync are skipped so the emulator debugger can use breakpoints
 * for frame synchronisation without blocking on VBlank or DMA.
 *
 * Core subsystem initialisation (input, UI, fonts) always runs.
 */
volatile uint8_t g_harness_mode = 0;

int main(void)
{
    if (!g_harness_mode) {
        audio_init();

        CRITICAL {
            add_VBL(audio_update);
        }

        enable_interrupts();
    }

    input_init();
    ui_init();

    game_init(&g_game);

    while (1) {
        input_update();
        game_update(&g_game);
        game_render(&g_game);
        if (!g_harness_mode) {
            vsync();
        }
    }
}

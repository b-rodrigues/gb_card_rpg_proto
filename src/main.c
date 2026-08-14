#include <gb/gb.h>
#include "game.h"
#include "audio.h"
#include "input.h"
#include "ui.h"
#include "content.h"

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

/*
 * Boot-phase breadcrumb for the test harness.  Incremented at each
 * startup checkpoint so the harness can verify the boot sequence.
 *  0 = ROM loaded, CRT0 not yet entered
 *  1 = main() entered
 *  2 = ui_init() complete
 *  3 = game_init() complete
 *  4 = first game_render() complete
 */
volatile uint8_t g_boot_phase = 0;

int main(void)
{
    g_boot_phase = 1;

    if (!g_harness_mode) {
        audio_init();
        /* The VBlank ISR is set up by CRT0 (vector -> WRAM ISR -> audio_update)
         * and VBlank IE is enabled there; this enables IME. */
        enable_interrupts();
    }

    input_init();
    ui_init();
    g_boot_phase = 2;

    /* Composition root: register THIS game's content with the generic
     * engine before boot glue runs. */
    game_content_init();
    game_init(&g_game);
    g_boot_phase = 3;

    while (1) {
        input_update();
        game_update(&g_game);
        if (g_boot_phase == 3) {
            g_boot_phase = 4;
        }
        if (!g_harness_mode) {
            /* Wait for VBlank AFTER gameplay logic, immediately BEFORE
             * game_render, so every tilemap write inside game_render lands
             * during VBlank.  GBDK's vsync() busy-waits for LY == 145 (the
             * second VBlank scanline) and returns there.  The PPU silently
             * ignores VRAM writes during LCD modes 2/3 (mid-scanout); the
             * previous render-then-vsync order dropped window/HUD tilemap
             * writes (verified: cells read back 0x00 at the next VBlank
             * where the game had written a tile).  Incremental redraws fit
             * easily inside VBlank; only full-screen wipes overrun, and
             * those hide the sprite first and accept a partial tail. */
            vsync();
        }
        game_render(&g_game);
        /* DMA shadow OAM to real OAM.  Runs right after game_render, so for
         * steady frames it is still inside the VBlank opened by the vsync
         * above.  A full redraw already committed the sprite inside
         * game_render (ui_sprite_commit before ui_lcd_on, while the LCD is
         * off), so this late copy is only redundant on those frames -- it
         * cannot drop the reveal to scanout. */
        ui_sprite_commit();
    }
}

#include <gb/gb.h>
#include "game.h"
#include "audio.h"
#include "input.h"
#include "ui.h"

Game g_game;
volatile uint8_t g_harness_mode;

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

    if (!g_harness_mode) {
        ui_init();
    } else {
        BGP_REG = 0xE4;
        SHOW_BKG;
        DISPLAY_ON;
    }

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

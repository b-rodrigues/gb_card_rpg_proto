#include <gb/gb.h>
#include "game.h"
#include "audio.h"
#include "input.h"
#include "ui.h"

Game g_game;

int main(void)
{
#ifndef DEBUG_BUILD
    audio_init();

    CRITICAL {
        add_VBL(audio_update);
    }

    enable_interrupts();
    
    input_init();
    ui_init();
#else
    /* Minimal init for debug harness: no audio, no fonts, no interrupts */
    BGP_REG = 0xE4;
    SHOW_BKG;
    DISPLAY_ON;
    input_init();
#endif

    game_init(&g_game);

    while (1) {
        input_update();
        game_update(&g_game);
        game_render(&g_game);
#ifndef DEBUG_BUILD
        vsync();
#endif
    }
}

#include <gb/gb.h>
#include "game.h"
#include "audio.h"
#include "input.h"
#include "ui.h"

static Game g_game;

int main(void)
{
    CRITICAL {
        audio_init();
        add_VBL(audio_update);
    }

    input_init();
    ui_init();
    game_init(&g_game);

    while (1) {
        input_update();
        game_update(&g_game);
        game_render(&g_game);
        vsync();
    }
}

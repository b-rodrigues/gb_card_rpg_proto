#include <gb/gb.h>
#include "game.h"
#include "input.h"
#include "render.h"
#include "sound.h"

void main(void) {
    sound_init();
    render_init();
    input_init();
    game_init();

    while (1) {
        input_update();
        game_update();
        game_render();
        render_vsync();
    }
}

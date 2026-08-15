#include "game.h"
#include "screen.h"
#include "telemetry.h"
#include "menu.h"

/* Final ending screen: shown after the Lord of Slimes is defeated.  A or
 * START restarts the game. */
static const char * const s_ending_lines[9] = {
    "      THE END       ",
    "--------------------",
    "",
    "The Hero cleared",
    "the land of slimes!",
    "Now peace has",
    "returned!",
    "Thank you for",
    "playing!"
};

static void ending_draw(void)
{
    uint8_t i;
    ui_clear_screen();
    for (i = 0; i < 9; i++) {
        if (s_ending_lines[i][0]) {
            ui_draw_text_line(0, (uint8_t)(1 + i), s_ending_lines[i], 20);
        }
    }
    ui_draw_text_line(0, 14, "[A] RESTART", 20);
}

void ending_screen_update(Game *g)
{
    if (!g) return;

    if (input_pressed(INPUT_A) || input_pressed(INPUT_START)) {
        game_restart(g);
    }
}

void ending_screen_render(Game *g)
{
    RenderCache *rc;

    if (!g) return;
    rc = &g->render_cache;

    if (!rc->valid || rc->prev_screen != SCREEN_ENDING) {
        ending_draw();
        telemetry_emit(EVENT_RENDER_SCREEN, (uint8_t)SCREEN_ENDING, 0, 0, 0);
        rc->valid = true;
        rc->prev_screen = SCREEN_ENDING;
    }
}

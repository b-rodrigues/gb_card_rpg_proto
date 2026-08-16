#include "game.h"
#include "screen.h"
#include "telemetry.h"
#include "menu.h"

/* Final ending screen: shown after the Lord of Slimes is defeated.  A or
 * START restarts the game. */
static void ending_draw(void)
{
    ui_clear_screen();
    ui_draw_text_line(0, 1, "      THE END       ", 20);
    ui_draw_text_line(0, 2, "--------------------", 20);
    ui_draw_text_line(0, 4, "The Hero cleared", 16);
    ui_draw_text_line(0, 5, "the land of slimes!", 19);
    ui_draw_text_line(0, 6, "Now peace has", 13);
    ui_draw_text_line(0, 7, "returned!", 9);
    ui_draw_text_line(0, 8, "Thank you for", 13);
    ui_draw_text_line(0, 9, "playing!", 8);
    ui_draw_text_line(0, 14, "[A] RESTART", 11);
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

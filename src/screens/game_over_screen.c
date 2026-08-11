#include "game.h"
#include "screen.h"
#include "telemetry.h"

void game_over_screen_update(Game *g)
{
    if (!g) return;

    /* Up/Down moves YES/NO, A/START confirms. */
    if (input_pressed(INPUT_UP) || input_pressed(INPUT_DOWN)) {
        g->game_over_choice = !g->game_over_choice;
    }
    if (input_pressed(INPUT_A) || input_pressed(INPUT_START)) {
        if (g->game_over_choice == 0) {
            game_restart(g);
        } else {
            screen_change(g, SCREEN_THANKS);
        }
    }
}

void game_over_screen_render(Game *g)
{
    RenderCache *rc;

    if (!g) return;
    rc = &g->render_cache;

    if (!rc->valid || rc->prev_screen != SCREEN_GAME_OVER ||
        g->game_over_choice != rc->prev_game_over_choice) {
        ui_draw_game_over(g->game_over_choice);
        telemetry_emit(EVENT_RENDER_SCREEN, (uint8_t)SCREEN_GAME_OVER, 0, 0, 0);
        rc->valid = true;
        rc->prev_screen = SCREEN_GAME_OVER;
        rc->prev_game_over_choice = g->game_over_choice;
    }
}

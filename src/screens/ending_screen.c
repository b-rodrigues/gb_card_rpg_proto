#include "game.h"
#include "screen.h"
#include "telemetry.h"
#include "menu.h"

/* Final ending screen: shown after the Lord of Slimes is defeated.  A or
 * START restarts the game. */
static void ending_draw(void)
{
    MenuFrame frame;

    frame.title = "THE END";
    frame.title_row = 1;
    frame.top_row = 4;
    frame.bottom_row = 15;
    frame.boxed = false;

    menu_draw_frame(&frame);
    menu_draw_content(&frame, 0, "The Hero cleared");
    menu_draw_content(&frame, 1, "the land of slimes!");
    menu_draw_content(&frame, 2, "Now peace has");
    menu_draw_content(&frame, 3, "returned!");
    menu_draw_content(&frame, 4, "Thank you for");
    menu_draw_content(&frame, 5, "playing!");
    menu_draw_content(&frame, 10, "[A] RESTART");
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

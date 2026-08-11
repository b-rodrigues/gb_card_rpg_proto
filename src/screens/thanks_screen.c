#include "game.h"
#include "screen.h"
#include "telemetry.h"

/* Terminal "Thanks for playing!" screen — no further interaction. */
void thanks_screen_update(Game *g)
{
    (void)g;
}

void thanks_screen_render(Game *g)
{
    RenderCache *rc;

    if (!g) return;
    rc = &g->render_cache;

    if (!rc->valid || rc->prev_screen != SCREEN_THANKS) {
        ui_draw_thanks();
        telemetry_emit(EVENT_RENDER_SCREEN, (uint8_t)SCREEN_THANKS, 0, 0, 0);
        rc->valid = true;
        rc->prev_screen = SCREEN_THANKS;
    }
}

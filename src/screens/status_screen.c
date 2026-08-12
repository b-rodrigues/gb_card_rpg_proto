#include "game.h"
#include "screen.h"
#include "telemetry.h"
#include "rpg/currency.h"
#include "rpg/progression.h"

/* Pause / status screen: shows the hero's HP, gold, and hero progression
 * level/progress.  Opened with START from the overworld. */
static void status_draw(Game *g)
{
    const CharacterState *hero = &g->state.party.members[0];
    int16_t gold = currency_get(&g->state, CURRENCY_ID_GOLD);
    ProgressionTarget t;
    ProgressionState *ps;
    char buf[7];

    t.type = PROG_TYPE_HERO;
    t.id = 1;
    ps = progression_get(&g->state, t);

    ui_clear_screen();
    ui_draw_text_line(0, 1, "STATUS", 20);

    ui_draw_text_line(0, 3, "HERO", 20);
    ui_draw_text_line(0, 4, "HP:", 3);
    ui_format_int((int16_t)hero->hp, buf);
    ui_draw_text_line(4, 4, buf, 4);
    ui_draw_text_line(8, 4, "/", 1);
    ui_format_int((int16_t)hero->max_hp, buf);
    ui_draw_text_line(9, 4, buf, 4);

    ui_draw_text_line(0, 6, "GOLD:", 6);
    ui_format_int(gold, buf);
    ui_draw_text_line(6, 6, buf, 12);

    ui_draw_text_line(0, 8, "LEVEL:", 6);
    ui_format_int((int16_t)(ps ? ps->level : 1), buf);
    ui_draw_text_line(6, 8, buf, 4);

    ui_draw_text_line(0, 9, "PROGRESS:", 9);
    ui_format_int((int16_t)(ps ? ps->progress : 0), buf);
    ui_draw_text_line(10, 9, buf, 6);

    ui_draw_text_line(0, 14, "[START/B] CLOSE", 20);
}

void status_screen_update(Game *g)
{
    if (!g) return;

    if (input_pressed(INPUT_B) || input_pressed(INPUT_START)) {
        screen_change(g, SCREEN_OVERWORLD);
    }
}

void status_screen_render(Game *g)
{
    RenderCache *rc;

    if (!g) return;
    rc = &g->render_cache;

    if (!rc->valid || rc->prev_screen != SCREEN_STATUS) {
        status_draw(g);
        telemetry_emit(EVENT_RENDER_SCREEN, (uint8_t)SCREEN_STATUS, 0, 0, 0);
        rc->valid = true;
        rc->prev_screen = SCREEN_STATUS;
    }
}

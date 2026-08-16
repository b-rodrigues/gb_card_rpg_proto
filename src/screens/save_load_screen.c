#include "game.h"
#include "screen.h"
#include "telemetry.h"
#include "rpg/save.h"
#include "menu.h"
#include "scene.h"

static void save_load_draw(Game *g)
{
    uint8_t i, y;
    MenuFrame frame;
    frame.title = (g->save_slot_mode == 1) ? "SAVE GAME" : "LOAD GAME";
    frame.title_row = 0;
    frame.top_row = 3;
    frame.bottom_row = 15;
    menu_draw_frame(&frame);

    for (i = 0; i < SAVE_SLOT_COUNT; i++) {
        y = (uint8_t)(4 + (i << 1));
        ui_draw_text_line(0, y, (g->save_slot_index == i) ? ">" : " ", 1);
        ui_draw_text_line(1, y, (i == 0) ? "SLOT 1:" : ((i == 1) ? "SLOT 2:" : "SLOT 3:"), 7);
        ui_draw_text_line(9, y, save_present_slot(i) ? "SAVED" : "(EMPTY)", 7);
    }
    ui_draw_text_line(0, 11, (g->save_slot_mode == 1) ? "[A] SAVE  [B] BACK" : "[A] LOAD  [B] BACK", 18);
    if (g->save_slot_message == 1) {
        ui_draw_text_line(2, 13, "Game Saved!", 11);
    } else if (g->save_slot_message == 2) {
        ui_draw_text_line(2, 13, "Slot is empty!", 14);
    }
}

void save_load_screen_update(Game *g)
{
    uint8_t idx;
    if (!g) return;
    idx = g->save_slot_index;
    if (input_pressed(INPUT_UP) && idx > 0) {
        g->save_slot_index = (uint8_t)(idx - 1);
        g->save_slot_message = 0;
        g->render_cache.valid = false;
    } else if (input_pressed(INPUT_DOWN) && idx < (SAVE_SLOT_COUNT - 1)) {
        g->save_slot_index = (uint8_t)(idx + 1);
        g->save_slot_message = 0;
        g->render_cache.valid = false;
    } else if (input_pressed(INPUT_A)) {
        if (g->save_slot_mode == 1) {
            scene_sync_from_world(g);
            save_game_slot(idx, &g->state);
            g->save_slot_message = 1;
            telemetry_emit(EVENT_GAME_SAVED, (uint8_t)(idx + 1), 0, 0, 0);
        } else if (save_present_slot(idx)) {
            load_game_slot(idx, &g->state);
            telemetry_emit(EVENT_GAME_LOADED, (uint8_t)(idx + 1), 0, 0, 0);
            scene_load(g, g->state.scene.scene_id, g->state.scene.player_x, g->state.scene.player_y);
            g->world.player.facing = (Direction)g->state.scene.player_facing;
            screen_change(g, SCREEN_OVERWORLD);
            return;
        } else {
            g->save_slot_message = 2;
        }
        g->render_cache.valid = false;
    } else if (input_pressed(INPUT_B) || input_pressed(INPUT_START)) {
        screen_change(g, SCREEN_OVERWORLD);
    }
}

void save_load_screen_render(Game *g)
{
    RenderCache *rc;
    if (!g) return;
    rc = &g->render_cache;
    if (!rc->valid || rc->prev_screen != SCREEN_SAVE_LOAD) {
        save_load_draw(g);
        telemetry_emit(EVENT_RENDER_SCREEN, (uint8_t)SCREEN_SAVE_LOAD, 0, 0, 0);
        rc->valid = true;
        rc->prev_screen = SCREEN_SAVE_LOAD;
    }
}

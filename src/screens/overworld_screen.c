#include "game.h"
#include "screen.h"
#include "dialogue.h"
#include "interaction.h"
#include "story.h"
#include "telemetry.h"
#include "audio.h"

void overworld_screen_update(Game *g)
{
    int8_t dx = 0;
    int8_t dy = 0;
    WorldMoveResult move_res = MOVE_RESULT_NONE;

    if (input_pressed(INPUT_UP))    dy = -1;
    if (input_pressed(INPUT_DOWN))  dy = 1;
    if (input_pressed(INPUT_LEFT))  dx = -1;
    if (input_pressed(INPUT_RIGHT)) dx = 1;

    if (dx != 0 || dy != 0) {
        move_res = world_move_player(&g->world, dx, dy);

        if (move_res == MOVE_RESULT_MAP_CHANGED) {
            g->world.map_changed = false;
            story_on_map_enter(&g->story_flags, g->world.map_id);
            return;
        }
    }

    /* Check interaction: PRESS A checks facing tile; movement bump only
     * triggers when the move was blocked (player walked into an NPC). */
    if (input_pressed(INPUT_A)) {
        if (interaction_try_facing(&g->world, &g->dialogue)) {
            screen_change(g, SCREEN_DIALOGUE);
            return;
        }
    } else if (move_res == MOVE_RESULT_BLOCKED) {
        if (interaction_try_bump(&g->world, dx, dy, &g->dialogue)) {
            screen_change(g, SCREEN_DIALOGUE);
            return;
        }
    }

    if (g->world.encounter_triggered) {
        g->world.encounter_triggered = false;
        battle_start(&g->battle, g->world.player.hp, g->world.player.max_hp,
                     g->world.enemy.hp, g->world.enemy.max_hp);
        audio_play_music(MUSIC_BATTLE);
        telemetry_emit(EVENT_MUSIC_CHANGED, MUSIC_BATTLE, 0, 0, 0);
        screen_change(g, SCREEN_BATTLE);
    }
}

void overworld_screen_render(Game *g)
{
    RenderCache *rc;

    if (!g) return;
    rc = &g->render_cache;

    /* Map transition, cache reset, or return from Battle/Dialogue */
    if (!rc->valid || rc->prev_screen != SCREEN_OVERWORLD ||
        g->world.map_id != rc->prev_map_id) {
        ui_draw_world_full(&g->world);
        telemetry_emit(EVENT_RENDER_SCREEN, (uint8_t)SCREEN_OVERWORLD, 0,
                       (uint8_t)g->world.map_id, 0);
        rc->valid = true;
        rc->prev_screen = SCREEN_OVERWORLD;
        rc->prev_map_id = g->world.map_id;
        rc->prev_player_x = g->world.player.position.x;
        rc->prev_player_y = g->world.player.position.y;
        rc->prev_dialogue_active = false;
        rc->prev_dialogue_line = 255;
        rc->prev_dialogue_id = DIALOGUE_ID_NONE;
        return;
    }

    /* Incremental overworld player movement (NO full screen clear) */
    if (g->world.player.position.x != rc->prev_player_x ||
        g->world.player.position.y != rc->prev_player_y) {
        ui_update_player_position(&g->world, rc->prev_player_x, rc->prev_player_y,
                                  g->world.player.position.x, g->world.player.position.y);
        rc->prev_player_x = g->world.player.position.x;
        rc->prev_player_y = g->world.player.position.y;
    }
}

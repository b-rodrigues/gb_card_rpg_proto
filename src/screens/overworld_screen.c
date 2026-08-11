#include "game.h"
#include "screen.h"
#include "scene.h"
#include "interaction.h"
#include "story.h"
#include "telemetry.h"
#include "audio.h"

static void start_battle_from_world(Game *g)
{
    uint8_t idx = g->world.encounter_actor_index;
    if (idx == NO_ACTOR_INDEX) return;

    /* Hero HP is authoritative in the party state; the world entity is the
     * runtime engine copy. */
    battle_start(&g->battle, g->state.party.members[0].hp,
                 g->state.party.members[0].max_hp,
                 g->world.actors[idx].hp, g->world.actors[idx].max_hp);
    audio_play_music(MUSIC_BATTLE);
    telemetry_emit(EVENT_MUSIC_CHANGED, MUSIC_BATTLE, 0, 0, 0);
    telemetry_emit(EVENT_ACTOR_COMBAT_START, (uint8_t)g->world.actors[idx].id, 0, 0, 0);
    screen_change(g, SCREEN_BATTLE);
    /* encounter_actor_index stays set until world_on_battle_end() */
}

void overworld_screen_update(Game *g)
{
    int8_t dx = 0;
    int8_t dy = 0;
    WorldMoveResult move_res = MOVE_RESULT_NONE;
    ActorEngageResult engage = ENGAGE_NONE;

    if (input_pressed(INPUT_UP))    dy = -1;
    if (input_pressed(INPUT_DOWN))  dy = 1;
    if (input_pressed(INPUT_LEFT))  dx = -1;
    if (input_pressed(INPUT_RIGHT)) dx = 1;

    if (dx != 0 || dy != 0) {
        move_res = world_move_player(&g->world, dx, dy);

        if (move_res == MOVE_RESULT_MAP_CHANGED) {
            g->world.map_changed = false;
            scene_update_from_map(g);
            story_on_map_enter(&g->state, g->world.map_id);
            return;
        }
    }

    if (input_pressed(INPUT_A)) {
        engage = interaction_try_facing(&g->world, &g->dialogue);
    } else if (move_res == MOVE_RESULT_BLOCKED) {
        engage = interaction_try_bump(&g->world, dx, dy, &g->dialogue);
    }

    if (engage == ENGAGE_DIALOGUE) {
        screen_change(g, SCREEN_DIALOGUE);
        return;
    } else if (engage == ENGAGE_BATTLE) {
        start_battle_from_world(g);
        return;
    }

    if (g->world.encounter_actor_index != NO_ACTOR_INDEX) {
        start_battle_from_world(g);
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

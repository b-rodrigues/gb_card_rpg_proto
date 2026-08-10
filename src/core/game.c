#include "game.h"
#include "story.h"
#include "dialogue.h"
#include "telemetry.h"
#include "scenarios.h"

static const char *g_mayor_lines[2] = {
    "Welcome, traveler.",
    "Road is dangerous."
};

void game_init(Game *g)
{
    if (!g) return;
    g->frame = 0;
    g->story_flags = 0;
    telemetry_init();
    telemetry_set_frame_ptr(&g->frame);
    state_init(&g->state_machine);
    world_init(&g->world);
    dialogue_init(&g->dialogue);
    audio_play_music(MUSIC_OVERWORLD);
    telemetry_emit(EVENT_MUSIC_CHANGED, MUSIC_OVERWORLD, 0, 0, 0);
    ui_draw_world_full(&g->world);

#ifdef DEBUG_BUILD
    debug_snapshot();
#endif
}

static void update_overworld(Game *g)
{
    int8_t dx = 0;
    int8_t dy = 0;
    uint8_t old_x, old_y;

    if (g->dialogue.active) {
        if (input_pressed(INPUT_A)) {
            if (dialogue_next(&g->dialogue)) {
                ui_draw_dialogue(&g->dialogue);
            } else {
                story_set_flag(&g->story_flags, STORY_FLAG_ID_MET_MAYOR);
                ui_draw_world_full(&g->world);
            }
        }
        return;
    }

    if (input_pressed(INPUT_UP))    dy = -1;
    if (input_pressed(INPUT_DOWN))  dy = 1;
    if (input_pressed(INPUT_LEFT))  dx = -1;
    if (input_pressed(INPUT_RIGHT)) dx = 1;

    if (dx != 0 || dy != 0) {
        old_x = g->world.player.position.x;
        old_y = g->world.player.position.y;
        world_move_player(&g->world, dx, dy);

        if (g->world.map_changed) {
            g->world.map_changed = false;
            story_on_map_enter(&g->story_flags, g->world.map_id);
        } else if (!g->world.encounter_triggered) {
            ui_update_player_position(old_x, old_y, g->world.player.position.x, g->world.player.position.y);
        }
    }

    /* Check NPC Mayor interaction (A press or adjacent bump) */
    if (g->world.map_id == MAP_TOWN && g->world.npc.active) {
        uint8_t px = g->world.player.position.x;
        uint8_t py = g->world.player.position.y;
        uint8_t nx = g->world.npc.position.x;
        uint8_t ny = g->world.npc.position.y;

        /* Adjacent check: dist_x + dist_y == 1 */
        uint8_t dist_x = (px > nx) ? (px - nx) : (nx - px);
        uint8_t dist_y = (py > ny) ? (py - ny) : (ny - py);

        if ((dist_x + dist_y == 1) && (input_pressed(INPUT_A) || (dx != 0 || dy != 0))) {
            dialogue_start(&g->dialogue, DIALOGUE_ID_MAYOR_GREETING, "MAYOR:", g_mayor_lines, 2);
            ui_draw_dialogue(&g->dialogue);
            return;
        }
    }

    if (g->world.encounter_triggered) {
        state_set(&g->state_machine, GAME_STATE_BATTLE);
        battle_start(&g->battle, g->world.player.hp, g->world.player.max_hp, g->world.enemy.hp, g->world.enemy.max_hp);
        audio_play_music(MUSIC_BATTLE);
        telemetry_emit(EVENT_MUSIC_CHANGED, MUSIC_BATTLE, 0, 0, 0);
        ui_draw_battle_full(&g->battle);
    }
}

static void update_battle(Game *g)
{
    if (g->battle.battle_over) {
        if (input_pressed(INPUT_A) || input_pressed(INPUT_START)) {
            bool victory = (g->battle.result == BATTLE_RESULT_VICTORY);
            if (victory) {
                g->world.player.hp = g->battle.player.hp;
            }
            world_on_battle_end(&g->world, victory);
            state_set(&g->state_machine, GAME_STATE_OVERWORLD);
            audio_play_music(MUSIC_OVERWORLD);
            telemetry_emit(EVENT_MUSIC_CHANGED, MUSIC_OVERWORLD, 0, 0, 0);
            ui_draw_world_full(&g->world);
        }
    } else {
        if (g->battle.turn == BATTLE_TURN_PLAYER) {
            if (input_pressed(INPUT_A)) {
                battle_execute_action(&g->battle, BATTLE_ACTION_ATTACK);
                ui_update_battle(&g->battle);
            } else if (input_pressed(INPUT_B)) {
                battle_execute_action(&g->battle, BATTLE_ACTION_RUN);
                ui_update_battle(&g->battle);
            }
        }
        battle_update(&g->battle);
        ui_update_battle(&g->battle);
    }
}

void game_update(Game *g)
{
    if (!g) return;

#ifdef DEBUG_BUILD
    scenario_check_and_load();
#endif
    g->frame++;

    if (g->state_machine.state_changed) {
        telemetry_emit(EVENT_GAME_STATE_CHANGED, g->state_machine.previous, g->state_machine.current, 0, 0);
        g->state_machine.state_changed = false;
    }

    switch (g->state_machine.current) {
        case GAME_STATE_OVERWORLD:
            update_overworld(g);
            break;
        case GAME_STATE_BATTLE:
            update_battle(g);
            break;
    }

#ifdef DEBUG_BUILD
    debug_snapshot();
#endif
}

void game_render(const Game *g)
{
    (void)g;
}

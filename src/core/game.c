#include "game.h"
#include "story.h"
#include "dialogue.h"
#include "interaction.h"
#include "telemetry.h"
#include "scenarios.h"

void game_render_reset(Game *g)
{
    if (!g) return;
    g->render_cache.valid = false;
    g->render_cache.prev_state = (GameState)255;
    g->render_cache.prev_map_id = (MapId)255;
    g->render_cache.prev_player_x = 255;
    g->render_cache.prev_player_y = 255;
    g->render_cache.prev_dialogue_active = false;
    g->render_cache.prev_dialogue_line = 255;
    g->render_cache.prev_dialogue_id = DIALOGUE_ID_NONE;
    g->render_cache.prev_battle_turn = (BattleTurn)255;
    g->render_cache.prev_player_hp = 255;
    g->render_cache.prev_enemy_hp = 255;
    g->render_cache.prev_battle_result = (BattleResult)255;
    g->render_cache.prev_game_over_choice = 255;
}

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

    game_render_reset(g);
    game_render(g);

#ifdef DEBUG_BUILD
    debug_snapshot();
#endif
}

/* Reset the world to a fresh new-game state (used by the Continue? menu). */
void game_restart(Game *g)
{
    if (!g) return;
    g->frame = 0;
    g->story_flags = 0;
    g->game_over_choice = 0;
    state_init(&g->state_machine);
    world_init(&g->world);
    dialogue_init(&g->dialogue);
    audio_play_music(MUSIC_OVERWORLD);
    telemetry_emit(EVENT_MUSIC_CHANGED, MUSIC_OVERWORLD, 0, 0, 0);
    game_render_reset(g);
}

static void update_overworld(Game *g)
{
    int8_t dx = 0;
    int8_t dy = 0;
    WorldMoveResult move_res = MOVE_RESULT_NONE;

    if (g->dialogue.active) {
        if (input_pressed(INPUT_A)) {
            if (!dialogue_next(&g->dialogue)) {
                interaction_on_dialogue_end(&g->dialogue, &g->story_flags);
            }
        }
        return;
    }

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
            return;
        }
    } else if (move_res == MOVE_RESULT_BLOCKED) {
        if (interaction_try_bump(&g->world, dx, dy, &g->dialogue)) {
            return;
        }
    }

    if (g->world.encounter_triggered) {
        state_set(&g->state_machine, GAME_STATE_BATTLE);
        battle_start(&g->battle, g->world.player.hp, g->world.player.max_hp, g->world.enemy.hp, g->world.enemy.max_hp);
        audio_play_music(MUSIC_BATTLE);
        telemetry_emit(EVENT_MUSIC_CHANGED, MUSIC_BATTLE, 0, 0, 0);
    }
}

static void update_battle(Game *g)
{
    if (g->battle.battle_over) {
        if (input_pressed(INPUT_A) || input_pressed(INPUT_START)) {
            bool victory = (g->battle.result == BATTLE_RESULT_VICTORY);
            if (victory) {
                g->world.player.hp = g->battle.player.hp;
                world_on_battle_end(&g->world, victory);
                state_set(&g->state_machine, GAME_STATE_OVERWORLD);
                audio_play_music(MUSIC_OVERWORLD);
                telemetry_emit(EVENT_MUSIC_CHANGED, MUSIC_OVERWORLD, 0, 0, 0);
            } else {
                world_on_battle_end(&g->world, false);
                g->game_over_choice = 0;
                state_set(&g->state_machine, GAME_STATE_GAME_OVER);
            }
        }
    } else {
        if (g->battle.turn == BATTLE_TURN_PLAYER) {
            if (input_pressed(INPUT_A)) {
                battle_execute_action(&g->battle, BATTLE_ACTION_ATTACK);
            } else if (input_pressed(INPUT_B)) {
                battle_execute_action(&g->battle, BATTLE_ACTION_RUN);
            }
        }
        battle_update(&g->battle);
    }
}

/* Game-over continue prompt: Up/Down moves YES/NO, A/START confirms. */
static void update_game_over(Game *g)
{
    if (input_pressed(INPUT_UP) || input_pressed(INPUT_DOWN)) {
        g->game_over_choice = !g->game_over_choice;
    }
    if (input_pressed(INPUT_A) || input_pressed(INPUT_START)) {
        if (g->game_over_choice == 0) {
            game_restart(g);
        } else {
            state_set(&g->state_machine, GAME_STATE_THANKS);
        }
    }
}

/* Terminal "Thanks for playing!" screen — no further interaction. */
static void update_thanks(Game *g)
{
    (void)g;
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
        case GAME_STATE_GAME_OVER:
            update_game_over(g);
            break;
        case GAME_STATE_THANKS:
            update_thanks(g);
            break;
    }

#ifdef DEBUG_BUILD
    debug_snapshot();
#endif
}

void game_render(Game *g)
{
    RenderCache *rc;

    if (!g) return;
    rc = &g->render_cache;

    /* Handle Battle State rendering */
    if (g->state_machine.current == GAME_STATE_BATTLE) {
        if (!rc->valid || rc->prev_state != GAME_STATE_BATTLE) {
            ui_draw_battle_full(&g->battle);
            telemetry_emit(EVENT_RENDER_SCREEN, (uint8_t)GAME_STATE_BATTLE, 0, 0, 0);
            rc->valid = true;
            rc->prev_state = GAME_STATE_BATTLE;
            rc->prev_battle_turn = g->battle.turn;
            rc->prev_player_hp = g->battle.player.hp;
            rc->prev_enemy_hp = g->battle.enemy.hp;
            rc->prev_battle_result = g->battle.result;
        } else if (g->battle.turn != rc->prev_battle_turn ||
                   g->battle.player.hp != rc->prev_player_hp ||
                   g->battle.enemy.hp != rc->prev_enemy_hp ||
                   g->battle.result != rc->prev_battle_result) {
            ui_update_battle(&g->battle);
            rc->prev_battle_turn = g->battle.turn;
            rc->prev_player_hp = g->battle.player.hp;
            rc->prev_enemy_hp = g->battle.enemy.hp;
            rc->prev_battle_result = g->battle.result;
        }
        return;
    }

    /* Game over continue prompt */
    if (g->state_machine.current == GAME_STATE_GAME_OVER) {
        if (!rc->valid || rc->prev_state != GAME_STATE_GAME_OVER ||
            g->game_over_choice != rc->prev_game_over_choice) {
            ui_draw_game_over(g->game_over_choice);
            telemetry_emit(EVENT_RENDER_SCREEN, (uint8_t)GAME_STATE_GAME_OVER, 0, 0, 0);
            rc->valid = true;
            rc->prev_state = GAME_STATE_GAME_OVER;
            rc->prev_game_over_choice = g->game_over_choice;
        }
        return;
    }

    /* Terminal thanks screen */
    if (g->state_machine.current == GAME_STATE_THANKS) {
        if (!rc->valid || rc->prev_state != GAME_STATE_THANKS) {
            ui_draw_thanks();
            telemetry_emit(EVENT_RENDER_SCREEN, (uint8_t)GAME_STATE_THANKS, 0, 0, 0);
            rc->valid = true;
            rc->prev_state = GAME_STATE_THANKS;
        }
        return;
    }

    /* Overworld State rendering */

    /* Map transition, cache reset, or return from Battle */
    if (!rc->valid || rc->prev_state != GAME_STATE_OVERWORLD || g->world.map_id != rc->prev_map_id) {
        ui_draw_world_full(&g->world);
        telemetry_emit(EVENT_RENDER_SCREEN, (uint8_t)GAME_STATE_OVERWORLD, g->dialogue.active ? 1 : 0, (uint8_t)g->world.map_id, 0);
        rc->valid = true;
        rc->prev_state = GAME_STATE_OVERWORLD;
        rc->prev_map_id = g->world.map_id;
        rc->prev_player_x = g->world.player.position.x;
        rc->prev_player_y = g->world.player.position.y;
        rc->prev_dialogue_active = g->dialogue.active;
        rc->prev_dialogue_line = g->dialogue.current_line;
        rc->prev_dialogue_id = g->dialogue.id;
        if (g->dialogue.active) {
            ui_draw_dialogue(&g->dialogue);
            telemetry_emit(EVENT_RENDER_DIALOGUE, (uint8_t)g->dialogue.id, g->dialogue.current_line, 0, 0);
        }
        return;
    }

    /* Dialogue closed: restore overworld HUD on rows 12-17 */
    if (rc->prev_dialogue_active && !g->dialogue.active) {
        ui_draw_overworld_hud(&g->world);
        telemetry_emit(EVENT_RENDER_SCREEN, (uint8_t)GAME_STATE_OVERWORLD, 0, (uint8_t)g->world.map_id, 0);
        rc->prev_dialogue_active = false;
        rc->prev_dialogue_line = 255;
        rc->prev_dialogue_id = DIALOGUE_ID_NONE;
        return;
    }

    /* Dialogue active or dialogue line changed */
    if (g->dialogue.active) {
        if (!rc->prev_dialogue_active ||
            g->dialogue.current_line != rc->prev_dialogue_line ||
            g->dialogue.id != rc->prev_dialogue_id) {
            
            ui_draw_dialogue(&g->dialogue);
            telemetry_emit(EVENT_RENDER_DIALOGUE, (uint8_t)g->dialogue.id, g->dialogue.current_line, 0, 0);
            rc->prev_dialogue_active = true;
            rc->prev_dialogue_line = g->dialogue.current_line;
            rc->prev_dialogue_id = g->dialogue.id;
        }
        return;
    }

    /* Incremental overworld player movement (NO full screen clear, NO map redraw) */
    if (g->world.player.position.x != rc->prev_player_x || g->world.player.position.y != rc->prev_player_y) {
        ui_update_player_position(&g->world, rc->prev_player_x, rc->prev_player_y, g->world.player.position.x, g->world.player.position.y);
        rc->prev_player_x = g->world.player.position.x;
        rc->prev_player_y = g->world.player.position.y;
    }
}

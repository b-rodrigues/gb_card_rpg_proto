#include "game.h"
#include "story.h"
#include "dialogue.h"
#include "interaction.h"
#include "telemetry.h"
#include "scenarios.h"

static GameState s_prev_state = (GameState)255;
static MapId s_prev_map_id = (MapId)255;
static uint8_t s_prev_player_x = 255;
static uint8_t s_prev_player_y = 255;
static bool s_prev_dialogue_active = false;
static uint8_t s_prev_dialogue_line = 255;
static DialogueId s_prev_dialogue_id = DIALOGUE_ID_NONE;
static BattleTurn s_prev_battle_turn = (BattleTurn)255;
static uint8_t s_prev_player_hp = 255;
static uint8_t s_prev_enemy_hp = 255;
static BattleResult s_prev_battle_result = (BattleResult)255;

void game_render_reset(void)
{
    s_prev_state = (GameState)255;
    s_prev_map_id = (MapId)255;
    s_prev_player_x = 255;
    s_prev_player_y = 255;
    s_prev_dialogue_active = false;
    s_prev_dialogue_line = 255;
    s_prev_dialogue_id = DIALOGUE_ID_NONE;
    s_prev_battle_turn = (BattleTurn)255;
    s_prev_player_hp = 255;
    s_prev_enemy_hp = 255;
    s_prev_battle_result = (BattleResult)255;
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
    game_render_reset();

#ifdef DEBUG_BUILD
    debug_snapshot();
#endif
}

static void update_overworld(Game *g)
{
    int8_t dx = 0;
    int8_t dy = 0;

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
        world_move_player(&g->world, dx, dy);

        if (g->world.map_changed) {
            g->world.map_changed = false;
            story_on_map_enter(&g->story_flags, g->world.map_id);
            return;
        }
    }

    /* Check interaction: PRESS A checks facing tile; movement bump checks targeted tile */
    if (input_pressed(INPUT_A)) {
        if (interaction_try_facing(&g->world, &g->dialogue)) {
            return;
        }
    } else if (dx != 0 || dy != 0) {
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
            }
            world_on_battle_end(&g->world, victory);
            state_set(&g->state_machine, GAME_STATE_OVERWORLD);
            audio_play_music(MUSIC_OVERWORLD);
            telemetry_emit(EVENT_MUSIC_CHANGED, MUSIC_OVERWORLD, 0, 0, 0);
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
    if (!g) return;

    /* Handle Game State transitions (e.g., OVERWORLD <-> BATTLE) */
    if (g->state_machine.current != s_prev_state) {
        s_prev_state = g->state_machine.current;
        if (g->state_machine.current == GAME_STATE_BATTLE) {
            ui_draw_battle_full(&g->battle);
            s_prev_battle_turn = g->battle.turn;
            s_prev_player_hp = g->battle.player.hp;
            s_prev_enemy_hp = g->battle.enemy.hp;
            s_prev_battle_result = g->battle.result;
            return;
        } else {
            ui_draw_world_full(&g->world);
            s_prev_map_id = g->world.map_id;
            s_prev_player_x = g->world.player.position.x;
            s_prev_player_y = g->world.player.position.y;
            s_prev_dialogue_active = g->dialogue.active;
            s_prev_dialogue_line = g->dialogue.current_line;
            s_prev_dialogue_id = g->dialogue.id;
            if (g->dialogue.active) {
                ui_draw_dialogue(&g->dialogue);
            }
            return;
        }
    }

    if (g->state_machine.current == GAME_STATE_BATTLE) {
        if (g->battle.turn != s_prev_battle_turn ||
            g->battle.player.hp != s_prev_player_hp ||
            g->battle.enemy.hp != s_prev_enemy_hp ||
            g->battle.result != s_prev_battle_result) {
            
            ui_update_battle(&g->battle);
            s_prev_battle_turn = g->battle.turn;
            s_prev_player_hp = g->battle.player.hp;
            s_prev_enemy_hp = g->battle.enemy.hp;
            s_prev_battle_result = g->battle.result;
        }
        return;
    }

    /* GAME_STATE_OVERWORLD rendering */

    /* Map transition */
    if (g->world.map_id != s_prev_map_id) {
        ui_draw_world_full(&g->world);
        s_prev_map_id = g->world.map_id;
        s_prev_player_x = g->world.player.position.x;
        s_prev_player_y = g->world.player.position.y;
        s_prev_dialogue_active = g->dialogue.active;
        s_prev_dialogue_line = g->dialogue.current_line;
        s_prev_dialogue_id = g->dialogue.id;
        if (g->dialogue.active) {
            ui_draw_dialogue(&g->dialogue);
        }
        return;
    }

    /* Dialogue closed: restore overworld UI rows 15 and 17 */
    if (s_prev_dialogue_active && !g->dialogue.active) {
        ui_draw_world_full(&g->world);
        s_prev_dialogue_active = false;
        s_prev_dialogue_line = 255;
        s_prev_dialogue_id = DIALOGUE_ID_NONE;
        s_prev_player_x = g->world.player.position.x;
        s_prev_player_y = g->world.player.position.y;
        return;
    }

    /* Dialogue active or text line changed */
    if (g->dialogue.active) {
        if (!s_prev_dialogue_active ||
            g->dialogue.current_line != s_prev_dialogue_line ||
            g->dialogue.id != s_prev_dialogue_id) {
            
            ui_draw_dialogue(&g->dialogue);
            s_prev_dialogue_active = true;
            s_prev_dialogue_line = g->dialogue.current_line;
            s_prev_dialogue_id = g->dialogue.id;
        }
        return;
    }

    /* Player position update */
    if (g->world.player.position.x != s_prev_player_x || g->world.player.position.y != s_prev_player_y) {
        ui_update_player_position(s_prev_player_x, s_prev_player_y, g->world.player.position.x, g->world.player.position.y);
        s_prev_player_x = g->world.player.position.x;
        s_prev_player_y = g->world.player.position.y;
    }
}

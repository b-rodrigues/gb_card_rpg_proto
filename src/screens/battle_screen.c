#include "game.h"
#include "screen.h"
#include "battle.h"
#include "telemetry.h"
#include "audio.h"

void battle_screen_update(Game *g)
{
    if (!g) return;

    if (g->battle.battle_over) {
        if (input_pressed(INPUT_A) || input_pressed(INPUT_START)) {
            bool victory = (g->battle.result == BATTLE_RESULT_VICTORY);
            if (victory) {
                g->world.player.hp = g->battle.player.hp;
                g->state.party.members[0].hp = g->battle.player.hp;
                world_on_battle_end(g, true);
                audio_play_music(MUSIC_OVERWORLD);
                telemetry_emit(EVENT_MUSIC_CHANGED, MUSIC_OVERWORLD, 0, 0, 0);
                screen_change(g, SCREEN_OVERWORLD);
            } else {
                world_on_battle_end(g, false);
                g->game_over_choice = 0;
                screen_change(g, SCREEN_GAME_OVER);
            }
        }
        return;
    }

    if (g->battle.turn == BATTLE_TURN_PLAYER) {
        if (input_pressed(INPUT_START)) {
            g->item_menu_index = 0;
            g->item_menu_tab = 0;
            g->item_menu_tab_focus = 0;
            screen_change(g, SCREEN_ITEM);
        } else if (input_pressed(INPUT_A)) {
            battle_execute_action(&g->battle, BATTLE_ACTION_ATTACK);
        } else if (input_pressed(INPUT_B)) {
            battle_execute_action(&g->battle, BATTLE_ACTION_RUN);
        }
    }
    battle_update(&g->battle);
}

void battle_screen_render(Game *g)
{
    RenderCache *rc;

    if (!g) return;
    rc = &g->render_cache;

    if (!rc->valid || rc->prev_screen != SCREEN_BATTLE) {
        ui_draw_battle_full(&g->battle);
        telemetry_emit(EVENT_RENDER_SCREEN, (uint8_t)SCREEN_BATTLE, 0, 0, 0);
        rc->valid = true;
        rc->prev_screen = SCREEN_BATTLE;
        rc->prev_battle_turn = g->battle.turn;
        rc->prev_player_hp = g->battle.player.hp;
        rc->prev_enemy_hp = g->battle.enemy.hp;
        rc->prev_battle_result = g->battle.result;
        return;
    }

    if (g->battle.turn != rc->prev_battle_turn ||
        g->battle.player.hp != rc->prev_player_hp ||
        g->battle.enemy.hp != rc->prev_enemy_hp ||
        g->battle.result != rc->prev_battle_result) {
        ui_update_battle(&g->battle);
        rc->prev_battle_turn = g->battle.turn;
        rc->prev_player_hp = g->battle.player.hp;
        rc->prev_enemy_hp = g->battle.enemy.hp;
        rc->prev_battle_result = g->battle.result;
    }
}

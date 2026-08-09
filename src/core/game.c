#include "game.h"

void game_init(Game *g)
{
    if (!g) return;
    state_init(&g->state_machine);
    world_init(&g->world);
    audio_play_music(MUSIC_OVERWORLD);
    ui_draw_world_full(&g->world);
}

static void update_overworld(Game *g)
{
    int8_t dx = 0;
    int8_t dy = 0;
    uint8_t old_x, old_y;

    if (input_pressed(INPUT_UP))    dy = -1;
    if (input_pressed(INPUT_DOWN))  dy = 1;
    if (input_pressed(INPUT_LEFT))  dx = -1;
    if (input_pressed(INPUT_RIGHT)) dx = 1;

    if (dx != 0 || dy != 0) {
        old_x = g->world.player.position.x;
        old_y = g->world.player.position.y;
        world_move_player(&g->world, dx, dy);

        if (!g->world.encounter_triggered) {
            ui_update_player_position(old_x, old_y, g->world.player.position.x, g->world.player.position.y);
        }
    }

    if (g->world.encounter_triggered) {
        state_set(&g->state_machine, GAME_STATE_BATTLE);
        battle_start(&g->battle, g->world.player.hp, g->world.player.max_hp, g->world.enemy.hp, g->world.enemy.max_hp);
        audio_play_music(MUSIC_BATTLE);
        ui_draw_battle_full(&g->battle);
    }
}

static void update_battle(Game *g)
{
    if (g->battle.battle_over) {
        if (input_pressed(INPUT_A) || input_pressed(INPUT_START)) {
            if (g->battle.result == BATTLE_RESULT_VICTORY) {
                g->world.player.hp = g->battle.player.hp;
            }
            world_reset_encounter(&g->world);
            state_set(&g->state_machine, GAME_STATE_OVERWORLD);
            audio_play_music(MUSIC_OVERWORLD);
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

    if (g->state_machine.state_changed) {
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
}

void game_render(const Game *g)
{
    (void)g;
}

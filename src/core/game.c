#include "game.h"
#include "story.h"
#include "dialogue.h"
#include "interaction.h"
#include "telemetry.h"
#include "scenarios.h"

typedef enum {
    RENDER_MODE_OVERWORLD,
    RENDER_MODE_DIALOGUE,
    RENDER_MODE_BATTLE
} RenderMode;

static RenderMode s_render_mode = RENDER_MODE_OVERWORLD;

void game_render_reset(void)
{
    ui_draw_world_full(&g_game.world);
    if (g_game.dialogue.active) {
        ui_draw_dialogue(&g_game.dialogue);
        s_render_mode = RENDER_MODE_DIALOGUE;
    } else if (g_game.state_machine.current == GAME_STATE_BATTLE) {
        ui_draw_battle_full(&g_game.battle);
        s_render_mode = RENDER_MODE_BATTLE;
    } else {
        s_render_mode = RENDER_MODE_OVERWORLD;
    }
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

    ui_draw_world_full(&g->world);
    s_render_mode = RENDER_MODE_OVERWORLD;

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
                /* Line advanced: update ONLY the dialogue overlay rows 12-17 */
                ui_draw_dialogue(&g->dialogue);
            } else {
                /* Dialogue ended: restore overworld screen & HUD */
                interaction_on_dialogue_end(&g->dialogue, &g->story_flags);
                ui_draw_world_full(&g->world);
                s_render_mode = RENDER_MODE_OVERWORLD;
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
            ui_draw_world_full(&g->world);
            s_render_mode = RENDER_MODE_OVERWORLD;
            return;
        } else if (!g->world.encounter_triggered) {
            /* Incremental tile update: NO full screen clear or redraw */
            ui_update_player_position(&g->world, old_x, old_y, g->world.player.position.x, g->world.player.position.y);
        }
    }

    /* Check interaction: PRESS A checks facing tile; movement bump checks targeted tile */
    if (input_pressed(INPUT_A)) {
        if (interaction_try_facing(&g->world, &g->dialogue)) {
            ui_draw_dialogue(&g->dialogue);
            s_render_mode = RENDER_MODE_DIALOGUE;
            return;
        }
    } else if (dx != 0 || dy != 0) {
        if (interaction_try_bump(&g->world, dx, dy, &g->dialogue)) {
            ui_draw_dialogue(&g->dialogue);
            s_render_mode = RENDER_MODE_DIALOGUE;
            return;
        }
    }

    if (g->world.encounter_triggered) {
        state_set(&g->state_machine, GAME_STATE_BATTLE);
        battle_start(&g->battle, g->world.player.hp, g->world.player.max_hp, g->world.enemy.hp, g->world.enemy.max_hp);
        audio_play_music(MUSIC_BATTLE);
        telemetry_emit(EVENT_MUSIC_CHANGED, MUSIC_BATTLE, 0, 0, 0);
        ui_draw_battle_full(&g->battle);
        s_render_mode = RENDER_MODE_BATTLE;
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
            s_render_mode = RENDER_MODE_OVERWORLD;
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

void game_render(Game *g)
{
    (void)g;
}

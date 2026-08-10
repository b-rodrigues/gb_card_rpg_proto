#ifndef GAME_H
#define GAME_H

#include "state.h"
#include "world.h"
#include "battle.h"
#include "audio.h"
#include "input.h"
#include "ui.h"

#include "dialogue.h"

typedef struct {
    bool valid;
    GameState prev_state;
    MapId prev_map_id;
    uint8_t prev_player_x;
    uint8_t prev_player_y;
    bool prev_dialogue_active;
    uint8_t prev_dialogue_line;
    DialogueId prev_dialogue_id;
    BattleTurn prev_battle_turn;
    uint8_t prev_player_hp;
    uint8_t prev_enemy_hp;
    BattleResult prev_battle_result;
} RenderCache;

typedef struct {
    GameStateMachine state_machine;
    World world;
    Battle battle;
    DialogueState dialogue;
    uint32_t story_flags;
    uint32_t frame;
    RenderCache render_cache;
} Game;

extern Game g_game;

void game_init(Game *g);
void game_update(Game *g);
void game_render(Game *g);
void game_render_reset(Game *g);

#endif /* GAME_H */

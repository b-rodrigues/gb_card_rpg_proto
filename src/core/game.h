#ifndef GAME_H
#define GAME_H

#include "state.h"
#include "world.h"
#include "battle.h"
#include "audio.h"
#include "input.h"
#include "ui.h"

#define STORY_FLAG_ARRIVED_TOWN (1UL << 0)
#define STORY_FLAG_MET_MAYOR    (1UL << 1)

typedef struct {
    GameStateMachine state_machine;
    World world;
    Battle battle;
    uint32_t story_flags;
    uint32_t frame;
} Game;

extern Game g_game;

void game_init(Game *g);
void game_update(Game *g);
void game_render(const Game *g);

#endif /* GAME_H */

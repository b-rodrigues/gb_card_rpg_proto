#ifndef STATE_H
#define STATE_H

#include <stdbool.h>

typedef enum {
    GAME_STATE_OVERWORLD,
    GAME_STATE_BATTLE
} GameState;

typedef struct {
    GameState current;
    GameState previous;
    bool state_changed;
} GameStateMachine;

void state_init(GameStateMachine *sm);
void state_set(GameStateMachine *sm, GameState new_state);

#endif /* STATE_H */

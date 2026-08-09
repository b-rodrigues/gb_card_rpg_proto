#include "state.h"

void state_init(GameStateMachine *sm)
{
    if (!sm) return;
    sm->current = GAME_STATE_OVERWORLD;
    sm->previous = GAME_STATE_OVERWORLD;
    sm->state_changed = true;
}

void state_set(GameStateMachine *sm, GameState new_state)
{
    if (!sm) return;
    if (sm->current != new_state) {
        sm->previous = sm->current;
        sm->current = new_state;
        sm->state_changed = true;
    }
}

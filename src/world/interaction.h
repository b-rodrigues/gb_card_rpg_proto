#ifndef INTERACTION_H
#define INTERACTION_H

#include <stdint.h>
#include <stdbool.h>
#include "world.h"
#include "dialogue.h"
#include "actor.h"
#include "rpg/state.h"

ActorEngageResult interaction_try_at(Game *g, uint8_t target_x, uint8_t target_y);
ActorEngageResult interaction_try_facing(Game *g);
ActorEngageResult interaction_try_bump(Game *g, int8_t dx, int8_t dy);
void interaction_on_dialogue_end(DialogueState *dialogue, GameState *state);

#endif /* INTERACTION_H */

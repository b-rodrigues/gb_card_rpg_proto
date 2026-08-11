#ifndef INTERACTION_H
#define INTERACTION_H

#include <stdint.h>
#include <stdbool.h>
#include "world.h"
#include "dialogue.h"
#include "actor.h"

ActorEngageResult interaction_try_at(const World *world, uint8_t target_x, uint8_t target_y, DialogueState *dialogue);
ActorEngageResult interaction_try_facing(const World *world, DialogueState *dialogue);
ActorEngageResult interaction_try_bump(const World *world, int8_t dx, int8_t dy, DialogueState *dialogue);
void interaction_on_dialogue_end(DialogueState *dialogue, uint32_t *story_flags);

#endif /* INTERACTION_H */

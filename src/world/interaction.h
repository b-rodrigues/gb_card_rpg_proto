#ifndef INTERACTION_H
#define INTERACTION_H

#include <stdint.h>
#include <stdbool.h>
#include "world.h"
#include "dialogue.h"
#include "npc.h"

bool interaction_try_at(MapId map_id, uint8_t target_x, uint8_t target_y, DialogueState *dialogue);
void interaction_on_dialogue_end(DialogueState *dialogue, uint32_t *story_flags);

#endif /* INTERACTION_H */

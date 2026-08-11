#ifndef STORY_H
#define STORY_H

#include <stdint.h>
#include <stdbool.h>
#include "world.h"
#include "rpg/state.h"

typedef enum {
    STORY_FLAG_ID_ARRIVED_TOWN = 1,
    STORY_FLAG_ID_MET_MAYOR    = 2,
    STORY_FLAG_ID_COUNT        = 3
} StoryFlagId;

#define STORY_FLAG_ARRIVED_TOWN (1UL << (STORY_FLAG_ID_ARRIVED_TOWN - 1))
#define STORY_FLAG_MET_MAYOR    (1UL << (STORY_FLAG_ID_MET_MAYOR - 1))

bool story_flag_id_valid(StoryFlagId flag_id);
bool story_has_flag(const GameState *state, StoryFlagId flag_id);
void story_set_flag(GameState *state, StoryFlagId flag_id);
void story_clear_flag(GameState *state, StoryFlagId flag_id);

#endif /* STORY_H */

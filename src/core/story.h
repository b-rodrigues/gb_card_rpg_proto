#ifndef STORY_H
#define STORY_H

#include <stdint.h>
#include <stdbool.h>
#include "world.h"

typedef enum {
    STORY_FLAG_ID_ARRIVED_TOWN = 1,
    STORY_FLAG_ID_MET_MAYOR    = 2
} StoryFlagId;

#define STORY_FLAG_ARRIVED_TOWN (1UL << (STORY_FLAG_ID_ARRIVED_TOWN - 1))
#define STORY_FLAG_MET_MAYOR    (1UL << (STORY_FLAG_ID_MET_MAYOR - 1))

bool story_has_flag(uint32_t flags, StoryFlagId flag_id);
void story_set_flag(uint32_t *flags, StoryFlagId flag_id);
void story_clear_flag(uint32_t *flags, StoryFlagId flag_id);
void story_on_map_enter(uint32_t *flags, MapId to_map);

#endif /* STORY_H */

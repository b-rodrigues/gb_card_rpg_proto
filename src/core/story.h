#ifndef STORY_H
#define STORY_H

#include <stdint.h>
#include <stdbool.h>
#include "world.h"

#define STORY_FLAG_ARRIVED_TOWN (1UL << 0)
#define STORY_FLAG_MET_MAYOR    (1UL << 1)

bool story_has_flag(uint32_t flags, uint32_t flag);
void story_set_flag(uint32_t *flags, uint32_t flag);
void story_clear_flag(uint32_t *flags, uint32_t flag);
void story_on_map_enter(uint32_t *flags, MapId to_map);

#endif /* STORY_H */

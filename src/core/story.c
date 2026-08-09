#include "story.h"
#include "telemetry.h"

bool story_flag_id_valid(StoryFlagId flag_id)
{
    return (flag_id >= 1 && flag_id < STORY_FLAG_ID_COUNT);
}

bool story_has_flag(uint32_t flags, StoryFlagId flag_id)
{
    uint32_t mask;
    if (!story_flag_id_valid(flag_id)) return false;
    mask = 1UL << (flag_id - 1);
    return (flags & mask) != 0;
}

void story_set_flag(uint32_t *flags, StoryFlagId flag_id)
{
    uint32_t mask;
    if (!flags || !story_flag_id_valid(flag_id)) return;
    mask = 1UL << (flag_id - 1);
    if (!(*flags & mask)) {
        *flags |= mask;
        telemetry_emit(EVENT_STORY_FLAG_SET, (uint8_t)flag_id, 0, 0, 0);
    }
}

void story_clear_flag(uint32_t *flags, StoryFlagId flag_id)
{
    uint32_t mask;
    if (!flags || !story_flag_id_valid(flag_id)) return;
    mask = 1UL << (flag_id - 1);
    if (*flags & mask) {
        *flags &= ~mask;
        telemetry_emit(EVENT_STORY_FLAG_CLEARED, (uint8_t)flag_id, 0, 0, 0);
    }
}

void story_on_map_enter(uint32_t *flags, MapId to_map)
{
    if (!flags) return;
    if (to_map == MAP_TOWN && !story_has_flag(*flags, STORY_FLAG_ID_ARRIVED_TOWN)) {
        story_set_flag(flags, STORY_FLAG_ID_ARRIVED_TOWN);
    }
}

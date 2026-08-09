#include "story.h"
#include "telemetry.h"

bool story_has_flag(uint32_t flags, uint32_t flag)
{
    return (flags & flag) != 0;
}

void story_set_flag(uint32_t *flags, uint32_t flag)
{
    if (!flags) return;
    if (!story_has_flag(*flags, flag)) {
        *flags |= flag;
        telemetry_emit(EVENT_STORY_FLAG_SET, (uint8_t)(flag & 0xFF), (uint8_t)((flag >> 8) & 0xFF), 0, 0);
    }
}

void story_clear_flag(uint32_t *flags, uint32_t flag)
{
    if (!flags) return;
    *flags &= ~flag;
}

void story_on_map_enter(uint32_t *flags, MapId to_map)
{
    if (!flags) return;
    if (to_map == MAP_TOWN && !story_has_flag(*flags, STORY_FLAG_ARRIVED_TOWN)) {
        story_set_flag(flags, STORY_FLAG_ARRIVED_TOWN);
    }
}

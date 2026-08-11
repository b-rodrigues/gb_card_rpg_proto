#include "story.h"
#include "telemetry.h"

bool story_flag_id_valid(StoryFlagId flag_id)
{
    return (flag_id >= 1 && flag_id < STORY_FLAG_ID_COUNT);
}

bool story_has_flag(const GameState *state, StoryFlagId flag_id)
{
    if (!story_flag_id_valid(flag_id)) return false;
    return game_flag_is_set(state, (FlagId)flag_id);
}

void story_set_flag(GameState *state, StoryFlagId flag_id)
{
    if (!story_flag_id_valid(flag_id)) return;
    game_flag_set(state, (FlagId)flag_id);
}

void story_clear_flag(GameState *state, StoryFlagId flag_id)
{
    if (!story_flag_id_valid(flag_id)) return;
    game_flag_clear(state, (FlagId)flag_id);
}

void story_on_map_enter(GameState *state, MapId to_map)
{
    if (!state) return;
    if (to_map == MAP_TOWN && !story_has_flag(state, STORY_FLAG_ID_ARRIVED_TOWN)) {
        story_set_flag(state, STORY_FLAG_ID_ARRIVED_TOWN);
    }
}

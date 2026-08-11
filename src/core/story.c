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

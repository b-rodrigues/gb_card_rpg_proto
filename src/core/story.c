#include "story.h"
#include "telemetry.h"

static uint8_t g_story_flag_max = 0;

void story_init(uint8_t flag_count)
{
    g_story_flag_max = flag_count;
}

static bool story_flag_id_valid(FlagId flag_id)
{
    return (flag_id >= 1 && flag_id < g_story_flag_max);
}

bool story_has_flag(const GameState *state, FlagId flag_id)
{
    if (!story_flag_id_valid(flag_id)) return false;
    return game_flag_is_set(state, flag_id);
}

void story_set_flag(GameState *state, FlagId flag_id)
{
    if (!story_flag_id_valid(flag_id)) return;
    game_flag_set(state, flag_id);
}

void story_clear_flag(GameState *state, FlagId flag_id)
{
    if (!story_flag_id_valid(flag_id)) return;
    game_flag_clear(state, flag_id);
}

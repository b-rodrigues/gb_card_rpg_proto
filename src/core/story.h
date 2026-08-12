#ifndef STORY_H
#define STORY_H

#include <stdint.h>
#include <stdbool.h>
#include "rpg/state.h"

/* Story flags are a named sub-set of GameState.flags (FlagId).  The engine
 * is generic: the exclusive upper bound on valid flag ids is provided by the
 * game layer via story_init() (e.g. STORY_FLAG_ID_COUNT in game_ids.h). */

void story_init(uint8_t flag_count);

bool story_has_flag(const GameState *state, FlagId flag_id);
void story_set_flag(GameState *state, FlagId flag_id);
void story_clear_flag(GameState *state, FlagId flag_id);

#endif /* STORY_H */

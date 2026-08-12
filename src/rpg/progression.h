#ifndef RPG_PROGRESSION_H
#define RPG_PROGRESSION_H

#include <stdint.h>
#include <stdbool.h>
#include "rpg/state.h"

/* Generic progression mechanic.  A progression target is anything in the
 * game that can progress (hero, weapon, card, companion, ...).  The engine
 * does NOT know what a target type means; it only maps a target to a static
 * ProgressionDefinition (max level + thresholds) and mutates its
 * ProgressionState.  Consequences of a level-up are handled by the caller
 * (the game-specific layer), not by this engine. */

typedef struct {
    uint8_t max_level;
    const uint16_t *thresholds;  /* thresholds[level-1] = progress needed to reach level+1 */
    uint8_t threshold_count;
} ProgressionDefinition;

typedef struct {
    uint8_t level_before;
    uint8_t level_after;
    bool crossed;                /* a level was gained */
} ProgressionAddResult;

const ProgressionDefinition *progression_get_def(uint8_t target_type);

/* Return the state of a target, or NULL if it has no progression entry. */
ProgressionState *progression_get(GameState *state, ProgressionTarget target);

/* Ensure a target exists with the given level/progress.  Used by scenario
 * injection; emits no telemetry and never applies consequences. */
bool progression_ensure(GameState *state, ProgressionTarget target,
                        uint8_t level, uint16_t progress);

/* Add progress to a target, crossing thresholds and emitting
 * PROGRESSION_GAINED + LEVEL_UP.  The caller decides what the level-up
 * means for the game. */
ProgressionAddResult progression_add(GameState *state, ProgressionTarget target,
                                     uint16_t amount);

#endif /* RPG_PROGRESSION_H */

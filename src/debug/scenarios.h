#ifndef SCENARIOS_H
#define SCENARIOS_H

#include "game.h"

typedef enum {
    SCENARIO_NEW_GAME,
    SCENARIO_FIRST_ENCOUNTER
} ScenarioId;

void scenario_load(ScenarioId id, Game *g);

#endif /* SCENARIOS_H */

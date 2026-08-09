#ifndef SCENARIOS_H
#define SCENARIOS_H

#include <stdint.h>
#include "game.h"

extern volatile uint8_t g_scen_load;

void scenario_load(void);
void scenario_check_and_load(void);

#endif /* SCENARIOS_H */

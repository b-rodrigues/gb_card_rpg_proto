#ifndef SCENARIOS_H
#define SCENARIOS_H

#include <stdint.h>
#include "game.h"

extern volatile uint8_t g_scen_load;
extern volatile uint8_t g_scen_load_state;
extern volatile uint8_t g_debug_action[6];
extern volatile uint8_t g_debug_action_pending;

void scenario_check_and_load(void);

#endif /* SCENARIOS_H */

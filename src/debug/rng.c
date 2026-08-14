#include "rng.h"

static uint16_t rng_state = 1;

void rng_set_seed(uint16_t seed)
{
    rng_state = seed ? seed : 1;
}

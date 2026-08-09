#include "rng.h"

static uint16_t rng_state = 1;

void rng_init(uint16_t seed)
{
    rng_state = seed ? seed : 1;
}

uint16_t rng_next(void)
{
    rng_state ^= rng_state << 7;
    rng_state ^= rng_state >> 9;
    rng_state ^= rng_state << 8;
    return rng_state;
}

uint16_t rng_get_seed(void)
{
    return rng_state;
}

void rng_set_seed(uint16_t seed)
{
    rng_state = seed ? seed : 1;
}

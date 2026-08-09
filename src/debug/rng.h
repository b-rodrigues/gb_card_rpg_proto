#ifndef RNG_H
#define RNG_H

#include <stdint.h>

void rng_init(uint16_t seed);
uint16_t rng_next(void);
uint16_t rng_get_seed(void);
void rng_set_seed(uint16_t seed);

#endif /* RNG_H */

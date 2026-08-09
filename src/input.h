#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>

void input_init(void);
void input_update(void);
uint8_t input_is_down(uint8_t button);
uint8_t input_is_pressed(uint8_t button);

#endif /* INPUT_H */

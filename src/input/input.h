#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    INPUT_RIGHT,
    INPUT_LEFT,
    INPUT_UP,
    INPUT_DOWN,
    INPUT_A,
    INPUT_B,
    INPUT_START,
    INPUT_SELECT
} InputButton;

extern volatile uint8_t g_inp_mask;

void input_init(void);
void input_reset(void);
void input_update(void);
bool input_pressed(InputButton button);
bool input_held(InputButton button);
void input_inject_press(InputButton button);

#endif /* INPUT_H */

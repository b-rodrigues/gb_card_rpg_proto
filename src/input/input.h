#ifndef INPUT_H
#define INPUT_H

#include <gb/gb.h>
#include <stdbool.h>

typedef enum {
    INPUT_UP,
    INPUT_DOWN,
    INPUT_LEFT,
    INPUT_RIGHT,
    INPUT_A,
    INPUT_B,
    INPUT_START,
    INPUT_SELECT
} InputButton;

void input_init(void);
void input_update(void);
bool input_pressed(InputButton button);
bool input_held(InputButton button);

#endif /* INPUT_H */

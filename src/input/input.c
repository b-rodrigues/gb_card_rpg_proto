#include "input.h"

static uint8_t pad_state = 0;
static uint8_t prev_pad_state = 0;

static uint8_t get_button_mask(InputButton button)
{
    switch (button) {
        case INPUT_UP: return J_UP;
        case INPUT_DOWN: return J_DOWN;
        case INPUT_LEFT: return J_LEFT;
        case INPUT_RIGHT: return J_RIGHT;
        case INPUT_A: return J_A;
        case INPUT_B: return J_B;
        case INPUT_START: return J_START;
        case INPUT_SELECT: return J_SELECT;
        default: return 0;
    }
}

void input_init(void)
{
    pad_state = joypad();
    prev_pad_state = pad_state;
}

void input_update(void)
{
    prev_pad_state = pad_state;
    pad_state = joypad();
}

bool input_pressed(InputButton button)
{
    uint8_t mask = get_button_mask(button);
    return ((pad_state & mask) != 0) && ((prev_pad_state & mask) == 0);
}

bool input_held(InputButton button)
{
    uint8_t mask = get_button_mask(button);
    return (pad_state & mask) != 0;
}

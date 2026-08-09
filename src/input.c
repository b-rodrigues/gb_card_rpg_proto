#include "input.h"
#include <gb/gb.h>

static uint8_t pad_state;
static uint8_t prev_pad_state;

void input_init(void) {
    pad_state = joypad();
    prev_pad_state = pad_state;
}

void input_update(void) {
    prev_pad_state = pad_state;
    pad_state = joypad();
}

uint8_t input_is_down(uint8_t button) {
    return (pad_state & button) != 0;
}

uint8_t input_is_pressed(uint8_t button) {
    return (pad_state & button) != 0 && (prev_pad_state & button) == 0;
}

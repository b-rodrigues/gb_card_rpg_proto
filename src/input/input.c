#include <gb/gb.h>
#include "input.h"

volatile uint8_t g_inp_mask = 0;
static uint8_t pad_state = 0;
static uint8_t prev_pad_state = 0;
static uint8_t injected_pad_state = 0;

void input_init(void)
{
    pad_state = joypad();
    prev_pad_state = pad_state;
    injected_pad_state = 0;
    g_inp_mask = 0;
}

void input_reset(void)
{
    pad_state = 0;
    prev_pad_state = 0;
    injected_pad_state = 0;
    g_inp_mask = 0;
}

void input_update(void)
{
    prev_pad_state = pad_state;
#ifdef DEBUG_BUILD
    if (g_inp_mask != 0) {
        injected_pad_state = g_inp_mask;
        g_inp_mask = 0;
    }
    if (injected_pad_state != 0) {
        pad_state = injected_pad_state;
        injected_pad_state = 0;
        return;
    }
    pad_state = 0;
#else
    pad_state = joypad() | injected_pad_state;
    injected_pad_state = 0;
    g_inp_mask = 0;
#endif
}

bool input_pressed(InputButton button)
{
    uint8_t mask = (1 << button);
    return ((pad_state & mask) != 0) && ((prev_pad_state & mask) == 0);
}

bool input_held(InputButton button)
{
    uint8_t mask = (1 << button);
    return (pad_state & mask) != 0;
}

void input_inject_press(InputButton button)
{
    injected_pad_state |= (1 << button);
}

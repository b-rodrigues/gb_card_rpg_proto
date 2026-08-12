#include <gb/gb.h>
#include "input.h"

/* InputButton bits MUST equal GBDK joypad()'s J_* bits, otherwise the real
 * game's controls diverge from what the harness tests (g_inp_mask uses
 * 1 << InputButton while release input uses joypad()'s layout).  A mismatch
 * makes this array size 0, which is a hard compile error. */
static const int g_input_bit_layout_ok[
    (1 << INPUT_RIGHT) == J_RIGHT &&
    (1 << INPUT_LEFT)  == J_LEFT  &&
    (1 << INPUT_UP)    == J_UP    &&
    (1 << INPUT_DOWN)  == J_DOWN  &&
    (1 << INPUT_A)     == J_A     &&
    (1 << INPUT_B)     == J_B     &&
    (1 << INPUT_SELECT)== J_SELECT &&
    (1 << INPUT_START) == J_START ? 1 : 0
];

volatile uint8_t g_inp_mask = 0;
static uint8_t pad_state = 0;
static uint8_t prev_pad_state = 0;
static uint8_t injected_pad_state = 0;

const uint8_t g_input_button_bits[8] = {
    (uint8_t)(1 << INPUT_RIGHT),
    (uint8_t)(1 << INPUT_LEFT),
    (uint8_t)(1 << INPUT_UP),
    (uint8_t)(1 << INPUT_DOWN),
    (uint8_t)(1 << INPUT_A),
    (uint8_t)(1 << INPUT_B),
    (uint8_t)(1 << INPUT_SELECT),
    (uint8_t)(1 << INPUT_START)
};

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

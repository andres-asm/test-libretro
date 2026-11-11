#ifndef TEST_WIZARD_INPUT_H
#define TEST_WIZARD_INPUT_H

/* Reads port 0 (joypad, analog sticks, pointer) once per frame. */

#include <stdbool.h>
#include <stdint.h>

#include "libretro.h"

/* How long Start/Select must be held to pass/fail a page (frames at 60 fps). */
#define INPUT_HOLD_FRAMES 60

typedef struct InputState
{
    uint16_t buttons;            /* bit N = RETRO_DEVICE_ID_JOYPAD_N, asked one button at a time */
    uint16_t buttons_previous;
    uint16_t buttons_bitmask;    /* same, from one RETRO_DEVICE_ID_JOYPAD_MASK query */
    bool     bitmask_supported;  /* frontend answered GET_INPUT_BITMASKS = true */
    bool     bitmask_mismatch;   /* the two answers disagreed at least once */

    int16_t  analog[2][2];       /* [left/right stick][x/y] */

    bool     pointer_valid;      /* frontend reported a pointer position at all */
    int      pointer_x;          /* in framebuffer pixels */
    int      pointer_y;
    bool     pointer_pressed;

    unsigned start_held_frames;
    unsigned select_held_frames;
} InputState;

void input_update(InputState* input, retro_input_poll_t poll, retro_input_state_t state,
                  unsigned framebuffer_width, unsigned framebuffer_height);

bool input_is_down(const InputState* input, unsigned button_id);
bool input_was_pressed(const InputState* input, unsigned button_id);   /* down now, up last frame */

const char* input_button_name(unsigned button_id);

#endif

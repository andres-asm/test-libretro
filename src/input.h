#ifndef TEST_LIBRETRO_INPUT_H
#define TEST_LIBRETRO_INPUT_H

/*
 * Controller 1's buttons (to move between pages), the pointer and the mouse,
 * once per frame. Every port's pads are read by controllers.h, the keyboard
 * by keyboard.h.
 */

#include <stdbool.h>
#include <stdint.h>

#include "libretro.h"

#ifdef __cplusplus
extern "C" {
#endif

/* How long Start/Select must be held to pass/fail a page (frames at 60 fps). */
#define INPUT_HOLD_FRAMES 60
/* How long L/R must be held to go to the previous / next page. */
#define INPUT_PAGE_HOLD_FRAMES 30

/* The mouse buttons, as bits of InputState::mouse_buttons. */
enum
{
    MOUSE_BUTTON_LEFT   = 1 << 0,
    MOUSE_BUTTON_RIGHT  = 1 << 1,
    MOUSE_BUTTON_MIDDLE = 1 << 2,
    MOUSE_BUTTON_4      = 1 << 3,
    MOUSE_BUTTON_5      = 1 << 4
};

typedef struct InputState
{
    uint16_t buttons;            /* controller 1: bit N = RETRO_DEVICE_ID_JOYPAD_N */
    uint16_t buttons_previous;

    bool     pointer_valid;      /* frontend reported a pointer position at all */
    int      pointer_x;          /* in framebuffer pixels */
    int      pointer_y;
    bool     pointer_pressed;

    bool     mouse_seen;         /* the mouse moved or a button was pressed at least once */
    int      mouse_x;            /* its relative moves added up, kept inside the framebuffer */
    int      mouse_y;
    int      mouse_dx;           /* this frame's move */
    int      mouse_dy;
    unsigned mouse_buttons;      /* MOUSE_BUTTON_* held */
    unsigned wheel_up;           /* wheel clicks counted */
    unsigned wheel_down;

    unsigned start_held_frames;
    unsigned select_held_frames;
    unsigned l_held_frames;
    unsigned r_held_frames;
} InputState;

void input_update(InputState* input, retro_input_poll_t poll, retro_input_state_t state,
                  unsigned framebuffer_width, unsigned framebuffer_height);

bool input_is_down(const InputState* input, unsigned button_id);
bool input_was_pressed(const InputState* input, unsigned button_id);   /* down now, up last frame */

#ifdef __cplusplus
}
#endif

#endif

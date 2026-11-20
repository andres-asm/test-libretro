#include "input.h"

/* Pointer coordinates are -0x7fff..0x7fff across the visible image. */
static int pointer_to_pixels(int16_t value, unsigned size)
{
    long position = ((long)value + 0x7fff) * (long)size / 0xfffe;
    if (position < 0)
    {
        position = 0;
    }
    if (position >= (long)size)
    {
        position = (long)size - 1;
    }
    return (int)position;
}

static int clamp_int(int value, int low, int high)
{
    return value < low ? low : (value > high ? high : value);
}

static unsigned held_frames(const InputState* input, unsigned button_id, unsigned frames)
{
    return input_is_down(input, button_id) ? frames + 1 : 0;
}

static void read_mouse(InputState* input, retro_input_state_t state, unsigned width, unsigned height)
{
    static const unsigned BUTTON_IDS[5] = {
        RETRO_DEVICE_ID_MOUSE_LEFT, RETRO_DEVICE_ID_MOUSE_RIGHT, RETRO_DEVICE_ID_MOUSE_MIDDLE,
        RETRO_DEVICE_ID_MOUSE_BUTTON_4, RETRO_DEVICE_ID_MOUSE_BUTTON_5,
    };
    unsigned b = 0;

    input->mouse_dx = state(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
    input->mouse_dy = state(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y);
    input->mouse_buttons = 0;
    for (b = 0; b < 5; b++)
    {
        if (state(0, RETRO_DEVICE_MOUSE, 0, BUTTON_IDS[b]) != 0)
        {
            input->mouse_buttons |= 1u << b;
        }
    }
    if (state(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_WHEELUP) != 0)
    {
        input->wheel_up++;
    }
    if (state(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_WHEELDOWN) != 0)
    {
        input->wheel_down++;
    }
    if (input->mouse_dx != 0 || input->mouse_dy != 0 || input->mouse_buttons != 0)
    {
        if (!input->mouse_seen)
        {
            input->mouse_x = (int)width / 2;   /* starts in the middle */
            input->mouse_y = (int)height / 2;
        }
        input->mouse_seen = true;
    }
    input->mouse_x = clamp_int(input->mouse_x + input->mouse_dx, 0, (int)width - 1);
    input->mouse_y = clamp_int(input->mouse_y + input->mouse_dy, 0, (int)height - 1);
}

void input_update(InputState* input, retro_input_poll_t poll, retro_input_state_t state,
                  unsigned framebuffer_width, unsigned framebuffer_height)
{
    unsigned id = 0;
    int16_t pointer_x = 0;
    int16_t pointer_y = 0;

    poll();

    input->buttons_previous = input->buttons;
    input->buttons = 0;
    for (id = 0; id < 16; id++)
    {
        if (state(0, RETRO_DEVICE_JOYPAD, 0, id) != 0)
        {
            input->buttons = (uint16_t)(input->buttons | (1u << id));
        }
    }

    pointer_x = state(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_X);
    pointer_y = state(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_Y);
    input->pointer_pressed = state(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_PRESSED) != 0;
    if (pointer_x != 0 || pointer_y != 0 || input->pointer_pressed)
    {
        input->pointer_valid = true;
    }
    input->pointer_x = pointer_to_pixels(pointer_x, framebuffer_width);
    input->pointer_y = pointer_to_pixels(pointer_y, framebuffer_height);

    read_mouse(input, state, framebuffer_width, framebuffer_height);

    input->start_held_frames  = held_frames(input, RETRO_DEVICE_ID_JOYPAD_START, input->start_held_frames);
    input->select_held_frames = held_frames(input, RETRO_DEVICE_ID_JOYPAD_SELECT, input->select_held_frames);
    input->l_held_frames      = held_frames(input, RETRO_DEVICE_ID_JOYPAD_L, input->l_held_frames);
    input->r_held_frames      = held_frames(input, RETRO_DEVICE_ID_JOYPAD_R, input->r_held_frames);
    input->x_held_frames      = held_frames(input, RETRO_DEVICE_ID_JOYPAD_X, input->x_held_frames);
    input->y_held_frames      = held_frames(input, RETRO_DEVICE_ID_JOYPAD_Y, input->y_held_frames);
}

bool input_is_down(const InputState* input, unsigned button_id)
{
    return (input->buttons & (1u << button_id)) != 0;
}

bool input_was_pressed(const InputState* input, unsigned button_id)
{
    bool down_now = (input->buttons & (1u << button_id)) != 0;
    bool down_before = (input->buttons_previous & (1u << button_id)) != 0;
    return down_now && !down_before;
}

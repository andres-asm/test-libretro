#include "input.h"

static const char* const BUTTON_NAMES[16] = {
    "B", "Y", "Select", "Start", "Up", "Down", "Left", "Right",
    "A", "X", "L", "R", "L2", "R2", "L3", "R3",
};

const char* input_button_name(unsigned button_id)
{
    if (button_id < 16)
    {
        return BUTTON_NAMES[button_id];
    }
    return "?";
}

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

    /* Both ways of reading buttons must agree; a frontend that gets one wrong is caught here. */
    if (input->bitmask_supported)
    {
        input->buttons_bitmask = (uint16_t)state(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_MASK);
        if (input->buttons_bitmask != input->buttons)
        {
            input->bitmask_mismatch = true;
        }
    }

    input->analog[0][0] = state(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT,  RETRO_DEVICE_ID_ANALOG_X);
    input->analog[0][1] = state(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT,  RETRO_DEVICE_ID_ANALOG_Y);
    input->analog[1][0] = state(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X);
    input->analog[1][1] = state(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y);

    pointer_x = state(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_X);
    pointer_y = state(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_Y);
    input->pointer_pressed = state(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_PRESSED) != 0;
    if (pointer_x != 0 || pointer_y != 0 || input->pointer_pressed)
    {
        input->pointer_valid = true;
    }
    input->pointer_x = pointer_to_pixels(pointer_x, framebuffer_width);
    input->pointer_y = pointer_to_pixels(pointer_y, framebuffer_height);

    input->start_held_frames  = input_is_down(input, RETRO_DEVICE_ID_JOYPAD_START)  ? input->start_held_frames + 1  : 0;
    input->select_held_frames = input_is_down(input, RETRO_DEVICE_ID_JOYPAD_SELECT) ? input->select_held_frames + 1 : 0;
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

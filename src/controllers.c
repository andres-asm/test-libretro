#include "controllers.h"

#include <stddef.h>

static const ControllerButton RETROPAD_BUTTONS[] = {
    {RETRO_DEVICE_ID_JOYPAD_UP, "Up"},       {RETRO_DEVICE_ID_JOYPAD_DOWN, "Down"},
    {RETRO_DEVICE_ID_JOYPAD_LEFT, "Left"},   {RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right"},
    {RETRO_DEVICE_ID_JOYPAD_B, "B"},         {RETRO_DEVICE_ID_JOYPAD_A, "A"},
    {RETRO_DEVICE_ID_JOYPAD_Y, "Y"},         {RETRO_DEVICE_ID_JOYPAD_X, "X"},
    {RETRO_DEVICE_ID_JOYPAD_L, "L"},         {RETRO_DEVICE_ID_JOYPAD_R, "R"},
    {RETRO_DEVICE_ID_JOYPAD_L2, "L2"},       {RETRO_DEVICE_ID_JOYPAD_R2, "R2"},
    {RETRO_DEVICE_ID_JOYPAD_L3, "L3"},       {RETRO_DEVICE_ID_JOYPAD_R3, "R3"},
    {RETRO_DEVICE_ID_JOYPAD_SELECT, "Select"}, {RETRO_DEVICE_ID_JOYPAD_START, "Start"},
};

static const ControllerButton TWO_BUTTONS[] = {
    {RETRO_DEVICE_ID_JOYPAD_UP, "Up"},     {RETRO_DEVICE_ID_JOYPAD_DOWN, "Down"},
    {RETRO_DEVICE_ID_JOYPAD_LEFT, "Left"}, {RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right"},
    {RETRO_DEVICE_ID_JOYPAD_B, "Fire"},    {RETRO_DEVICE_ID_JOYPAD_A, "Jump"},
    {RETRO_DEVICE_ID_JOYPAD_START, "Pause"},
};

static const ControllerButton SIX_BUTTONS[] = {
    {RETRO_DEVICE_ID_JOYPAD_UP, "Up"},     {RETRO_DEVICE_ID_JOYPAD_DOWN, "Down"},
    {RETRO_DEVICE_ID_JOYPAD_LEFT, "Left"}, {RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right"},
    {RETRO_DEVICE_ID_JOYPAD_Y, "A"},       {RETRO_DEVICE_ID_JOYPAD_B, "B"},
    {RETRO_DEVICE_ID_JOYPAD_A, "C"},       {RETRO_DEVICE_ID_JOYPAD_L, "X"},
    {RETRO_DEVICE_ID_JOYPAD_X, "Y"},       {RETRO_DEVICE_ID_JOYPAD_R, "Z"},
    {RETRO_DEVICE_ID_JOYPAD_START, "Start"}, {RETRO_DEVICE_ID_JOYPAD_SELECT, "Mode"},
};

#define COUNT_OF(array) (sizeof(array) / sizeof((array)[0]))

static const struct retro_controller_description TYPES[] = {
    {"RetroPad", RETRO_DEVICE_JOYPAD},
    {"Two-button pad", DEVICE_TWO_BUTTON},
    {"Six-button pad", DEVICE_SIX_BUTTON},
};

const char* controllers_device_name(unsigned device)
{
    unsigned t = 0;
    for (t = 0; t < COUNT_OF(TYPES); t++)
    {
        if (TYPES[t].id == device)
        {
            return TYPES[t].desc;
        }
    }
    return device == RETRO_DEVICE_NONE ? "None" : "Unknown";
}

unsigned controllers_buttons(unsigned device, const ControllerButton** buttons)
{
    if (device == DEVICE_TWO_BUTTON)
    {
        *buttons = TWO_BUTTONS;
        return (unsigned)COUNT_OF(TWO_BUTTONS);
    }
    if (device == DEVICE_SIX_BUTTON)
    {
        *buttons = SIX_BUTTONS;
        return (unsigned)COUNT_OF(SIX_BUTTONS);
    }
    if (device == RETRO_DEVICE_JOYPAD)
    {
        *buttons = RETROPAD_BUTTONS;
        return (unsigned)COUNT_OF(RETROPAD_BUTTONS);
    }
    *buttons = NULL;
    return 0;
}

/* Every port's buttons, by its type's names, in one list ending in a NULL description. */
static void send_descriptors(retro_environment_t environment, ControllersState* state)
{
    struct retro_input_descriptor descriptors[CONTROLLER_PORTS * 16 + 1];
    unsigned count = 0;
    unsigned port = 0;
    for (port = 0; port < CONTROLLER_PORTS; port++)
    {
        const ControllerButton* buttons = NULL;
        unsigned button_count = controllers_buttons(state->device[port], &buttons);
        unsigned b = 0;
        for (b = 0; b < button_count; b++)
        {
            descriptors[count].port = port;
            descriptors[count].device = RETRO_DEVICE_JOYPAD;
            descriptors[count].index = 0;
            descriptors[count].id = buttons[b].id;
            descriptors[count].description = buttons[b].name;
            count++;
        }
    }
    descriptors[count].port = 0;
    descriptors[count].device = 0;
    descriptors[count].index = 0;
    descriptors[count].id = 0;
    descriptors[count].description = NULL;
    state->descriptors_accepted = environment(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, descriptors);
    state->descriptor_sends++;
}

void controllers_register(retro_environment_t environment, ControllersState* state)
{
    struct retro_controller_info info[CONTROLLER_PORTS + 1];
    unsigned port = 0;
    for (port = 0; port < CONTROLLER_PORTS; port++)
    {
        info[port].types = TYPES;
        info[port].num_types = (unsigned)COUNT_OF(TYPES);
        state->device[port] = RETRO_DEVICE_JOYPAD;
        state->unknown_device[port] = false;
    }
    info[CONTROLLER_PORTS].types = NULL;   /* the list ends with an empty entry */
    info[CONTROLLER_PORTS].num_types = 0;
    state->info_accepted = environment(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, info);
    send_descriptors(environment, state);
}

void controllers_set_device(retro_environment_t environment, ControllersState* state, unsigned port, unsigned device)
{
    const ControllerButton* buttons = NULL;
    if (port >= CONTROLLER_PORTS)
    {
        return;
    }
    state->device[port] = device;
    state->unknown_device[port] = device != RETRO_DEVICE_NONE && controllers_buttons(device, &buttons) == 0;
    state->device_changes++;
    if (environment != NULL)
    {
        send_descriptors(environment, state);
    }
}

void controllers_read(ControllersState* state, retro_input_state_t input_state)
{
    unsigned port = 0;
    for (port = 0; port < CONTROLLER_PORTS; port++)
    {
        const ControllerButton* buttons = NULL;
        unsigned count = controllers_buttons(state->device[port], &buttons);
        unsigned b = 0;
        state->buttons[port] = 0;
        for (b = 0; b < count; b++)
        {
            if (input_state(port, RETRO_DEVICE_JOYPAD, 0, buttons[b].id) != 0)
            {
                state->buttons[port] = (uint16_t)(state->buttons[port] | (1u << buttons[b].id));
            }
        }
    }
}

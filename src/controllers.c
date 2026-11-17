#include "controllers.h"

#include <stddef.h>

static const ControllerButton RETROPAD_BUTTONS[] = {
    {RETRO_DEVICE_ID_JOYPAD_UP, "D-Pad Up"},       {RETRO_DEVICE_ID_JOYPAD_DOWN, "D-Pad Down"},
    {RETRO_DEVICE_ID_JOYPAD_LEFT, "D-Pad Left"},   {RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right"},
    {RETRO_DEVICE_ID_JOYPAD_B, "B"},               {RETRO_DEVICE_ID_JOYPAD_A, "A"},
    {RETRO_DEVICE_ID_JOYPAD_Y, "Y"},               {RETRO_DEVICE_ID_JOYPAD_X, "X"},
    {RETRO_DEVICE_ID_JOYPAD_L, "L"},               {RETRO_DEVICE_ID_JOYPAD_R, "R"},
    {RETRO_DEVICE_ID_JOYPAD_L2, "L2"},             {RETRO_DEVICE_ID_JOYPAD_R2, "R2"},
    {RETRO_DEVICE_ID_JOYPAD_L3, "L3"},             {RETRO_DEVICE_ID_JOYPAD_R3, "R3"},
    {RETRO_DEVICE_ID_JOYPAD_SELECT, "Select"},     {RETRO_DEVICE_ID_JOYPAD_START, "Start"},
};

static const ControllerButton NES_BUTTONS[] = {
    {RETRO_DEVICE_ID_JOYPAD_UP, "D-Pad Up"},       {RETRO_DEVICE_ID_JOYPAD_DOWN, "D-Pad Down"},
    {RETRO_DEVICE_ID_JOYPAD_LEFT, "D-Pad Left"},   {RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right"},
    {RETRO_DEVICE_ID_JOYPAD_B, "B"},               {RETRO_DEVICE_ID_JOYPAD_A, "A"},
    {RETRO_DEVICE_ID_JOYPAD_SELECT, "Select"},     {RETRO_DEVICE_ID_JOYPAD_START, "Start"},
};

static const ControllerButton SNES_BUTTONS[] = {
    {RETRO_DEVICE_ID_JOYPAD_UP, "D-Pad Up"},       {RETRO_DEVICE_ID_JOYPAD_DOWN, "D-Pad Down"},
    {RETRO_DEVICE_ID_JOYPAD_LEFT, "D-Pad Left"},   {RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right"},
    {RETRO_DEVICE_ID_JOYPAD_B, "B"},               {RETRO_DEVICE_ID_JOYPAD_A, "A"},
    {RETRO_DEVICE_ID_JOYPAD_Y, "Y"},               {RETRO_DEVICE_ID_JOYPAD_X, "X"},
    {RETRO_DEVICE_ID_JOYPAD_L, "L"},               {RETRO_DEVICE_ID_JOYPAD_R, "R"},
    {RETRO_DEVICE_ID_JOYPAD_SELECT, "Select"},     {RETRO_DEVICE_ID_JOYPAD_START, "Start"},
};

/* PlayStation names on the RetroPad's positions: Cross is the bottom button (B). */
static const ControllerButton DUALSHOCK_BUTTONS[] = {
    {RETRO_DEVICE_ID_JOYPAD_UP, "D-Pad Up"},       {RETRO_DEVICE_ID_JOYPAD_DOWN, "D-Pad Down"},
    {RETRO_DEVICE_ID_JOYPAD_LEFT, "D-Pad Left"},   {RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right"},
    {RETRO_DEVICE_ID_JOYPAD_B, "Cross"},           {RETRO_DEVICE_ID_JOYPAD_A, "Circle"},
    {RETRO_DEVICE_ID_JOYPAD_Y, "Square"},          {RETRO_DEVICE_ID_JOYPAD_X, "Triangle"},
    {RETRO_DEVICE_ID_JOYPAD_L, "L1"},              {RETRO_DEVICE_ID_JOYPAD_R, "R1"},
    {RETRO_DEVICE_ID_JOYPAD_L2, "L2"},             {RETRO_DEVICE_ID_JOYPAD_R2, "R2"},
    {RETRO_DEVICE_ID_JOYPAD_L3, "L3"},             {RETRO_DEVICE_ID_JOYPAD_R3, "R3"},
    {RETRO_DEVICE_ID_JOYPAD_SELECT, "Select"},     {RETRO_DEVICE_ID_JOYPAD_START, "Start"},
};

#define COUNT_OF(array) (sizeof(array) / sizeof((array)[0]))

static const struct retro_controller_description TYPES[] = {
    {"RetroPad", RETRO_DEVICE_JOYPAD},
    {"NES style gamepad", DEVICE_NES},
    {"SNES style gamepad", DEVICE_SNES},
    {"PSX DualShock style gamepad", DEVICE_DUALSHOCK},
};

/* The sticks' names: one per stick, for both of its axes. */
static const char* const STICK_NAMES[2] = {"Left Stick", "Right Stick"};

unsigned controllers_type_count(void)
{
    return (unsigned)COUNT_OF(TYPES);
}

unsigned controllers_type_device(unsigned index)
{
    return index < COUNT_OF(TYPES) ? TYPES[index].id : RETRO_DEVICE_JOYPAD;
}

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

bool controllers_has_sticks(unsigned device)
{
    return device == RETRO_DEVICE_JOYPAD || device == DEVICE_DUALSHOCK;
}

unsigned controllers_buttons(unsigned device, const ControllerButton** buttons)
{
    switch (device)
    {
        case RETRO_DEVICE_JOYPAD:
            *buttons = RETROPAD_BUTTONS;
            return (unsigned)COUNT_OF(RETROPAD_BUTTONS);
        case DEVICE_NES:
            *buttons = NES_BUTTONS;
            return (unsigned)COUNT_OF(NES_BUTTONS);
        case DEVICE_SNES:
            *buttons = SNES_BUTTONS;
            return (unsigned)COUNT_OF(SNES_BUTTONS);
        case DEVICE_DUALSHOCK:
            *buttons = DUALSHOCK_BUTTONS;
            return (unsigned)COUNT_OF(DUALSHOCK_BUTTONS);
        default:
            *buttons = NULL;
            return 0;
    }
}

uint16_t controllers_button_mask(unsigned device)
{
    const ControllerButton* buttons = NULL;
    unsigned count = controllers_buttons(device, &buttons);
    unsigned b = 0;
    uint16_t mask = 0;
    for (b = 0; b < count; b++)
    {
        mask = (uint16_t)(mask | (1u << buttons[b].id));
    }
    return mask;
}

/* Every port's buttons and sticks, by its type's names, in one list ending in a NULL description. */
static void send_descriptors(retro_environment_t environment, ControllersState* state)
{
    struct retro_input_descriptor descriptors[CONTROLLER_PORTS * (16 + 4) + 1];
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
        if (controllers_has_sticks(state->device[port]))
        {
            unsigned stick = 0;
            unsigned axis = 0;
            for (stick = 0; stick < 2; stick++)
            {
                for (axis = 0; axis < 2; axis++)
                {
                    descriptors[count].port = port;
                    descriptors[count].device = RETRO_DEVICE_ANALOG;
                    descriptors[count].index = stick;   /* RETRO_DEVICE_INDEX_ANALOG_LEFT / RIGHT */
                    descriptors[count].id = axis;       /* RETRO_DEVICE_ID_ANALOG_X / Y */
                    descriptors[count].description = STICK_NAMES[stick];
                    count++;
                }
            }
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
        unsigned id = 0;
        unsigned stick = 0;
        state->buttons[port] = 0;
        for (id = 0; id < 16; id++)
        {
            if (input_state(port, RETRO_DEVICE_JOYPAD, 0, id) != 0)
            {
                state->buttons[port] = (uint16_t)(state->buttons[port] | (1u << id));
            }
        }
        /* Both ways of reading buttons must agree; a frontend that gets one wrong is caught here. */
        if (state->bitmask_supported)
        {
            state->buttons_bitmask[port] = (uint16_t)input_state(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_MASK);
            if (state->buttons_bitmask[port] != state->buttons[port])
            {
                state->bitmask_mismatch[port] = true;
            }
        }
        for (stick = 0; stick < 2; stick++)
        {
            state->analog[port][stick][0] = input_state(port, RETRO_DEVICE_ANALOG, stick, RETRO_DEVICE_ID_ANALOG_X);
            state->analog[port][stick][1] = input_state(port, RETRO_DEVICE_ANALOG, stick, RETRO_DEVICE_ID_ANALOG_Y);
        }
    }
}

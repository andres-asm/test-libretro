#ifndef TEST_LIBRETRO_CONTROLLERS_H
#define TEST_LIBRETRO_CONTROLLERS_H

/*
 * Controller types, as a core with several emulated pads has them: each port
 * offers three (SET_CONTROLLER_INFO), each with its own buttons and its own
 * names for them (SET_INPUT_DESCRIPTORS, sent again whenever the frontend
 * plugs another type into a port with retro_set_controller_port_device).
 *
 *   RetroPad          RETRO_DEVICE_JOYPAD: every button, by the RetroPad's names,
 *                     and both sticks (analog descriptors "Move" / "Aim")
 *   Two-button pad    a subclass: the d-pad, FIRE (B), JUMP (A), PAUSE (Start)
 *   Six-button pad    a subclass: the d-pad, A (Y) B (B) C (A) X (L) Y (X) Z (R),
 *                     START, MODE (Select): the names don't match the RetroPad's
 */

#include <stdbool.h>
#include <stdint.h>

#include "libretro.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CONTROLLER_PORTS 4

#define DEVICE_TWO_BUTTON RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 0)
#define DEVICE_SIX_BUTTON RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 1)

typedef struct ControllerButton
{
    unsigned    id;     /* RETRO_DEVICE_ID_JOYPAD_* */
    const char* name;   /* the type's own name for it */
} ControllerButton;

typedef struct ControllersState
{
    bool     info_accepted;          /* SET_CONTROLLER_INFO */
    bool     descriptors_accepted;   /* the last SET_INPUT_DESCRIPTORS */
    unsigned descriptor_sends;       /* how many times they were sent */
    unsigned device[CONTROLLER_PORTS];        /* what the frontend plugged in last (RetroPad until it says) */
    unsigned device_changes;                  /* retro_set_controller_port_device calls */
    bool     unknown_device[CONTROLLER_PORTS];   /* it plugged something we never offered */
    uint16_t buttons[CONTROLLER_PORTS];       /* bit N = RETRO_DEVICE_ID_JOYPAD_N held, read this frame */
    int16_t  analog[CONTROLLER_PORTS][2][2];  /* [port][left / right stick][x / y], RetroPad ports only */
} ControllersState;

/* SET_CONTROLLER_INFO and the first descriptors (every port a RetroPad). */
void controllers_register(retro_environment_t environment, ControllersState* state);

/* retro_set_controller_port_device: keeps it and sends the descriptors again. */
void controllers_set_device(retro_environment_t environment, ControllersState* state, unsigned port, unsigned device);

/* Every port's buttons of its type, once per frame (after input_poll). */
void controllers_read(ControllersState* state, retro_input_state_t input_state);

const char* controllers_device_name(unsigned device);

/* Does the type have the two sticks? */
bool controllers_has_sticks(unsigned device);

/* The type's buttons (in the order shown), and how many. */
unsigned controllers_buttons(unsigned device, const ControllerButton** buttons);

#ifdef __cplusplus
}
#endif

#endif

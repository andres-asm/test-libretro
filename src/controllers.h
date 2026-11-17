#ifndef TEST_LIBRETRO_CONTROLLERS_H
#define TEST_LIBRETRO_CONTROLLERS_H

/*
 * The four ports' pads, and the controller types a core with several
 * emulated pads offers: each port offers four (SET_CONTROLLER_INFO), each with
 * its own buttons and its own names for them (SET_INPUT_DESCRIPTORS, sent
 * again whenever the frontend plugs another type into a port with
 * retro_set_controller_port_device).
 *
 *   RetroPad               RETRO_DEVICE_JOYPAD: every button and both sticks
 *   NES style gamepad      a JOYPAD subclass: d-pad, Select, Start, B, A
 *   SNES style gamepad     a JOYPAD subclass: + Y, X, L, R
 *   PSX DualShock style    an ANALOG subclass: Cross, Circle, Square, Triangle,
 *                          L1 R1 L2 R2 L3 R3, Select, Start and both sticks
 *
 * The sticks are described as "Left Stick" / "Right Stick" (both axes): a
 * frontend adds the direction.
 */

#include <stdbool.h>
#include <stdint.h>

#include "libretro.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CONTROLLER_PORTS 4

#define DEVICE_NES       RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 0)
#define DEVICE_SNES      RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 1)
#define DEVICE_DUALSHOCK RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_ANALOG, 0)

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

    /* Read every frame, whatever the type: */
    bool     bitmask_supported;                  /* GET_INPUT_BITMASKS answered yes */
    uint16_t buttons[CONTROLLER_PORTS];          /* bit N = RETRO_DEVICE_ID_JOYPAD_N, one button at a time */
    uint16_t buttons_bitmask[CONTROLLER_PORTS];  /* the same, from one RETRO_DEVICE_ID_JOYPAD_MASK query */
    bool     bitmask_mismatch[CONTROLLER_PORTS]; /* the two answers disagreed at least once */
    int16_t  analog[CONTROLLER_PORTS][2][2];     /* [port][left / right stick][x / y] */
} ControllersState;

/* SET_CONTROLLER_INFO and the first descriptors (every port a RetroPad). */
void controllers_register(retro_environment_t environment, ControllersState* state);

/* retro_set_controller_port_device: keeps it and sends the descriptors again. */
void controllers_set_device(retro_environment_t environment, ControllersState* state, unsigned port, unsigned device);

/* Every port's buttons and sticks, once per frame (after input_poll). */
void controllers_read(ControllersState* state, retro_input_state_t input_state);

/* The types offered, in order (for a port picker), and how many. */
unsigned controllers_type_count(void);
unsigned controllers_type_device(unsigned index);

const char* controllers_device_name(unsigned device);

/* Does the type have the two sticks? */
bool controllers_has_sticks(unsigned device);

/* The type's buttons, and how many. */
unsigned controllers_buttons(unsigned device, const ControllerButton** buttons);

/* The type's buttons as a mask (bit N = RETRO_DEVICE_ID_JOYPAD_N). */
uint16_t controllers_button_mask(unsigned device);

#ifdef __cplusplus
}
#endif

#endif

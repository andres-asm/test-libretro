#ifndef TEST_LIBRETRO_KEYBOARD_H
#define TEST_LIBRETRO_KEYBOARD_H

/*
 * The keyboard, both ways libretro gives it to a core:
 *   callback  SET_KEYBOARD_CALLBACK: the frontend calls the core on each
 *             key event (with the typed character and the modifiers)
 *   polled    input_state(port 0, RETRO_DEVICE_KEYBOARD, 0, RETROK_*) for
 *             every key, every frame: what is held now
 * The Keyboard page shows one or the other.
 */

#include <stdbool.h>
#include <stdint.h>

#include "libretro.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum KeyboardMode
{
    KEYBOARD_CALLBACK,
    KEYBOARD_POLLED,
    KEYBOARD_MODE_COUNT
} KeyboardMode;

#define KEYBOARD_LOG_SIZE 64

typedef struct KeyboardLogEntry
{
    bool     down;
    unsigned keycode;     /* RETROK_* */
    uint32_t character;   /* UTF-32, 0 if none */
    uint16_t modifiers;   /* RETROKMOD_* */
} KeyboardLogEntry;

typedef struct KeyboardState
{
    KeyboardMode     mode;
    bool             held[RETROK_LAST];          /* polled: held now */
    unsigned         held_count;
    bool             polled_any;                 /* polled: a key was seen held at least once */
    KeyboardLogEntry log[KEYBOARD_LOG_SIZE];     /* callback: the newest first */
    unsigned         log_count;
    unsigned         events;                     /* callback: events seen */
} KeyboardState;

/* Polled mode: every key's state now. */
void keyboard_poll(KeyboardState* keyboard, retro_input_state_t input_state);

/* Callback mode: one event from the frontend, kept in the log. */
void keyboard_log_event(KeyboardState* keyboard, bool down, unsigned keycode, uint32_t character, uint16_t modifiers);

const char* keyboard_mode_name(KeyboardMode mode);

/* A key's name ("A", "F1", "Left Shift"...) into `out`. */
void keyboard_key_name(unsigned keycode, char* out, unsigned out_size);

#ifdef __cplusplus
}
#endif

#endif

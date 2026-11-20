#ifndef TEST_LIBRETRO_TEXT_INPUT_H
#define TEST_LIBRETRO_TEXT_INPUT_H

/*
 * Multitap's text input extension (libretro_extensions.h, from host-libretro):
 * the core asks the frontend for the keyboard (KEYS: a phone opens its
 * on-screen keyboard) or for a string (TEXT: the frontend's own text box).
 *
 * Built in only when CMake finds libretro_extensions.h (TEST_LIBRETRO_EXTENSIONS);
 * shown only when the frontend takes it: at init the core asks for OFF, which
 * a frontend with the extension accepts and any other answers false.
 *
 * The Keyboard and Save RAM pages ask for the keyboard while they're up; the
 * Keyboard page asks for a name (hold Y) or a number.
 */

#include <stdbool.h>

#include "libretro.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TEXT_INPUT_ANSWER_SIZE 256

typedef struct TextInputState
{
    bool     supported;        /* the frontend took the extension */
    bool     keys_asked;       /* our last request was KEYS */
    bool     waiting;          /* a TEXT request is out, no answer yet */
    bool     last_cancelled;   /* the last answer was a cancel */
    unsigned answers;          /* TEXT answers received (cancels included) */
    char     last_answer[TEXT_INPUT_ANSWER_SIZE];
} TextInputState;

/* At init: whether the frontend has the extension (asks for OFF). */
void text_input_probe(TextInputState* text, retro_environment_t environment);

/* Every frame: whether a page wants the keyboard now (KEYS); asks only on a change. */
void text_input_want_keys(TextInputState* text, retro_environment_t environment, bool want);

/* Asks for a string (TEXT): a name, or digits when `number`. */
void text_input_ask(TextInputState* text, retro_environment_t environment, bool number);

#ifdef __cplusplus
}
#endif

#endif

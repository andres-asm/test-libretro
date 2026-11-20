#include "text_input.h"

#include <string.h>

#if defined(TEST_LIBRETRO_EXTENSIONS)

#include "libretro_extensions.h"

/* libretro's callbacks carry no pointer back to us; one core, one state. */
static TextInputState* g_text_input = NULL;

static void text_done(const char* text)
{
    TextInputState* state = g_text_input;
    if (state == NULL)
    {
        return;
    }
    state->waiting = false;
    state->answers++;
    state->last_cancelled = text == NULL;
    state->last_answer[0] = '\0';
    if (text != NULL)
    {
        size_t length = strlen(text);
        if (length >= sizeof(state->last_answer))
        {
            length = sizeof(state->last_answer) - 1;
        }
        memcpy(state->last_answer, text, length);
        state->last_answer[length] = '\0';
    }
}

static bool send_request(retro_environment_t environment, unsigned mode, unsigned type, const char* prompt,
                         unsigned max_length)
{
    struct retro_ext_text_input request;
    memset(&request, 0, sizeof(request));
    request.mode = mode;
    request.type = type;
    request.prompt = prompt;
    request.initial = "";
    request.max_length = max_length;
    request.done = text_done;
    return environment(RETRO_ENVIRONMENT_EXT_SET_TEXT_INPUT, &request);
}

void text_input_probe(TextInputState* text, retro_environment_t environment)
{
    memset(text, 0, sizeof(*text));
    g_text_input = text;
    text->supported = send_request(environment, RETRO_EXT_TEXT_INPUT_OFF, RETRO_EXT_TEXT_INPUT_TYPE_TEXT, NULL, 0);
}

void text_input_want_keys(TextInputState* text, retro_environment_t environment, bool want)
{
    /* A TEXT request out replaces KEYS until it's answered. */
    if (!text->supported || text->waiting || want == text->keys_asked)
    {
        return;
    }
    send_request(environment, want ? RETRO_EXT_TEXT_INPUT_KEYS : RETRO_EXT_TEXT_INPUT_OFF,
                 RETRO_EXT_TEXT_INPUT_TYPE_TEXT, NULL, 0);
    text->keys_asked = want;
}

void text_input_ask(TextInputState* text, retro_environment_t environment, bool number)
{
    if (!text->supported || text->waiting)
    {
        return;
    }
    if (send_request(environment, RETRO_EXT_TEXT_INPUT_TEXT,
                     number ? RETRO_EXT_TEXT_INPUT_TYPE_NUMBER : RETRO_EXT_TEXT_INPUT_TYPE_TEXT,
                     number ? "A NUMBER (UP TO 6 DIGITS)" : "YOUR NAME (UP TO 12 CHARACTERS)", number ? 6u : 12u))
    {
        text->waiting = true;
        text->keys_asked = false;   /* the TEXT request replaced KEYS; asked again after the answer */
    }
}

#else

/* Built without libretro_extensions.h: the frontend is never asked. */
void text_input_probe(TextInputState* text, retro_environment_t environment)
{
    (void)environment;
    memset(text, 0, sizeof(*text));
}

void text_input_want_keys(TextInputState* text, retro_environment_t environment, bool want)
{
    (void)text;
    (void)environment;
    (void)want;
}

void text_input_ask(TextInputState* text, retro_environment_t environment, bool number)
{
    (void)text;
    (void)environment;
    (void)number;
}

#endif

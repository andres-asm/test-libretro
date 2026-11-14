#include "options.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------------------------------------------------------------------------
 * The one definition (core options v2)
 * ------------------------------------------------------------------------- */

static struct retro_core_option_v2_category CATEGORIES[] = {
    { "video", "Video", "How the test pictures are produced." },
    { "audio", "Audio", "How the test tone is produced." },
    { NULL, NULL, NULL },
};

static struct retro_core_option_v2_definition DEFINITIONS[] = {
    {
        "libretro_test_pixel_format",
        "Video > Pixel format (restart)",
        "Pixel format (restart)",
        "Pixel format the core asks for. Applied when the core is loaded again.",
        NULL,
        "video",
        {
            { "XRGB8888", NULL },
            { "RGB565",   NULL },
            { "0RGB1555", NULL },
            { NULL, NULL },
        },
        "XRGB8888",
    },
    {
        "libretro_test_width",
        "Video > Width",
        "Width",
        "Image width, with square pixels: 640 x 480 is 4:3, 720 x 480 is 3:2. Changes are "
        "sent with SET_GEOMETRY (new base size and aspect ratio) while running.",
        NULL,
        "video",
        {
            { "640", "640 x 480" },
            { "720", "720 x 480" },
            { NULL, NULL },
        },
        "640",
    },
    {
        "libretro_test_height",
        "Video > Height (SET_SYSTEM_AV_INFO)",
        "Height (SET_SYSTEM_AV_INFO)",
        "Image height. Taller than the maximum the core reported at load, so it is sent with "
        "SET_SYSTEM_AV_INFO (new maximum size). Pixels stay square.",
        NULL,
        "video",
        {
            { "480", NULL },
            { "600", NULL },
            { "720", NULL },
            { NULL, NULL },
        },
        "480",
    },
    {
        "libretro_test_fps",
        "Video > Frame rate (SET_SYSTEM_AV_INFO)",
        "Frame rate (SET_SYSTEM_AV_INFO)",
        "Frames per second the core runs at, sent with SET_SYSTEM_AV_INFO. The frontend must run "
        "the core at this speed: each frame carries sample rate / fps audio frames.",
        NULL,
        "video",
        {
            { "60", "60 fps" },
            { "50", "50 fps" },
            { "40", "40 fps" },
            { "30", "30 fps" },
            { NULL, NULL },
        },
        "60",
    },
    {
        "libretro_test_sample_rate",
        "Audio > Sample rate (SET_SYSTEM_AV_INFO)",
        "Sample rate (SET_SYSTEM_AV_INFO)",
        "The tone's sample rate, sent with SET_SYSTEM_AV_INFO. Lower and higher than the usual "
        "48000 Hz device, so the frontend's resampler works both ways. The tone must stay a clean "
        "440 Hz at every rate.",
        NULL,
        "audio",
        {
            { "48000", "48000 Hz" },
            { "44100", "44100 Hz" },
            { "32000", "32000 Hz" },
            { "96000", "96000 Hz" },
            { NULL, NULL },
        },
        "48000",
    },
    { NULL, NULL, NULL, NULL, NULL, NULL, { { NULL, NULL } }, NULL },
};

#define DEFINITION_COUNT (sizeof(DEFINITIONS) / sizeof(DEFINITIONS[0]) - 1)

/* ---------------------------------------------------------------------------
 * Conversions for older frontends
 * ------------------------------------------------------------------------- */

/*
 * The frontend has to copy what we hand it (libretro.h), but keeping these
 * static is cheap and protects us from frontends that don't.
 */
static struct retro_core_option_definition g_v1_definitions[DEFINITION_COUNT + 1];

#define V0_VALUE_SIZE 256
static char g_v0_values[DEFINITION_COUNT][V0_VALUE_SIZE];
static struct retro_variable g_v0_variables[DEFINITION_COUNT + 1];

static void build_v1(void)
{
    size_t i;

    memset(g_v1_definitions, 0, sizeof(g_v1_definitions));
    for (i = 0; i < DEFINITION_COUNT; i++)
    {
        const struct retro_core_option_v2_definition* v2 = &DEFINITIONS[i];
        struct retro_core_option_definition* v1 = &g_v1_definitions[i];

        /* v1 has no categories, so it gets the full "Video > ..." label. */
        v1->key = v2->key;
        v1->desc = v2->desc;
        v1->info = v2->info;
        memcpy(v1->values, v2->values, sizeof(v1->values));   /* same struct in v1 and v2 */
        v1->default_value = v2->default_value;
    }
}

/* v0 has no separate default: the FIRST value is the default. So it goes first. */
static void build_v0(void)
{
    size_t i;
    size_t v;

    memset(g_v0_variables, 0, sizeof(g_v0_variables));
    for (i = 0; i < DEFINITION_COUNT; i++)
    {
        const struct retro_core_option_v2_definition* v2 = &DEFINITIONS[i];
        char* text = g_v0_values[i];
        size_t used = 0;

        used += (size_t)snprintf(text, V0_VALUE_SIZE, "%s; %s", v2->desc, v2->default_value);
        for (v = 0; v2->values[v].value != NULL && used < V0_VALUE_SIZE; v++)
        {
            if (strcmp(v2->values[v].value, v2->default_value) != 0)
            {
                used += (size_t)snprintf(text + used, V0_VALUE_SIZE - used, "|%s", v2->values[v].value);
            }
        }

        g_v0_variables[i].key = v2->key;
        g_v0_variables[i].value = text;
    }
}

/* ---------------------------------------------------------------------------
 * Public functions
 * ------------------------------------------------------------------------- */

void options_register(retro_environment_t environment, OptionsReport* report)
{
    unsigned version = 0;

    memset(report, 0, sizeof(*report));
    report->version_answered = environment(RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION, &version);
    report->version = report->version_answered ? version : 0;

    if (report->version >= 2)
    {
        struct retro_core_options_v2 options;
        options.categories = CATEGORIES;
        options.definitions = DEFINITIONS;
        report->api_used = 2;
        report->accepted = environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2, &options);
    }
    else if (report->version == 1)
    {
        build_v1();
        report->api_used = 1;
        report->accepted = environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS, g_v1_definitions);
    }
    else
    {
        build_v0();
        report->api_used = 0;
        report->accepted = environment(RETRO_ENVIRONMENT_SET_VARIABLES, g_v0_variables);
    }
}

static bool get_variable(retro_environment_t environment, const char* key, const char** value)
{
    struct retro_variable variable;

    variable.key = key;
    variable.value = NULL;
    if (!environment(RETRO_ENVIRONMENT_GET_VARIABLE, &variable) || variable.value == NULL)
    {
        return false;
    }
    *value = variable.value;
    return true;
}

void options_read(retro_environment_t environment, OptionValues* values)
{
    const char* value = NULL;

    memset(values, 0, sizeof(*values));
    snprintf(values->pixel_format, sizeof(values->pixel_format), "%s", "XRGB8888");
    values->width = OPTION_WIDTH_DEFAULT;
    values->height = OPTION_HEIGHT_DEFAULT;
    values->fps = OPTION_FPS_DEFAULT;
    values->sample_rate = OPTION_SAMPLE_RATE_DEFAULT;

    if (get_variable(environment, "libretro_test_pixel_format", &value))
    {
        values->pixel_format_answered = true;
        snprintf(values->pixel_format, sizeof(values->pixel_format), "%s", value);
    }

    if (get_variable(environment, "libretro_test_width", &value))
    {
        values->width_answered = true;
        values->width = strcmp(value, "720") == 0 ? OPTION_WIDTH_WIDE : OPTION_WIDTH_DEFAULT;
    }

    if (get_variable(environment, "libretro_test_height", &value))
    {
        unsigned height = (unsigned)strtoul(value, NULL, 10);
        values->height_answered = true;
        values->height = (height == 600u || height == 720u) ? height : OPTION_HEIGHT_DEFAULT;
    }

    if (get_variable(environment, "libretro_test_fps", &value))
    {
        unsigned fps = (unsigned)strtoul(value, NULL, 10);
        values->fps_answered = true;
        values->fps = (fps == 50u || fps == 40u || fps == 30u) ? fps : OPTION_FPS_DEFAULT;
    }

    if (get_variable(environment, "libretro_test_sample_rate", &value))
    {
        unsigned rate = (unsigned)strtoul(value, NULL, 10);
        values->sample_rate_answered = true;
        values->sample_rate = (rate == 44100u || rate == 32000u || rate == 96000u) ? rate : OPTION_SAMPLE_RATE_DEFAULT;
    }
}

bool options_changed(retro_environment_t environment)
{
    bool updated = false;
    return environment(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated;
}

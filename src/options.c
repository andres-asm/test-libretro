#include "options.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------------------------------------------------------------------------
 * The one definition (core options v2)
 * ------------------------------------------------------------------------- */

static struct retro_core_option_v2_category CATEGORIES[] = {
    { "video", "Video", "How the test pictures are produced." },
    { NULL, NULL, NULL },
};

static struct retro_core_option_v2_definition DEFINITIONS[] = {
    {
        "testwizard_pixel_format",
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
        "testwizard_width",
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

    if (get_variable(environment, "testwizard_pixel_format", &value))
    {
        values->pixel_format_answered = true;
        snprintf(values->pixel_format, sizeof(values->pixel_format), "%s", value);
    }

    if (get_variable(environment, "testwizard_width", &value))
    {
        values->width_answered = true;
        values->width = strcmp(value, "720") == 0 ? OPTION_WIDTH_WIDE : OPTION_WIDTH_DEFAULT;
    }
}

bool options_changed(retro_environment_t environment)
{
    bool updated = false;
    return environment(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated;
}

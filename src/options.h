#ifndef TEST_WIZARD_OPTIONS_H
#define TEST_WIZARD_OPTIONS_H

/*
 * The core's options, defined once as core options v2 and handed to the
 * frontend in the newest format it says it supports, the way real cores do it
 * (libretro's libretro_core_options.h template):
 *
 *   GET_CORE_OPTIONS_VERSION >= 2  ->  SET_CORE_OPTIONS_V2 (with categories)
 *   GET_CORE_OPTIONS_VERSION == 1  ->  SET_CORE_OPTIONS    (v2 table converted, no categories)
 *   0 or no answer                 ->  SET_VARIABLES       ("Label; default|other|...")
 *
 * So one core tests a frontend's v2, v1 and v0 paths, depending only on which
 * version that frontend reports.
 */

#include <stdbool.h>

#include "libretro.h"

#define OPTION_WIDTH_DEFAULT 640u
#define OPTION_WIDTH_WIDE    720u

typedef struct OptionsReport
{
    bool     version_answered;   /* GET_CORE_OPTIONS_VERSION returned true */
    unsigned version;            /* what it reported (0 if not answered) */
    unsigned api_used;           /* 2, 1 or 0: which SET_* call we made */
    bool     accepted;           /* that call returned true */
} OptionsReport;

/* Current option values, as last read with GET_VARIABLE. */
typedef struct OptionValues
{
    bool     pixel_format_answered;
    char     pixel_format[32];     /* "XRGB8888", "RGB565" or "0RGB1555" */
    bool     width_answered;
    unsigned width;                /* OPTION_WIDTH_DEFAULT or OPTION_WIDTH_WIDE */
} OptionValues;

/* Called from retro_set_environment. */
void options_register(retro_environment_t environment, OptionsReport* report);

/* GET_VARIABLE for every option. Values the frontend doesn't answer keep their defaults. */
void options_read(retro_environment_t environment, OptionValues* values);

/* GET_VARIABLE_UPDATE: true if the frontend says an option changed since the last read. */
bool options_changed(retro_environment_t environment);

#endif

#ifndef TEST_WIZARD_CORE_H
#define TEST_WIZARD_CORE_H

/*
 * State shared by the core's modules.
 *
 * A libretro core has exactly one instance per loaded library (the API has no
 * instance pointer), so core.c keeps one CoreState and passes a pointer to it
 * around instead of each module having its own globals.
 */

#include <stdbool.h>
#include <stdint.h>

#include "audio.h"
#include "input.h"
#include "libretro.h"
#include "options.h"
#include "video.h"
#include "wizard.h"

#define TEXT_FIELD_SIZE 256

/* What the frontend answered to the environment calls we make. Shown on the first page. */
typedef struct EnvironmentReport
{
    bool log_interface;

    bool support_no_game_accepted;
    OptionsReport options;            /* which options API we used and whether it was accepted */

    bool format_0rgb1555_accepted;    /* libretro's default; a frontend must accept it */
    bool format_rgb565_accepted;
    bool format_xrgb8888_accepted;

    bool can_dupe_answered;
    bool can_dupe;

    bool system_directory_answered;
    char system_directory[TEXT_FIELD_SIZE];
    bool save_directory_answered;
    char save_directory[TEXT_FIELD_SIZE];
    bool libretro_path_answered;
    char libretro_path[TEXT_FIELD_SIZE];

    bool     language_answered;
    unsigned language;

    bool input_bitmasks;              /* GET_INPUT_BITMASKS (called with NULL data, as the API specifies) */

    bool audio_video_enable_answered;
    int  audio_video_enable;

    unsigned geometry_changes;        /* SET_GEOMETRY calls made (width option changes) */
    bool     geometry_accepted;       /* answer to the last one */
} EnvironmentReport;

typedef struct CoreState
{
    retro_environment_t        environment;
    retro_video_refresh_t      video_refresh;
    retro_audio_sample_t       audio_sample;
    retro_audio_sample_batch_t audio_sample_batch;
    retro_input_poll_t         input_poll;
    retro_input_state_t        input_state;
    retro_log_printf_t         log;

    EnvironmentReport report;
    OptionValues      options;        /* last GET_VARIABLE answers */

    Framebuffer framebuffer;
    bool        game_loaded;
    InputState  input;
    AudioState  audio;
    Wizard      wizard;

    uint64_t frame_count;          /* saved in savestates */
    unsigned load_state_count;     /* successful retro_unserialize calls */
} CoreState;

/* Logs through the frontend's log interface (or stderr when there is none), prefixed "[test-wizard] ". */
#if defined(__GNUC__)
void core_log(CoreState* core, enum retro_log_level level, const char* format, ...) __attribute__((format(printf, 3, 4)));
#else
void core_log(CoreState* core, enum retro_log_level level, const char* format, ...);
#endif

#endif

/*
 * libretro entry points of the Test libretro core.
 *
 * Order the frontend calls us in (libretro.h):
 *   retro_set_environment -> retro_get_system_info -> retro_init
 *   -> retro_set_video_refresh / audio / input callbacks
 *   -> retro_load_game -> retro_get_system_av_info -> retro_run (every frame)
 *   -> retro_unload_game -> retro_deinit
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "core.h"

/* The core's single instance. See core.h for why this is the one global. */
static CoreState g_core;

/*
 * Savestate layout. Fixed size, so retro_serialize_size() never changes.
 * Version 2 adds where the user was: the page and every page's result, so
 * loading a state brings back that screen and those results.
 */
#define SAVESTATE_MAGIC   0x5A57544Du   /* "MTWZ" */
#define SAVESTATE_VERSION 2u

typedef struct SaveStateData
{
    uint32_t magic;
    uint32_t version;
    uint64_t frame_count;
    uint32_t page;                     /* PageId */
    uint32_t results[PAGE_COUNT];      /* TestResult per page */
} SaveStateData;

/* ---------------------------------------------------------------------------
 * Logging
 * ------------------------------------------------------------------------- */

void core_log(CoreState* core, enum retro_log_level level, const char* format, ...)
{
    char message[512];
    va_list args;

    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);

    if (core->log != NULL)
    {
        core->log(level, "[Test libretro] %s", message);
    }
    else
    {
        fprintf(stderr, "[Test libretro] %s", message);
    }
}

/* ---------------------------------------------------------------------------
 * Environment helpers
 * ------------------------------------------------------------------------- */

static void copy_text(char* destination, size_t size, const char* source)
{
    snprintf(destination, size, "%s", source != NULL ? source : "(null)");
}

static bool try_pixel_format(enum retro_pixel_format format)
{
    enum retro_pixel_format value = format;
    return g_core.environment(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &value);
}

/*
 * Asks for every pixel format to fill the report, then for the one we'll use
 * LAST, because the frontend keeps the most recent accepted value.
 */
static enum retro_pixel_format choose_pixel_format(void)
{
    EnvironmentReport* report = &g_core.report;
    enum retro_pixel_format wanted = RETRO_PIXEL_FORMAT_XRGB8888;
    enum retro_pixel_format chosen = RETRO_PIXEL_FORMAT_0RGB1555;

    if (strcmp(g_core.options.pixel_format, "RGB565") == 0)
    {
        wanted = RETRO_PIXEL_FORMAT_RGB565;
    }
    else if (strcmp(g_core.options.pixel_format, "0RGB1555") == 0)
    {
        wanted = RETRO_PIXEL_FORMAT_0RGB1555;
    }

    report->format_0rgb1555_accepted = try_pixel_format(RETRO_PIXEL_FORMAT_0RGB1555);
    report->format_rgb565_accepted   = try_pixel_format(RETRO_PIXEL_FORMAT_RGB565);
    report->format_xrgb8888_accepted = try_pixel_format(RETRO_PIXEL_FORMAT_XRGB8888);

    if (wanted == RETRO_PIXEL_FORMAT_XRGB8888 && report->format_xrgb8888_accepted)
    {
        chosen = RETRO_PIXEL_FORMAT_XRGB8888;
    }
    else if (wanted == RETRO_PIXEL_FORMAT_RGB565 && report->format_rgb565_accepted)
    {
        chosen = RETRO_PIXEL_FORMAT_RGB565;
    }
    else if (report->format_xrgb8888_accepted && wanted != RETRO_PIXEL_FORMAT_0RGB1555)
    {
        chosen = RETRO_PIXEL_FORMAT_XRGB8888;
    }
    else
    {
        chosen = RETRO_PIXEL_FORMAT_0RGB1555;   /* libretro's default; works even if refused */
    }

    try_pixel_format(chosen);
    return chosen;
}

/* ---------------------------------------------------------------------------
 * Setup
 * ------------------------------------------------------------------------- */

void retro_set_environment(retro_environment_t callback)
{
    struct retro_log_callback log_callback;
    bool no_game = true;

    g_core.environment = callback;

    memset(&log_callback, 0, sizeof(log_callback));
    if (callback(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log_callback) && log_callback.log != NULL)
    {
        g_core.log = log_callback.log;
        g_core.report.log_interface = true;
    }

    g_core.report.support_no_game_accepted = callback(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_game);

    /* v2, v1 or SET_VARIABLES, whichever the frontend says it supports (options.h). */
    options_register(callback, &g_core.report.options);
}

void retro_set_video_refresh(retro_video_refresh_t callback)           { g_core.video_refresh = callback; }
void retro_set_audio_sample(retro_audio_sample_t callback)             { g_core.audio_sample = callback; }
void retro_set_audio_sample_batch(retro_audio_sample_batch_t callback) { g_core.audio_sample_batch = callback; }
void retro_set_input_poll(retro_input_poll_t callback)                 { g_core.input_poll = callback; }
void retro_set_input_state(retro_input_state_t callback)               { g_core.input_state = callback; }

unsigned retro_api_version(void)
{
    return RETRO_API_VERSION;
}

void retro_get_system_info(struct retro_system_info* info)
{
    memset(info, 0, sizeof(*info));
    info->library_name     = "Test libretro";
    info->library_version  = "0.1";
    info->valid_extensions = "";
    info->need_fullpath    = false;
    info->block_extract    = false;
}

/* The frontend's key events (typing on the SRAM page). Only queued here:
   Nuklear gets them at the start of the next frame. */
static void RETRO_CALLCONV keyboard_event(bool down, unsigned keycode, uint32_t character, uint16_t key_modifiers)
{
    KeyEvent* event = NULL;
    (void)key_modifiers;

    if (g_core.key_queue_count >= KEY_QUEUE_SIZE)
    {
        g_core.keys_dropped++;
        return;
    }
    event = &g_core.key_queue[g_core.key_queue_count];
    event->down = down;
    event->keycode = keycode;
    event->character = character;
    g_core.key_queue_count++;
}

void retro_init(void)
{
    EnvironmentReport* report = &g_core.report;
    const char* text = NULL;
    bool flag = false;

    text = NULL;
    report->system_directory_answered = g_core.environment(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &text);
    copy_text(report->system_directory, sizeof(report->system_directory), text);

    text = NULL;
    report->save_directory_answered = g_core.environment(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &text);
    copy_text(report->save_directory, sizeof(report->save_directory), text);

    text = NULL;
    report->libretro_path_answered = g_core.environment(RETRO_ENVIRONMENT_GET_LIBRETRO_PATH, &text);
    copy_text(report->libretro_path, sizeof(report->libretro_path), text);

    report->language_answered = g_core.environment(RETRO_ENVIRONMENT_GET_LANGUAGE, &report->language);

    flag = false;
    report->can_dupe_answered = g_core.environment(RETRO_ENVIRONMENT_GET_CAN_DUPE, &flag);
    report->can_dupe = flag;

    {
        struct retro_keyboard_callback keyboard;
        keyboard.callback = keyboard_event;
        report->keyboard_callback_accepted = g_core.environment(RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK, &keyboard);
    }

    /* Specified with NULL data: the return value IS the answer. */
    report->input_bitmasks = g_core.environment(RETRO_ENVIRONMENT_GET_INPUT_BITMASKS, NULL);
    g_core.input.bitmask_supported = report->input_bitmasks;

    core_log(&g_core, RETRO_LOG_INFO, "init\n");
}

void retro_deinit(void)
{
    core_log(&g_core, RETRO_LOG_INFO, "deinit\n");
    memset(&g_core, 0, sizeof(g_core));
}

/* ---------------------------------------------------------------------------
 * Content
 * ------------------------------------------------------------------------- */

bool retro_load_game(const struct retro_game_info* game)
{
    EnvironmentReport* report = &g_core.report;
    enum retro_pixel_format format;

    (void)game;   /* content is ignored; the core runs without it */

    options_read(g_core.environment, &g_core.options);
    format = choose_pixel_format();

    /* Zero until the frontend loads the saved block (after this function returns). */
    memset(&g_core.sram, 0, sizeof(g_core.sram));
    g_core.sram_state = SRAM_NOT_CHECKED;

    report->audio_video_enable_answered =
        g_core.environment(RETRO_ENVIRONMENT_GET_AUDIO_VIDEO_ENABLE, &report->audio_video_enable);

    g_core.fps = g_core.options.fps;
    g_core.max_height = g_core.options.height;
    g_core.audio.frames_per_run = (unsigned)(AUDIO_SAMPLE_RATE / (double)g_core.fps);
    if (!video_create(&g_core.framebuffer, format, g_core.options.width, g_core.options.height))
    {
        core_log(&g_core, RETRO_LOG_ERROR, "could not create the framebuffer\n");
        return false;
    }

    g_core.game_loaded = true;
    core_log(&g_core, RETRO_LOG_INFO, "running, pixel format %s, %ux%u, options API v%u (%s)\n",
             video_pixel_format_name(format), g_core.framebuffer.width, g_core.framebuffer.height,
             report->options.api_used, report->options.accepted ? "accepted" : "refused");
    return true;
}

bool retro_load_game_special(unsigned type, const struct retro_game_info* info, size_t count)
{
    (void)type;
    (void)info;
    (void)count;
    return false;
}

void retro_unload_game(void)
{
    video_destroy(&g_core.framebuffer);
    g_core.game_loaded = false;
}

void retro_get_system_av_info(struct retro_system_av_info* info)
{
    memset(info, 0, sizeof(*info));
    /* max_width covers both width options, so changing the option at runtime
       only needs SET_GEOMETRY (base size), never SET_SYSTEM_AV_INFO. */
    info->geometry.base_width   = g_core.framebuffer.width > 0 ? g_core.framebuffer.width : OPTION_WIDTH_DEFAULT;
    info->geometry.base_height  = g_core.framebuffer.height > 0 ? g_core.framebuffer.height : OPTION_HEIGHT_DEFAULT;
    info->geometry.max_width    = VIDEO_MAX_WIDTH;
    /* Only as tall as the current height: a taller height option then NEEDS
       SET_SYSTEM_AV_INFO (new maximum), which is what that option tests. */
    info->geometry.max_height   = g_core.max_height > 0 ? g_core.max_height : OPTION_HEIGHT_DEFAULT;
    info->geometry.aspect_ratio = video_aspect_ratio(info->geometry.base_width, info->geometry.base_height);
    info->timing.fps            = g_core.fps > 0 ? (double)g_core.fps : (double)OPTION_FPS_DEFAULT;
    info->timing.sample_rate    = AUDIO_SAMPLE_RATE;
}

/* ---------------------------------------------------------------------------
 * Running
 * ------------------------------------------------------------------------- */

/*
 * Re-reads the options when the frontend says one changed, and applies them
 * right away (only allowed from inside retro_run):
 *   - height or frame rate changed -> SET_SYSTEM_AV_INFO (new maximum size or
 *     new timing: the frontend must reinitialize video/audio timing);
 *   - only the width changed       -> SET_GEOMETRY (fits the reported maximum).
 * Pixels stay square, so the aspect ratio is always width / height.
 */
static bool resize_framebuffer(unsigned width, unsigned height)
{
    unsigned old_width = g_core.framebuffer.width;
    unsigned old_height = g_core.framebuffer.height;
    enum retro_pixel_format format = g_core.framebuffer.format;

    video_destroy(&g_core.framebuffer);
    if (video_create(&g_core.framebuffer, format, width, height))
    {
        return true;
    }
    core_log(&g_core, RETRO_LOG_ERROR, "could not resize the framebuffer to %ux%u, staying at %ux%u\n", width,
             height, old_width, old_height);
    if (!video_create(&g_core.framebuffer, format, old_width, old_height))
    {
        core_log(&g_core, RETRO_LOG_ERROR, "could not recreate the framebuffer either\n");
        g_core.game_loaded = false;
    }
    return false;
}

static void apply_option_changes(void)
{
    unsigned old_width = g_core.framebuffer.width;
    unsigned old_height = g_core.framebuffer.height;
    unsigned old_fps = g_core.fps;

    if (!options_changed(g_core.environment))
    {
        return;
    }
    options_read(g_core.environment, &g_core.options);

    if ((g_core.options.width != old_width || g_core.options.height != old_height) &&
        !resize_framebuffer(g_core.options.width, g_core.options.height))
    {
        return;
    }

    if (g_core.options.height != old_height || g_core.options.fps != old_fps)
    {
        struct retro_system_av_info info;

        g_core.max_height = g_core.framebuffer.height;
        g_core.fps = g_core.options.fps;
        g_core.audio.frames_per_run = (unsigned)(AUDIO_SAMPLE_RATE / (double)g_core.fps);
        retro_get_system_av_info(&info);
        g_core.report.av_info_changes++;
        g_core.report.av_info_accepted = g_core.environment(RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO, &info);
        core_log(&g_core, RETRO_LOG_INFO, "%ux%u @ %u fps -> %ux%u @ %u fps, SET_SYSTEM_AV_INFO %s\n", old_width,
                 old_height, old_fps, g_core.framebuffer.width, g_core.framebuffer.height, g_core.fps,
                 g_core.report.av_info_accepted ? "accepted" : "refused");
        return;
    }

    if (g_core.options.width != old_width)
    {
        struct retro_game_geometry geometry;

        memset(&geometry, 0, sizeof(geometry));
        geometry.base_width   = g_core.framebuffer.width;
        geometry.base_height  = g_core.framebuffer.height;
        geometry.max_width    = VIDEO_MAX_WIDTH;
        geometry.max_height   = g_core.max_height;
        geometry.aspect_ratio = video_aspect_ratio(g_core.framebuffer.width, g_core.framebuffer.height);
        g_core.report.geometry_changes++;
        g_core.report.geometry_accepted = g_core.environment(RETRO_ENVIRONMENT_SET_GEOMETRY, &geometry);
        core_log(&g_core, RETRO_LOG_INFO, "width %u -> %u, SET_GEOMETRY %s\n", old_width, g_core.framebuffer.width,
                 g_core.report.geometry_accepted ? "accepted" : "refused");
    }
}

void retro_run(void)
{
    if (!g_core.game_loaded)
    {
        return;
    }

    apply_option_changes();
    if (!g_core.game_loaded)
    {
        return;
    }

    /* First frame: the frontend had its chance to load save RAM. */
    if (g_core.sram_state == SRAM_NOT_CHECKED)
    {
        g_core.sram_state = sram_check(&g_core.sram);
        core_log(&g_core, g_core.sram_state == SRAM_CORRUPTED ? RETRO_LOG_ERROR : RETRO_LOG_INFO,
                 "save RAM: %s%s%s\n", sram_state_name(g_core.sram_state),
                 g_core.sram_state == SRAM_RESTORED ? ", text: " : "",
                 g_core.sram_state == SRAM_RESTORED ? g_core.sram.text : "");
    }

    g_core.frame_count++;

    input_update(&g_core.input, g_core.input_poll, g_core.input_state,
                 g_core.framebuffer.width, g_core.framebuffer.height);
    pages_run_frame(&g_core);

    g_core.video_refresh(g_core.framebuffer.pixels, g_core.framebuffer.width,
                         g_core.framebuffer.height, g_core.framebuffer.pitch);
    audio_run_frame(&g_core.audio, g_core.audio_sample, g_core.audio_sample_batch);
}

void retro_reset(void)
{
    g_core.frame_count = 0;
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
    (void)port;
    (void)device;
}

/* ---------------------------------------------------------------------------
 * Savestates
 * ------------------------------------------------------------------------- */

size_t retro_serialize_size(void)
{
    return sizeof(SaveStateData);
}

bool retro_serialize(void* data, size_t size)
{
    SaveStateData state;
    unsigned i = 0;

    if (size < sizeof(state))
    {
        return false;
    }
    memset(&state, 0, sizeof(state));
    state.magic = SAVESTATE_MAGIC;
    state.version = SAVESTATE_VERSION;
    state.frame_count = g_core.frame_count;
    state.page = (uint32_t)g_core.pages.page;
    for (i = 0; i < PAGE_COUNT; i++)
    {
        state.results[i] = (uint32_t)g_core.pages.results[i];
    }
    memcpy(data, &state, sizeof(state));

    g_core.states.saved++;
    g_core.states.saved_frame = g_core.frame_count;
    g_core.states.saved_page = g_core.pages.page;
    return true;
}

bool retro_unserialize(const void* data, size_t size)
{
    SaveStateData state;
    unsigned i = 0;

    if (size < sizeof(state))
    {
        core_log(&g_core, RETRO_LOG_WARN, "state too small (%zu bytes)\n", size);
        return false;
    }
    memcpy(&state, data, sizeof(state));
    if (state.magic != SAVESTATE_MAGIC || state.version != SAVESTATE_VERSION)
    {
        core_log(&g_core, RETRO_LOG_WARN, "not a Test libretro state (or an older version)\n");
        return false;
    }
    /* Never trust a file: reject values this core couldn't have written. */
    if (state.page >= PAGE_COUNT)
    {
        core_log(&g_core, RETRO_LOG_WARN, "state has an invalid page (%u)\n", state.page);
        return false;
    }
    for (i = 0; i < PAGE_COUNT; i++)
    {
        if (state.results[i] > RESULT_FAIL)
        {
            core_log(&g_core, RETRO_LOG_WARN, "state has an invalid result for page %u\n", i);
            return false;
        }
    }

    g_core.frame_count = state.frame_count;
    g_core.pages.page = (PageId)state.page;
    for (i = 0; i < PAGE_COUNT; i++)
    {
        g_core.pages.results[i] = (TestResult)state.results[i];
    }
    /* A Start/Select hold in progress mustn't carry over to the restored page. */
    g_core.pages.waiting_for_release = true;

    g_core.load_state_count++;
    g_core.states.loaded_frame = state.frame_count;
    g_core.states.loaded_page = (PageId)state.page;
    core_log(&g_core, RETRO_LOG_INFO, "state loaded: frame %llu, page %s\n", (unsigned long long)state.frame_count,
             pages_title((PageId)state.page));
    return true;
}

/* ---------------------------------------------------------------------------
 * Unused parts of the API
 * ------------------------------------------------------------------------- */

void retro_cheat_reset(void)
{
}

void retro_cheat_set(unsigned index, bool enabled, const char* code)
{
    (void)index;
    (void)enabled;
    (void)code;
}

unsigned retro_get_region(void)
{
    return RETRO_REGION_NTSC;
}

void* retro_get_memory_data(unsigned id)
{
    return id == RETRO_MEMORY_SAVE_RAM ? &g_core.sram : NULL;
}

size_t retro_get_memory_size(unsigned id)
{
    return id == RETRO_MEMORY_SAVE_RAM ? sizeof(g_core.sram) : 0;
}

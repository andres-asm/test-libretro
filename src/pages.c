#include "pages.h"

#include <stdio.h>
#include <string.h>

#include "core.h"
#include "nuklear_config.h"

/* ---------------------------------------------------------------------------
 * Colors
 * ------------------------------------------------------------------------- */

static struct nk_color color_good(void)    { return nk_rgb(90, 210, 120); }
static struct nk_color color_bad(void)     { return nk_rgb(235, 90, 90); }
static struct nk_color color_info(void)    { return nk_rgb(175, 175, 190); }
static struct nk_color color_pressed(void) { return nk_rgb(255, 220, 80); }

/* ---------------------------------------------------------------------------
 * Names
 * ------------------------------------------------------------------------- */

const char* pages_title(PageId page)
{
    switch (page)
    {
        case PAGE_ENVIRONMENT: return "Environment";
        case PAGE_VIDEO:       return "Video";
        case PAGE_INPUT:       return "Input";
        case PAGE_AUDIO:       return "Audio";
        case PAGE_SRAM:        return "Save RAM";
        case PAGE_SAVESTATE:   return "Savestates";
        case PAGE_SUMMARY:     return "Summary";
        default:               return "?";
    }
}

static const char* result_name(TestResult result)
{
    switch (result)
    {
        case RESULT_PASS: return "PASS";
        case RESULT_FAIL: return "FAIL";
        default:          return "not tested";
    }
}

static struct nk_color result_color(TestResult result)
{
    switch (result)
    {
        case RESULT_PASS: return color_good();
        case RESULT_FAIL: return color_bad();
        default:          return color_info();
    }
}

/* ---------------------------------------------------------------------------
 * Navigation
 * ------------------------------------------------------------------------- */

static void finish_page(CoreState* core, TestResult result)
{
    Pages* pages = &core->pages;

    if (pages->page == PAGE_SUMMARY)
    {
        if (!pages->shutdown_requested)
        {
            pages->shutdown_requested = true;
            core_log(core, RETRO_LOG_INFO, "finished, requesting SHUTDOWN\n");
            core->environment(RETRO_ENVIRONMENT_SHUTDOWN, NULL);
        }
        return;
    }

    pages->results[pages->page] = result;
    core_log(core, RETRO_LOG_INFO, "%s: %s\n", pages_title(pages->page), result_name(result));
    pages->page = (PageId)(pages->page + 1);
}

static void previous_page(Pages* pages)
{
    if (pages->page > PAGE_ENVIRONMENT)
    {
        pages->page = (PageId)(pages->page - 1);
    }
}

/* Joypad controls. Mouse/pointer clicks are handled by the buttons in draw_navigation(). */
static void handle_controller(CoreState* core)
{
    Pages* pages = &core->pages;
    const InputState* input = &core->input;

    if (pages->waiting_for_release)
    {
        if (!input_is_down(input, RETRO_DEVICE_ID_JOYPAD_START) && !input_is_down(input, RETRO_DEVICE_ID_JOYPAD_SELECT))
        {
            pages->waiting_for_release = false;
        }
        return;
    }

    if (input->start_held_frames >= INPUT_HOLD_FRAMES)
    {
        pages->waiting_for_release = true;
        finish_page(core, RESULT_PASS);
        return;
    }
    if (input->select_held_frames >= INPUT_HOLD_FRAMES && pages->page != PAGE_SUMMARY)
    {
        pages->waiting_for_release = true;
        finish_page(core, RESULT_FAIL);
        return;
    }
    if (input_was_pressed(input, RETRO_DEVICE_ID_JOYPAD_L))
    {
        previous_page(pages);
    }

    /* Page-specific controls. */
    if (pages->page == PAGE_AUDIO)
    {
        if (input_was_pressed(input, RETRO_DEVICE_ID_JOYPAD_RIGHT))
        {
            core->audio.channels = (AudioChannels)((core->audio.channels + 1) % AUDIO_CHANNELS_COUNT);
        }
        if (input_was_pressed(input, RETRO_DEVICE_ID_JOYPAD_LEFT))
        {
            core->audio.channels = (AudioChannels)((core->audio.channels + AUDIO_CHANNELS_COUNT - 1) % AUDIO_CHANNELS_COUNT);
        }
        if (input_was_pressed(input, RETRO_DEVICE_ID_JOYPAD_UP))
        {
            core->audio.use_single_sample_callback = !core->audio.use_single_sample_callback;
        }
    }
}

/* ---------------------------------------------------------------------------
 * Shared UI pieces
 * ------------------------------------------------------------------------- */

static void draw_row(struct nk_context* ctx, const char* name, const char* value, struct nk_color color)
{
    nk_layout_row_dynamic(ctx, 16, 2);
    nk_label(ctx, name, NK_TEXT_LEFT);
    nk_label_colored(ctx, value, NK_TEXT_LEFT, color);
}

static void draw_text(struct nk_context* ctx, float height, const char* text)
{
    nk_layout_row_dynamic(ctx, height, 1);
    nk_label_wrap(ctx, text);
}

/*
 * Navigation, drawn at the TOP of every page, so Back / Fail / Pass sit in the
 * same spot whatever the page's length (and a short frontend window never cuts
 * them off). Buttons first, then the hold-to-pass/fail progress.
 */
static void draw_navigation(CoreState* core, struct nk_context* ctx)
{
    Pages* pages = &core->pages;
    unsigned start_hold = core->input.start_held_frames;
    unsigned select_hold = core->input.select_held_frames;

    if (pages->waiting_for_release)
    {
        start_hold = 0;
        select_hold = 0;
    }

    nk_layout_row_dynamic(ctx, 28, 3);
    if (nk_button_label(ctx, "Back (L)"))
    {
        previous_page(pages);
    }
    if (pages->page == PAGE_SUMMARY)
    {
        nk_label(ctx, "", NK_TEXT_LEFT);   /* empty middle cell */
        if (nk_button_label(ctx, "Finish"))
        {
            finish_page(core, RESULT_PASS);
        }
    }
    else
    {
        if (nk_button_label(ctx, "Fail"))
        {
            finish_page(core, RESULT_FAIL);
        }
        if (nk_button_label(ctx, "Pass"))
        {
            finish_page(core, RESULT_PASS);
        }
    }

    nk_layout_row_dynamic(ctx, 18, 4);
    nk_labelf(ctx, NK_TEXT_LEFT, "Hold Start to %s:", pages->page == PAGE_SUMMARY ? "finish" : "pass");
    nk_prog(ctx, start_hold, INPUT_HOLD_FRAMES, nk_false);
    if (pages->page != PAGE_SUMMARY)
    {
        nk_label(ctx, "Hold Select to fail:", NK_TEXT_LEFT);
        nk_prog(ctx, select_hold, INPUT_HOLD_FRAMES, nk_false);
    }

    /* A thin gap between the navigation and the page. */
    nk_layout_row_dynamic(ctx, 6, 1);
    nk_label(ctx, "", NK_TEXT_LEFT);   /* not nk_spacing(): see page_video() */
}

/* ---------------------------------------------------------------------------
 * Pages
 * ------------------------------------------------------------------------- */

static const char* yes_no(bool value)
{
    return value ? "yes" : "no";
}

static void page_environment(CoreState* core, struct nk_context* ctx)
{
    const EnvironmentReport* report = &core->report;
    char text[TEXT_FIELD_SIZE + 32];

    draw_text(ctx, 32, "What the frontend answered to this core's environment calls. "
                       "Red = against the libretro spec. Grey = optional, informational.");

    draw_row(ctx, "GET_LOG_INTERFACE", yes_no(report->log_interface), report->log_interface ? color_good() : color_info());
    draw_row(ctx, "SET_SUPPORT_NO_GAME", report->support_no_game_accepted ? "accepted" : "refused",
             report->support_no_game_accepted ? color_good() : color_bad());

    draw_row(ctx, "Pixel format 0RGB1555", report->format_0rgb1555_accepted ? "accepted" : "REFUSED (must be accepted)",
             report->format_0rgb1555_accepted ? color_good() : color_bad());
    draw_row(ctx, "Pixel format RGB565", report->format_rgb565_accepted ? "accepted" : "refused",
             report->format_rgb565_accepted ? color_good() : color_info());
    draw_row(ctx, "Pixel format XRGB8888", report->format_xrgb8888_accepted ? "accepted" : "refused",
             report->format_xrgb8888_accepted ? color_good() : color_info());
    draw_row(ctx, "Pixel format in use", video_pixel_format_name(core->framebuffer.format), color_info());

    snprintf(text, sizeof(text), "%u", report->options.version);
    draw_row(ctx, "GET_CORE_OPTIONS_VERSION", report->options.version_answered ? text : "no answer (0 assumed)", color_info());
    snprintf(text, sizeof(text), "%s: %s",
             report->options.api_used == 2 ? "SET_CORE_OPTIONS_V2" :
             report->options.api_used == 1 ? "SET_CORE_OPTIONS (v1)" : "SET_VARIABLES (v0)",
             report->options.accepted ? "accepted" : "REFUSED");
    /* A frontend that reports a version must accept that version's call. */
    draw_row(ctx, "Core options sent as", text,
             report->options.accepted ? color_good() : (report->options.version_answered ? color_bad() : color_info()));
    draw_row(ctx, "  libretro_test_pixel_format",
             core->options.pixel_format_answered ? core->options.pixel_format : "no answer (default used)", color_info());
    snprintf(text, sizeof(text), "%u", core->options.width);
    draw_row(ctx, "  libretro_test_width", core->options.width_answered ? text : "no answer (640 used)", color_info());

    draw_row(ctx, "GET_CAN_DUPE", report->can_dupe_answered ? yes_no(report->can_dupe) : "no answer", color_info());
    draw_row(ctx, "GET_INPUT_BITMASKS", yes_no(report->input_bitmasks), color_info());

    snprintf(text, sizeof(text), "%u", report->language);
    draw_row(ctx, "GET_LANGUAGE", report->language_answered ? text : "no answer", color_info());

    snprintf(text, sizeof(text), "%d (bit0 video, bit1 audio)", report->audio_video_enable);
    draw_row(ctx, "GET_AUDIO_VIDEO_ENABLE", report->audio_video_enable_answered ? text : "no answer", color_info());

    draw_row(ctx, "GET_SYSTEM_DIRECTORY", report->system_directory_answered ? report->system_directory : "no answer", color_info());
    draw_row(ctx, "GET_SAVE_DIRECTORY", report->save_directory_answered ? report->save_directory : "no answer", color_info());
    draw_row(ctx, "GET_LIBRETRO_PATH", report->libretro_path_answered ? report->libretro_path : "no answer", color_info());
}

/* Returns where the test pattern goes; the caller draws it after the UI is rendered. */
static struct nk_rect page_video(CoreState* core, struct nk_context* ctx)
{
    struct nk_rect pattern_area;
    char text[256];

    snprintf(text, sizeof(text),
             "Pixel format %s, %ux%u, pitch %u bytes (wider than the image on purpose).",
             video_pixel_format_name(core->framebuffer.format), core->framebuffer.width,
             core->framebuffer.height, (unsigned)core->framebuffer.pitch);
    draw_text(ctx, 18, text);
    snprintf(text, sizeof(text), "Timing: %u fps, %u audio frames per retro_run (48000 Hz).", core->fps,
             core->audio.frames_per_run);
    draw_text(ctx, 18, text);
    if (core->report.av_info_changes > 0)
    {
        snprintf(text, sizeof(text), "Height / frame rate changed %u time(s); last SET_SYSTEM_AV_INFO %s.",
                 core->report.av_info_changes, core->report.av_info_accepted ? "accepted" : "REFUSED");
        draw_text(ctx, 18, text);
    }
    if (core->report.geometry_changes > 0)
    {
        snprintf(text, sizeof(text), "Width option changed %u time(s); last SET_GEOMETRY %s. Text must not look squeezed or stretched.",
                 core->report.geometry_changes, core->report.geometry_accepted ? "accepted" : "REFUSED");
        draw_text(ctx, 18, text);
    }
    draw_text(ctx, 70, "Expected: eight solid color bars (white, yellow, cyan, green, magenta, red, blue, black), "
                       "then red, green, blue and grey ramps from dark to bright. In XRGB8888 the ramps are smooth; "
                       "16-bit formats show 32 (or 64 for green in RGB565) visible steps. "
                       "A slanted or torn image means the frontend ignores the pitch.");

    /* nk_widget() reserves one cell and returns its rectangle; nothing is drawn in it.
       (nk_spacing() would fill the row and make Nuklear open a second empty row.) */
    nk_layout_row_dynamic(ctx, 220, 1);
    nk_widget(&pattern_area, ctx);
    return pattern_area;
}

static void draw_video_pattern(Framebuffer* framebuffer, struct nk_rect area)
{
    static const uint8_t bars[8][3] = {
        { 255, 255, 255 }, { 255, 255, 0 }, { 0, 255, 255 }, { 0, 255, 0 },
        { 255, 0, 255 },   { 255, 0, 0 },   { 0, 0, 255 },   { 0, 0, 0 },
    };
    int x = (int)area.x;
    int y = (int)area.y;
    int width = (int)area.w;
    int bars_height = (int)area.h / 2;
    int ramp_height = ((int)area.h - bars_height) / 4;
    int index = 0;
    int column = 0;

    for (index = 0; index < 8; index++)
    {
        int bar_x = x + width * index / 8;
        int bar_end = x + width * (index + 1) / 8;
        video_fill_rect(framebuffer, bar_x, y, bar_end - bar_x, bars_height,
                        bars[index][0], bars[index][1], bars[index][2]);
    }

    for (column = 0; column < width; column++)
    {
        uint8_t level = (uint8_t)(column * 255 / (width - 1));
        int ramp_y = y + bars_height;
        video_fill_rect(framebuffer, x + column, ramp_y,                   1, ramp_height, level, 0, 0);
        video_fill_rect(framebuffer, x + column, ramp_y + ramp_height,     1, ramp_height, 0, level, 0);
        video_fill_rect(framebuffer, x + column, ramp_y + ramp_height * 2, 1, ramp_height, 0, 0, level);
        video_fill_rect(framebuffer, x + column, ramp_y + ramp_height * 3, 1, ramp_height, level, level, level);
    }
}

static void page_input(CoreState* core, struct nk_context* ctx)
{
    const InputState* input = &core->input;
    unsigned id = 0;

    draw_text(ctx, 34, "Press every button on controller 1: each name lights up while held. "
                       "Move both sticks. Click or touch to move the pointer.");

    nk_layout_row_dynamic(ctx, 20, 8);
    for (id = 0; id < 16; id++)
    {
        bool down = input_is_down(input, id);
        nk_label_colored(ctx, input_button_name(id), NK_TEXT_CENTERED, down ? color_pressed() : color_info());
    }

    nk_layout_row_dynamic(ctx, 18, 1);
    nk_labelf(ctx, NK_TEXT_LEFT, "Left stick:  x %6d  y %6d", input->analog[0][0], input->analog[0][1]);
    nk_labelf(ctx, NK_TEXT_LEFT, "Right stick: x %6d  y %6d", input->analog[1][0], input->analog[1][1]);

    if (input->pointer_valid)
    {
        nk_labelf(ctx, NK_TEXT_LEFT, "Pointer: %d, %d %s", input->pointer_x, input->pointer_y,
                  input->pointer_pressed ? "(pressed)" : "");
    }
    else
    {
        nk_label_colored(ctx, "Pointer: no data yet", NK_TEXT_LEFT, color_info());
    }

    if (!input->bitmask_supported)
    {
        nk_label_colored(ctx, "Input bitmasks: not supported by the frontend (optional)", NK_TEXT_LEFT, color_info());
    }
    else if (input->bitmask_mismatch)
    {
        nk_label_colored(ctx, "Input bitmasks: MISMATCH with per-button reads (frontend bug)", NK_TEXT_LEFT, color_bad());
    }
    else
    {
        nk_label_colored(ctx, "Input bitmasks: supported, matches per-button reads", NK_TEXT_LEFT, color_good());
    }
}

static void page_audio(CoreState* core, struct nk_context* ctx)
{
    draw_text(ctx, 34, "You should hear a steady 440 Hz tone in the selected channel(s): "
                       "no clicks, no crackle, no drift in pitch.");

    nk_layout_row_dynamic(ctx, 18, 1);
    nk_labelf(ctx, NK_TEXT_LEFT, "Channels: %s   (D-pad Left/Right)", audio_channels_name(core->audio.channels));
    nk_labelf(ctx, NK_TEXT_LEFT, "Callback: %s   (D-pad Up)",
              core->audio.use_single_sample_callback ? "retro_audio_sample (one frame at a time)"
                                                     : "retro_audio_sample_batch");
    nk_labelf(ctx, NK_TEXT_LEFT, "Format: 48000 Hz stereo, %u frames per retro_run (%u fps)", core->audio.frames_per_run,
              core->options.fps);
}

static void page_sram(CoreState* core, struct nk_context* ctx, bool entered)
{
    char before[SRAM_TEXT_SIZE];
    char text[SRAM_TEXT_SIZE + 64];
    struct nk_color color = color_info();

    draw_text(ctx, 50, "Type a short text below (in the test frontend: turn on game focus with Scroll Lock or F12 first). "
                       "Then quit, start the core again and come back here: the frontend must have saved this "
                       "save RAM and loaded it back before the first frame.");

    switch (core->sram_state)
    {
        case SRAM_RESTORED:
            snprintf(text, sizeof(text), "restored, checksum OK: \"%s\"", core->sram.text);
            color = color_good();
            break;
        case SRAM_CORRUPTED:
            snprintf(text, sizeof(text), "CORRUPTED: wrong magic, length or checksum");
            color = color_bad();
            break;
        default:
            snprintf(text, sizeof(text), "empty (first run, or the frontend didn't load it)");
            break;
    }
    draw_row(ctx, "Save RAM at load", text, color);
    draw_row(ctx, "SET_KEYBOARD_CALLBACK", core->report.keyboard_callback_accepted ? "accepted" : "refused (can't type)",
             core->report.keyboard_callback_accepted ? color_good() : color_bad());
    if (core->keys_dropped > 0)
    {
        snprintf(text, sizeof(text), "%u key events dropped (queue full)", core->keys_dropped);
        draw_row(ctx, "Keyboard", text, color_bad());
    }

    nk_layout_row_dynamic(ctx, 26, 1);
    if (entered)
    {
        nk_edit_focus(ctx, NK_EDIT_FIELD);   /* ready to type without clicking first */
    }
    memcpy(before, core->sram.text, sizeof(before));
    nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, core->sram.text, SRAM_TEXT_SIZE, nk_filter_ascii);
    if (memcmp(before, core->sram.text, sizeof(before)) != 0)
    {
        sram_seal(&core->sram);   /* the save RAM is valid as soon as anything was typed */
    }
}

static void page_savestate(CoreState* core, struct nk_context* ctx)
{
    draw_text(ctx, 66, "A state holds the frame counter, the page you're on and every page's result. "
                       "Save a state here, wait a few seconds, load it: the frame counter must jump back. "
                       "Then save on another page, come back here and load: you must land on that page, "
                       "with the results it had when you saved.");

    nk_layout_row_dynamic(ctx, 18, 1);
    nk_labelf(ctx, NK_TEXT_LEFT, "Frame counter: %llu", (unsigned long long)core->frame_count);
    if (core->states.saved > 0)
    {
        nk_labelf(ctx, NK_TEXT_LEFT, "States saved: %u, last at frame %llu on page %s", core->states.saved,
                  (unsigned long long)core->states.saved_frame, pages_title(core->states.saved_page));
    }
    else
    {
        nk_label_colored(ctx, "States saved: none yet", NK_TEXT_LEFT, color_info());
    }
    if (core->load_state_count > 0)
    {
        nk_labelf_colored(ctx, NK_TEXT_LEFT, color_good(), "States loaded: %u, last back to frame %llu on page %s",
                          core->load_state_count, (unsigned long long)core->states.loaded_frame,
                          pages_title(core->states.loaded_page));
    }
    else
    {
        nk_label_colored(ctx, "States loaded: none yet", NK_TEXT_LEFT, color_info());
    }
}

static void page_summary(CoreState* core, struct nk_context* ctx)
{
    unsigned page = 0;

    draw_text(ctx, 18, "Results. Finishing asks the frontend to shut the core down (RETRO_ENVIRONMENT_SHUTDOWN).");
    for (page = 0; page < PAGE_SUMMARY; page++)
    {
        TestResult result = core->pages.results[page];
        draw_row(ctx, pages_title((PageId)page), result_name(result), result_color(result));
    }

    if (!core->pages.summary_logged)
    {
        core->pages.summary_logged = true;
        for (page = 0; page < PAGE_SUMMARY; page++)
        {
            core_log(core, RETRO_LOG_INFO, "summary: %-12s %s\n", pages_title((PageId)page),
                     result_name(core->pages.results[page]));
        }
    }
}

/* ---------------------------------------------------------------------------
 * Frame
 * ------------------------------------------------------------------------- */

/* Key events queued by the keyboard callback -> Nuklear. Typed characters
   (printable ASCII: the font has nothing else) and the editing keys. */
static void feed_keyboard(CoreState* core, struct nk_context* ctx)
{
    unsigned i = 0;

    for (i = 0; i < core->key_queue_count; i++)
    {
        const KeyEvent* event = &core->key_queue[i];
        int down = event->down ? nk_true : nk_false;

        if (event->down && event->character >= 32 && event->character < 127)
        {
            nk_input_unicode(ctx, (nk_rune)event->character);
        }
        switch (event->keycode)
        {
            case RETROK_BACKSPACE: nk_input_key(ctx, NK_KEY_BACKSPACE, down); break;
            case RETROK_DELETE:    nk_input_key(ctx, NK_KEY_DEL, down); break;
            case RETROK_LEFT:      nk_input_key(ctx, NK_KEY_LEFT, down); break;
            case RETROK_RIGHT:     nk_input_key(ctx, NK_KEY_RIGHT, down); break;
            case RETROK_HOME:      nk_input_key(ctx, NK_KEY_TEXT_LINE_START, down); break;
            case RETROK_END:       nk_input_key(ctx, NK_KEY_TEXT_LINE_END, down); break;
            case RETROK_RETURN:    nk_input_key(ctx, NK_KEY_ENTER, down); break;
            default:               break;
        }
    }
    core->key_queue_count = 0;
}

void pages_run_frame(CoreState* core)
{
    Pages* pages = &core->pages;
    struct nk_context* ctx = video_ui_context(&core->framebuffer);
    struct nk_rect pattern_area = nk_rect(0, 0, 0, 0);
    bool draw_pattern = false;
    bool entered = false;
    char title[64];

    handle_controller(core);
    core->audio.playing = (pages->page == PAGE_AUDIO);
    entered = !pages->drawn_once || pages->page != pages->last_drawn_page;
    pages->last_drawn_page = pages->page;
    pages->drawn_once = true;

    nk_input_begin(ctx);
    if (core->input.pointer_valid)
    {
        nk_input_motion(ctx, core->input.pointer_x, core->input.pointer_y);
        nk_input_button(ctx, NK_BUTTON_LEFT, core->input.pointer_x, core->input.pointer_y,
                        core->input.pointer_pressed ? nk_true : nk_false);
    }
    feed_keyboard(core, ctx);
    nk_input_end(ctx);

    snprintf(title, sizeof(title), "Test %d/%d: %s", (int)pages->page + 1, (int)PAGE_COUNT, pages_title(pages->page));

    /* Same window name every frame so Nuklear keeps its state; the visible title changes. */
    if (nk_begin_titled(ctx, "pages", title, nk_rect(0, 0, (float)core->framebuffer.width, (float)core->framebuffer.height),
                        NK_WINDOW_BORDER | NK_WINDOW_TITLE))
    {
        draw_navigation(core, ctx);
        switch (pages->page)
        {
            case PAGE_ENVIRONMENT: page_environment(core, ctx); break;
            case PAGE_VIDEO:       pattern_area = page_video(core, ctx); draw_pattern = true; break;
            case PAGE_INPUT:       page_input(core, ctx); break;
            case PAGE_AUDIO:       page_audio(core, ctx); break;
            case PAGE_SRAM:        page_sram(core, ctx, entered); break;
            case PAGE_SAVESTATE:   page_savestate(core, ctx); break;
            case PAGE_SUMMARY:     page_summary(core, ctx); break;
            default:               break;
        }
    }
    nk_end(ctx);

    video_ui_render(&core->framebuffer);

    /* Drawn straight into the framebuffer after the UI, in the area the page reserved. */
    if (draw_pattern)
    {
        draw_video_pattern(&core->framebuffer, pattern_area);
    }
}

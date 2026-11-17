#include "pages.h"

#include <cstdio>
#include <cstring>

#include "imgui.h"

#include "core.h"
#include "pad_drawing.h"
#include "ui.h"

/* ---------------------------------------------------------------------------
 * Colors
 * ------------------------------------------------------------------------- */

static ImVec4 color_good()    { return ImVec4(90.0f / 255.0f, 210.0f / 255.0f, 120.0f / 255.0f, 1.0f); }
static ImVec4 color_bad()     { return ImVec4(235.0f / 255.0f, 90.0f / 255.0f, 90.0f / 255.0f, 1.0f); }
static ImVec4 color_info()    { return ImVec4(175.0f / 255.0f, 175.0f / 255.0f, 190.0f / 255.0f, 1.0f); }
static ImVec4 color_pressed() { return ImVec4(255.0f / 255.0f, 220.0f / 255.0f, 80.0f / 255.0f, 1.0f); }

/* ---------------------------------------------------------------------------
 * Names
 * ------------------------------------------------------------------------- */

const char* pages_title(PageId page)
{
    switch (page)
    {
        case PAGE_CALLBACKS:   return "Callbacks";
        case PAGE_VIDEO:       return "Video";
        case PAGE_AUDIO:       return "Audio";
        case PAGE_RETROPAD:    return "RetroPad basic";
        case PAGE_CONTROLLERS: return "Controllers";
        case PAGE_KEYBOARD:    return "Keyboard";
        case PAGE_MOUSE:       return "Mouse & pointer";
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

static ImVec4 result_color(TestResult result)
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
            core->environment(RETRO_ENVIRONMENT_SHUTDOWN, nullptr);
        }
        return;
    }

    pages->results[pages->page] = result;
    core_log(core, RETRO_LOG_INFO, "%s: %s\n", pages_title(pages->page), result_name(result));
    pages->page = static_cast<PageId>(pages->page + 1);
}

static void previous_page(Pages* pages)
{
    if (pages->page > PAGE_CALLBACKS)
    {
        pages->page = static_cast<PageId>(pages->page - 1);
    }
}

/* To the next page without a result (it stays as it was). */
static void next_page(Pages* pages)
{
    if (pages->page < PAGE_SUMMARY)
    {
        pages->page = static_cast<PageId>(pages->page + 1);
    }
}

/* Joypad controls. Mouse/pointer clicks are handled by the buttons in draw_navigation(). */
static void handle_controller(CoreState* core)
{
    Pages* pages = &core->pages;
    const InputState* input = &core->input;

    /* After a hold did something: nothing more until those buttons are let go. */
    if (pages->waiting_for_release)
    {
        if (!input_is_down(input, RETRO_DEVICE_ID_JOYPAD_START) && !input_is_down(input, RETRO_DEVICE_ID_JOYPAD_SELECT) &&
            !input_is_down(input, RETRO_DEVICE_ID_JOYPAD_L) && !input_is_down(input, RETRO_DEVICE_ID_JOYPAD_R) &&
            !input_is_down(input, RETRO_DEVICE_ID_JOYPAD_X))
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
    if (input->l_held_frames >= INPUT_PAGE_HOLD_FRAMES)
    {
        pages->waiting_for_release = true;
        previous_page(pages);
        return;
    }
    if (input->r_held_frames >= INPUT_PAGE_HOLD_FRAMES)
    {
        pages->waiting_for_release = true;
        next_page(pages);
        return;
    }
    /* Hold X: the page's setting, for a frontend without a mouse (the combos need one). */
    if (input->x_held_frames >= INPUT_PAGE_HOLD_FRAMES &&
        (pages->page == PAGE_RETROPAD || pages->page == PAGE_CONTROLLERS || pages->page == PAGE_KEYBOARD))
    {
        pages->waiting_for_release = true;
        if (pages->page == PAGE_RETROPAD)
        {
            pages->retropad_port = (pages->retropad_port + 1) % CONTROLLER_PORTS;
        }
        else if (pages->page == PAGE_CONTROLLERS)
        {
            pages->controllers_port = (pages->controllers_port + 1) % CONTROLLER_PORTS;
        }
        else
        {
            core->keyboard.mode = static_cast<KeyboardMode>((core->keyboard.mode + 1) % KEYBOARD_MODE_COUNT);
        }
        return;
    }

    /* Page-specific controls. */
    if (pages->page == PAGE_AUDIO)
    {
        if (input_was_pressed(input, RETRO_DEVICE_ID_JOYPAD_RIGHT))
        {
            core->audio.channels = static_cast<AudioChannels>((core->audio.channels + 1) % AUDIO_CHANNELS_COUNT);
        }
        if (input_was_pressed(input, RETRO_DEVICE_ID_JOYPAD_LEFT))
        {
            core->audio.channels =
                static_cast<AudioChannels>((core->audio.channels + AUDIO_CHANNELS_COUNT - 1) % AUDIO_CHANNELS_COUNT);
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

/* Name on the left, its value in a second column (wrapped if it is long). */
static void draw_row(const char* name, const char* value, ImVec4 color)
{
    float column = ImGui::GetWindowContentRegionMax().x * 0.42f;
    ImGui::TextUnformatted(name);
    ImGui::SameLine(column);
    ImGui::PushStyleColor(ImGuiCol_Text, color);
    ImGui::TextWrapped("%s", value);
    ImGui::PopStyleColor();
}

static void draw_text(const char* text)
{
    ImGui::TextWrapped("%s", text);
}

static void draw_colored(ImVec4 color, const char* text)
{
    ImGui::TextColored(color, "%s", text);
}

/* The navigation's height: a separator, the holds' bars, the buttons. */
static float navigation_height()
{
    const ImGuiStyle& style = ImGui::GetStyle();
    return 1.0f + ImGui::GetTextLineHeight() + ImGui::GetFrameHeight() + 4.0f + style.ItemSpacing.y * 3.0f;
}

/*
 * Navigation, at the BOTTOM of every page, so Back / Fail / Pass / Next sit in
 * the same spot whatever the page's length: how far each hold has come, then
 * the buttons.
 */
static void draw_navigation(CoreState* core)
{
    Pages* pages = &core->pages;
    const InputState* input = &core->input;
    const ImGuiStyle& style = ImGui::GetStyle();
    float quarter = (ImGui::GetContentRegionAvail().x - style.ItemSpacing.x * 3.0f) / 4.0f;
    ImVec2 button(quarter, ImGui::GetFrameHeight() + 4.0f);
    bool waiting = pages->waiting_for_release;

    ImGui::Separator();   /* between the page and the navigation */

    /* How far each hold has come. */
    struct Hold
    {
        const char* label;
        unsigned    frames;
        unsigned    needed;
    };
    Hold holds[4] = {
        {pages->page == PAGE_SUMMARY ? "Start: finish" : "Start: pass", input->start_held_frames, INPUT_HOLD_FRAMES},
        {"Select: fail", pages->page == PAGE_SUMMARY ? 0u : input->select_held_frames, INPUT_HOLD_FRAMES},
        {"L: back", input->l_held_frames, INPUT_PAGE_HOLD_FRAMES},
        {"R: next", input->r_held_frames, INPUT_PAGE_HOLD_FRAMES},
    };
    float bar_height = ImGui::GetTextLineHeight();
    for (int h = 0; h < 4; h++)
    {
        if (h > 0)
        {
            ImGui::SameLine(style.WindowPadding.x + static_cast<float>(h) * (quarter + style.ItemSpacing.x));
        }
        float fraction = waiting ? 0.0f : static_cast<float>(holds[h].frames) / static_cast<float>(holds[h].needed);
        ImGui::ProgressBar(fraction > 1.0f ? 1.0f : fraction, ImVec2(quarter, bar_height), holds[h].label);
    }

    if (ImGui::Button("Back (hold L)", button))
    {
        previous_page(pages);
    }
    ImGui::SameLine();
    if (pages->page == PAGE_SUMMARY)
    {
        ImGui::Dummy(button);   /* empty middle cells */
        ImGui::SameLine();
        ImGui::Dummy(button);
        ImGui::SameLine();
        if (ImGui::Button("Finish", button))
        {
            finish_page(core, RESULT_PASS);
        }
    }
    else
    {
        if (ImGui::Button("Fail", button))
        {
            finish_page(core, RESULT_FAIL);
        }
        ImGui::SameLine();
        if (ImGui::Button("Pass", button))
        {
            finish_page(core, RESULT_PASS);
        }
        ImGui::SameLine();
        if (ImGui::Button("Next (hold R)", button))
        {
            next_page(pages);
        }
    }
}

/* ---------------------------------------------------------------------------
 * Pages
 * ------------------------------------------------------------------------- */

static const char* yes_no(bool value)
{
    return value ? "yes" : "no";
}

static void page_callbacks(CoreState* core)
{
    const EnvironmentReport* report = &core->report;
    char text[TEXT_FIELD_SIZE + 32];

    draw_text("What the frontend answered to this core's environment calls. "
              "Red = against the libretro spec. Grey = optional, informational.");

    draw_row("GET_LOG_INTERFACE", yes_no(report->log_interface), report->log_interface ? color_good() : color_info());
    draw_row("SET_SUPPORT_NO_GAME", report->support_no_game_accepted ? "accepted" : "refused",
             report->support_no_game_accepted ? color_good() : color_bad());

    draw_row("Pixel format 0RGB1555", report->format_0rgb1555_accepted ? "accepted" : "REFUSED (must be accepted)",
             report->format_0rgb1555_accepted ? color_good() : color_bad());
    draw_row("Pixel format RGB565", report->format_rgb565_accepted ? "accepted" : "refused",
             report->format_rgb565_accepted ? color_good() : color_info());
    draw_row("Pixel format XRGB8888", report->format_xrgb8888_accepted ? "accepted" : "refused",
             report->format_xrgb8888_accepted ? color_good() : color_info());
    draw_row("Pixel format in use", video_pixel_format_name(core->framebuffer.format), color_info());

    std::snprintf(text, sizeof(text), "%u", report->options.version);
    draw_row("GET_CORE_OPTIONS_VERSION", report->options.version_answered ? text : "no answer (0 assumed)", color_info());
    std::snprintf(text, sizeof(text), "%s: %s",
                  report->options.api_used == 2 ? "SET_CORE_OPTIONS_V2" :
                  report->options.api_used == 1 ? "SET_CORE_OPTIONS (v1)" : "SET_VARIABLES (v0)",
                  report->options.accepted ? "accepted" : "REFUSED");
    /* A frontend that reports a version must accept that version's call. */
    draw_row("Core options sent as", text,
             report->options.accepted ? color_good() : (report->options.version_answered ? color_bad() : color_info()));
    draw_row("  libretro_test_pixel_format",
             core->options.pixel_format_answered ? core->options.pixel_format : "no answer (default used)", color_info());
    std::snprintf(text, sizeof(text), "%u", core->options.width);
    draw_row("  libretro_test_width", core->options.width_answered ? text : "no answer (640 used)", color_info());

    draw_row("GET_CAN_DUPE", report->can_dupe_answered ? yes_no(report->can_dupe) : "no answer", color_info());
    draw_row("GET_INPUT_BITMASKS", yes_no(report->input_bitmasks), color_info());

    std::snprintf(text, sizeof(text), "%u", report->language);
    draw_row("GET_LANGUAGE", report->language_answered ? text : "no answer", color_info());

    std::snprintf(text, sizeof(text), "%d (bit0 video, bit1 audio)", report->audio_video_enable);
    draw_row("GET_AUDIO_VIDEO_ENABLE", report->audio_video_enable_answered ? text : "no answer", color_info());

    draw_row("GET_SYSTEM_DIRECTORY", report->system_directory_answered ? report->system_directory : "no answer", color_info());
    draw_row("GET_SAVE_DIRECTORY", report->save_directory_answered ? report->save_directory : "no answer", color_info());
    draw_row("GET_LIBRETRO_PATH", report->libretro_path_answered ? report->libretro_path : "no answer", color_info());
}

/* Where the video page's test pattern goes, in framebuffer pixels. */
struct PatternArea
{
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

/* Returns where the test pattern goes; the caller draws it after the UI is rendered. */
static PatternArea page_video(CoreState* core)
{
    char text[256];

    std::snprintf(text, sizeof(text), "Pixel format %s, %ux%u, pitch %u bytes (wider than the image on purpose).",
                  video_pixel_format_name(core->framebuffer.format), core->framebuffer.width, core->framebuffer.height,
                  static_cast<unsigned>(core->framebuffer.pitch));
    draw_text(text);
    std::snprintf(text, sizeof(text), "Timing: %u fps, %.1f audio frames per retro_run (%u Hz).", core->fps,
                  static_cast<double>(audio_frames_per_run(&core->audio)), core->audio.sample_rate);
    draw_text(text);
    if (core->report.av_info_changes > 0)
    {
        std::snprintf(text, sizeof(text), "Height / frame rate changed %u time(s); last SET_SYSTEM_AV_INFO %s.",
                      core->report.av_info_changes, core->report.av_info_accepted ? "accepted" : "REFUSED");
        draw_text(text);
    }
    if (core->report.geometry_changes > 0)
    {
        std::snprintf(text, sizeof(text),
                      "Width option changed %u time(s); last SET_GEOMETRY %s. Text must not look squeezed or stretched.",
                      core->report.geometry_changes, core->report.geometry_accepted ? "accepted" : "REFUSED");
        draw_text(text);
    }
    draw_text("Expected: eight solid color bars (white, yellow, cyan, green, magenta, red, blue, black), "
              "then red, green, blue and grey ramps from dark to bright. In XRGB8888 the ramps are smooth; "
              "16-bit formats show 32 (or 64 for green in RGB565) visible steps. "
              "A slanted or torn image means the frontend ignores the pitch.");

    /* Reserved, drawn into the framebuffer after the UI. */
    ImVec2 size(ImGui::GetContentRegionAvail().x, 220.0f);
    ImGui::Dummy(size);
    ImVec2 corner = ImGui::GetItemRectMin();
    PatternArea area;
    area.x = static_cast<int>(corner.x);
    area.y = static_cast<int>(corner.y);
    area.width = static_cast<int>(size.x);
    area.height = static_cast<int>(size.y);
    return area;
}

static void draw_video_pattern(Framebuffer* framebuffer, PatternArea area)
{
    static const uint8_t bars[8][3] = {
        {255, 255, 255}, {255, 255, 0}, {0, 255, 255}, {0, 255, 0},
        {255, 0, 255},   {255, 0, 0},   {0, 0, 255},   {0, 0, 0},
    };
    int bars_height = area.height / 2;
    int ramp_height = (area.height - bars_height) / 4;

    for (int index = 0; index < 8; index++)
    {
        int bar_x = area.x + area.width * index / 8;
        int bar_end = area.x + area.width * (index + 1) / 8;
        video_fill_rect(framebuffer, bar_x, area.y, bar_end - bar_x, bars_height, bars[index][0], bars[index][1],
                        bars[index][2]);
    }

    for (int column = 0; column < area.width; column++)
    {
        uint8_t level = static_cast<uint8_t>(column * 255 / (area.width - 1));
        int ramp_y = area.y + bars_height;
        video_fill_rect(framebuffer, area.x + column, ramp_y,                   1, ramp_height, level, 0, 0);
        video_fill_rect(framebuffer, area.x + column, ramp_y + ramp_height,     1, ramp_height, 0, level, 0);
        video_fill_rect(framebuffer, area.x + column, ramp_y + ramp_height * 2, 1, ramp_height, 0, 0, level);
        video_fill_rect(framebuffer, area.x + column, ramp_y + ramp_height * 3, 1, ramp_height, level, level, level);
    }
}

/* PORT [ Port 1 v ]: which port a page shows. */
static void port_combo(int* port)
{
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Port");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(90.0f);
    ImGui::Combo("##port", port, "Port 1\0Port 2\0Port 3\0Port 4\0");
    ImGui::SameLine();
    draw_colored(color_info(), "(hold X: next port)");
}

static void page_retropad(CoreState* core)
{
    const ControllersState* state = &core->controllers;
    int port = core->pages.retropad_port;

    draw_text("The chosen port's controller as a plain RetroPad (RETRO_DEVICE_JOYPAD), whatever type is plugged in: "
              "press every button and move both sticks, each part lights up.");
    port_combo(&core->pages.retropad_port);
    port = core->pages.retropad_port;

    ImGui::Spacing();
    pad_draw(PadStyle::retropad, state->buttons[port], state->analog[port], 380.0f);
    ImGui::Text("Left stick:  x %6d  y %6d      Right stick: x %6d  y %6d", state->analog[port][0][0],
                state->analog[port][0][1], state->analog[port][1][0], state->analog[port][1][1]);

    if (!state->bitmask_supported)
    {
        draw_colored(color_info(), "Input bitmasks: not supported by the frontend (optional)");
    }
    else if (state->bitmask_mismatch[port])
    {
        draw_colored(color_bad(), "Input bitmasks: MISMATCH with per-button reads (frontend bug)");
    }
    else
    {
        draw_colored(color_good(), "Input bitmasks: supported, matches per-button reads");
    }
}

static void page_controllers(CoreState* core)
{
    const ControllersState* state = &core->controllers;
    char text[160];

    draw_text("Each port offers four controller types, each with its own buttons and names "
              "(SET_CONTROLLER_INFO, SET_INPUT_DESCRIPTORS). Plug a type into the port in the frontend, "
              "then press every button: only that type's buttons light up.");
    port_combo(&core->pages.controllers_port);
    int port = core->pages.controllers_port;
    unsigned device = state->device[port];
    std::snprintf(text, sizeof(text), "Plugged in: %s%s", controllers_device_name(device),
                  state->unknown_device[port] ? " (never offered: frontend bug)" : "");
    draw_colored(state->unknown_device[port] ? color_bad() : color_pressed(), text);

    ImGui::Spacing();
    static const int16_t STILL[2][2] = {{0, 0}, {0, 0}};
    bool sticks = controllers_has_sticks(device);
    pad_draw(pad_style_for_device(device), static_cast<uint16_t>(state->buttons[port] & controllers_button_mask(device)),
             sticks ? state->analog[port] : STILL, 380.0f);

    draw_colored(state->info_accepted ? color_good() : color_bad(),
                 state->info_accepted ? "SET_CONTROLLER_INFO: accepted" : "SET_CONTROLLER_INFO: not accepted");
    std::snprintf(text, sizeof(text), "SET_INPUT_DESCRIPTORS: sent %u time(s), last one %s; %u device change(s)",
                  state->descriptor_sends, state->descriptors_accepted ? "accepted" : "not accepted", state->device_changes);
    draw_colored(state->descriptors_accepted ? color_good() : color_bad(), text);
}

static void page_keyboard(CoreState* core)
{
    KeyboardState* keyboard = &core->keyboard;
    char name[32];
    char text[128];

    draw_text("libretro gives a core the keyboard two ways: the frontend calls the core on each key event "
              "(SET_KEYBOARD_CALLBACK), or the core asks for every key each frame (RETRO_DEVICE_KEYBOARD). "
              "Pick one and press keys (in the test frontend: game focus first).");
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Mode");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(260.0f);
    int mode = static_cast<int>(keyboard->mode);
    if (ImGui::Combo("##keyboard_mode", &mode, "Callback (SET_KEYBOARD_CALLBACK)\0Polled (RETRO_DEVICE_KEYBOARD)\0"))
    {
        keyboard->mode = static_cast<KeyboardMode>(mode);
    }
    ImGui::SameLine();
    draw_colored(color_info(), "(hold X: the other mode)");
    ImGui::Spacing();

    if (keyboard->mode == KEYBOARD_CALLBACK)
    {
        draw_row("SET_KEYBOARD_CALLBACK", core->report.keyboard_callback_accepted ? "accepted" : "refused (no events)",
                 core->report.keyboard_callback_accepted ? color_good() : color_bad());
        std::snprintf(text, sizeof(text), "%u", keyboard->events);
        draw_row("Events received", text, keyboard->events > 0 ? color_good() : color_info());
        /* As many of the newest as the page has room for. */
        for (unsigned i = 0; i < keyboard->log_count && ImGui::GetContentRegionAvail().y >= ImGui::GetTextLineHeightWithSpacing(); i++)
        {
            const KeyboardLogEntry* entry = &keyboard->log[i];
            keyboard_key_name(entry->keycode, name, sizeof(name));
            char typed[8] = "";
            if (entry->character >= 32 && entry->character < 127)
            {
                std::snprintf(typed, sizeof(typed), "'%c'", static_cast<char>(entry->character));
            }
            std::snprintf(text, sizeof(text), "%-5s %-14s %-4s%s%s%s%s", entry->down ? "down" : "up", name, typed,
                          (entry->modifiers & RETROKMOD_SHIFT) != 0 ? " Shift" : "",
                          (entry->modifiers & RETROKMOD_CTRL) != 0 ? " Ctrl" : "",
                          (entry->modifiers & RETROKMOD_ALT) != 0 ? " Alt" : "",
                          (entry->modifiers & RETROKMOD_META) != 0 ? " Meta" : "");
            draw_colored(i == 0 ? color_pressed() : color_info(), text);
        }
    }
    else
    {
        draw_row("Keys seen held", keyboard->polled_any ? "yes" : "none yet", keyboard->polled_any ? color_good() : color_info());
        ImGui::TextUnformatted("Held now:");
        bool first = true;
        for (unsigned key = 1; key < RETROK_LAST; key++)
        {
            if (!keyboard->held[key])
            {
                continue;
            }
            keyboard_key_name(key, name, sizeof(name));
            if (!first)
            {
                ImGui::SameLine();
            }
            draw_colored(color_pressed(), name);
            first = false;
        }
    }
}

/* A crosshair at (x, y) in framebuffer pixels, over the whole page. */
static void draw_crosshair(int x, int y, ImU32 color)
{
    ImDrawList* draw = ImGui::GetForegroundDrawList();
    float cx = static_cast<float>(x) + 0.5f;
    float cy = static_cast<float>(y) + 0.5f;
    draw->AddLine(ImVec2(cx - 8.0f, cy), ImVec2(cx + 9.0f, cy), color);
    draw->AddLine(ImVec2(cx, cy - 8.0f), ImVec2(cx, cy + 9.0f), color);
}

static void page_mouse(CoreState* core)
{
    const InputState* input = &core->input;
    static const char* const BUTTON_NAMES[5] = {"Left", "Right", "Middle", "Button 4", "Button 5"};
    char text[96];

    draw_text("Move the mouse and click its buttons (RETRO_DEVICE_MOUSE: relative moves, added up here: the yellow "
              "cross). Touch or click for the pointer (RETRO_DEVICE_POINTER: absolute, the green cross).");
    ImGui::Spacing();

    if (input->mouse_seen)
    {
        std::snprintf(text, sizeof(text), "at %d, %d (last move %+d, %+d)", input->mouse_x, input->mouse_y, input->mouse_dx,
                      input->mouse_dy);
        draw_row("Mouse", text, color_good());
    }
    else
    {
        draw_row("Mouse", "no data yet", color_info());
    }
    ImGui::TextUnformatted("Mouse buttons");
    for (unsigned b = 0; b < 5; b++)
    {
        ImGui::SameLine(b == 0 ? ImGui::GetWindowContentRegionMax().x * 0.42f : 0.0f);
        draw_colored((input->mouse_buttons & (1u << b)) != 0 ? color_pressed() : color_info(), BUTTON_NAMES[b]);
    }
    std::snprintf(text, sizeof(text), "up %u, down %u", input->wheel_up, input->wheel_down);
    draw_row("Wheel clicks", text, input->wheel_up + input->wheel_down > 0 ? color_good() : color_info());

    if (input->pointer_valid)
    {
        std::snprintf(text, sizeof(text), "at %d, %d%s", input->pointer_x, input->pointer_y,
                      input->pointer_pressed ? " (pressed)" : "");
        draw_row("Pointer", text, input->pointer_pressed ? color_pressed() : color_good());
    }
    else
    {
        draw_row("Pointer", "no data yet", color_info());
    }

    if (input->mouse_seen)
    {
        draw_crosshair(input->mouse_x, input->mouse_y, IM_COL32(255, 220, 80, 255));
    }
    if (input->pointer_valid)
    {
        draw_crosshair(input->pointer_x, input->pointer_y, IM_COL32(90, 210, 120, 255));
    }
}

static void page_audio(CoreState* core)
{
    draw_text("You should hear a steady 440 Hz tone in the selected channel(s): "
              "no clicks, no crackle, no drift in pitch.");

    ImGui::Text("Channels: %s   (D-pad Left/Right)", audio_channels_name(core->audio.channels));
    ImGui::Text("Callback: %s   (D-pad Up)", core->audio.use_single_sample_callback ? "retro_audio_sample (one frame at a time)"
                                                                                     : "retro_audio_sample_batch");
    ImGui::Text("Format: %u Hz stereo, %.1f frames per retro_run (%u fps)", core->audio.sample_rate,
                static_cast<double>(audio_frames_per_run(&core->audio)), core->fps);
    ImGui::TextUnformatted("Sample rate: core option Audio > Sample rate (32000 / 44100 / 48000 / 96000)");
}

/* The save RAM's text is ASCII: nothing else is typed into it. */
static int ascii_only(ImGuiInputTextCallbackData* data)
{
    return data->EventChar >= 32 && data->EventChar < 127 ? 0 : 1;
}

static void page_sram(CoreState* core, bool entered)
{
    char before[SRAM_TEXT_SIZE];
    char text[SRAM_TEXT_SIZE + 64];
    ImVec4 color = color_info();

    draw_text("Type a short text below (in the test frontend: turn on game focus with Scroll Lock or F12 first). "
              "Then quit, start the core again and come back here: the frontend must have saved this "
              "save RAM and loaded it back before the first frame.");

    switch (core->sram_state)
    {
        case SRAM_RESTORED:
            std::snprintf(text, sizeof(text), "restored, checksum OK: \"%s\"", core->sram.text);
            color = color_good();
            break;
        case SRAM_CORRUPTED:
            std::snprintf(text, sizeof(text), "CORRUPTED: wrong magic, length or checksum");
            color = color_bad();
            break;
        default:
            std::snprintf(text, sizeof(text), "empty (first run, or the frontend didn't load it)");
            break;
    }
    draw_row("Save RAM at load", text, color);
    draw_row("SET_KEYBOARD_CALLBACK", core->report.keyboard_callback_accepted ? "accepted" : "refused (can't type)",
             core->report.keyboard_callback_accepted ? color_good() : color_bad());
    if (core->keys_dropped > 0)
    {
        std::snprintf(text, sizeof(text), "%u key events dropped (queue full)", core->keys_dropped);
        draw_row("Keyboard", text, color_bad());
    }

    if (entered)
    {
        ImGui::SetKeyboardFocusHere();   /* ready to type without clicking first */
    }
    std::memcpy(before, core->sram.text, sizeof(before));
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::InputText("##sram", core->sram.text, SRAM_TEXT_SIZE, ImGuiInputTextFlags_CallbackCharFilter, ascii_only);
    if (std::memcmp(before, core->sram.text, sizeof(before)) != 0)
    {
        sram_seal(&core->sram);   /* the save RAM is valid as soon as anything was typed */
    }
}

static void page_savestate(CoreState* core)
{
    draw_text("A state holds the frame counter, the page you're on and every page's result. "
              "Save a state here, wait a few seconds, load it: the frame counter must jump back. "
              "Then save on another page, come back here and load: you must land on that page, "
              "with the results it had when you saved.");

    ImGui::Text("Frame counter: %llu", static_cast<unsigned long long>(core->frame_count));
    if (core->states.saved > 0)
    {
        ImGui::Text("States saved: %u, last at frame %llu on page %s", core->states.saved,
                    static_cast<unsigned long long>(core->states.saved_frame), pages_title(core->states.saved_page));
    }
    else
    {
        draw_colored(color_info(), "States saved: none yet");
    }
    if (core->load_state_count > 0)
    {
        ImGui::TextColored(color_good(), "States loaded: %u, last back to frame %llu on page %s", core->load_state_count,
                           static_cast<unsigned long long>(core->states.loaded_frame), pages_title(core->states.loaded_page));
    }
    else
    {
        draw_colored(color_info(), "States loaded: none yet");
    }
}

static void page_summary(CoreState* core)
{
    draw_text("Results. Finishing asks the frontend to shut the core down (RETRO_ENVIRONMENT_SHUTDOWN).");
    for (unsigned page = 0; page < PAGE_SUMMARY; page++)
    {
        TestResult result = core->pages.results[page];
        draw_row(pages_title(static_cast<PageId>(page)), result_name(result), result_color(result));
    }

    if (!core->pages.summary_logged)
    {
        core->pages.summary_logged = true;
        for (unsigned page = 0; page < PAGE_SUMMARY; page++)
        {
            core_log(core, RETRO_LOG_INFO, "summary: %-12s %s\n", pages_title(static_cast<PageId>(page)),
                     result_name(core->pages.results[page]));
        }
    }
}

/* ---------------------------------------------------------------------------
 * Frame
 * ------------------------------------------------------------------------- */

void pages_run_frame(CoreState* core)
{
    Pages* pages = &core->pages;
    PatternArea pattern_area;
    bool draw_pattern = false;
    char title[64];

    handle_controller(core);
    core->audio.playing = (pages->page == PAGE_AUDIO);
    bool entered = !pages->drawn_once || pages->page != pages->last_drawn_page;
    pages->last_drawn_page = pages->page;
    pages->drawn_once = true;

    if (core->ui == nullptr)
    {
        core->key_queue_count = 0;
        return;
    }
    /* The keyboard callback's events, for the Keyboard page's log (then for ImGui). */
    for (unsigned i = 0; i < core->key_queue_count; i++)
    {
        const KeyEvent* event = &core->key_queue[i];
        keyboard_log_event(&core->keyboard, event->down, event->keycode, event->character, event->modifiers);
    }
    UiInput input = {};
    input.pointer_valid = core->input.pointer_valid;
    input.pointer_x = core->input.pointer_x;
    input.pointer_y = core->input.pointer_y;
    input.pointer_pressed = core->input.pointer_pressed;
    input.keys = core->key_queue;
    input.key_count = core->key_queue_count;
    input.delta_seconds = core->fps > 0 ? 1.0f / static_cast<float>(core->fps) : 1.0f / 60.0f;
    ui_begin_frame(core->ui, &core->framebuffer, &input);
    core->key_queue_count = 0;

    /* The same id every frame (after ###) so ImGui keeps its state; the visible title changes. */
    std::snprintf(title, sizeof(title), "Test %d/%d: %s###pages", static_cast<int>(pages->page) + 1,
                  static_cast<int>(PAGE_COUNT), pages_title(pages->page));
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;
    if (ImGui::Begin(title, nullptr, flags))
    {
        /* The page above, in its own region, so it never covers the navigation. */
        ImGui::BeginChild("page", ImVec2(0.0f, -navigation_height()), ImGuiChildFlags_None,
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        switch (pages->page)
        {
            case PAGE_CALLBACKS:   page_callbacks(core); break;
            case PAGE_VIDEO:       pattern_area = page_video(core); draw_pattern = true; break;
            case PAGE_AUDIO:       page_audio(core); break;
            case PAGE_RETROPAD:    page_retropad(core); break;
            case PAGE_CONTROLLERS: page_controllers(core); break;
            case PAGE_KEYBOARD:    page_keyboard(core); break;
            case PAGE_MOUSE:       page_mouse(core); break;
            case PAGE_SRAM:        page_sram(core, entered); break;
            case PAGE_SAVESTATE:   page_savestate(core); break;
            case PAGE_SUMMARY:     page_summary(core); break;
            default:               break;
        }
        ImGui::EndChild();
        draw_navigation(core);
    }
    ImGui::End();

    ui_end_frame(core->ui, &core->framebuffer);

    /* Drawn straight into the framebuffer after the UI, in the area the page reserved. */
    if (draw_pattern)
    {
        draw_video_pattern(&core->framebuffer, pattern_area);
    }
}

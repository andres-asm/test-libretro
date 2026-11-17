#include "ui.h"

#include <cfloat>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "imgui.h"
#include "imgui_sw.hpp"

#include "pixel_font.h"
#include "pixel_font_imgui.h"

struct Ui
{
    ImGuiContext*         context = nullptr;
    ImFont*               font = nullptr;
    uint32_t*             canvas = nullptr;   // canvas_pixels of ImGui's packed colors (width * height)
    size_t                canvas_pixels = 0;
};

static const ImU32 BACKGROUND = IM_COL32(24, 24, 32, 255);

// Square, flat and crisp: no rounding, no anti-aliasing (the pixel font has none either).
static void set_style()
{
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.ChildRounding = 0.0f;
    style.FrameRounding = 0.0f;
    style.GrabRounding = 0.0f;
    style.PopupRounding = 0.0f;
    style.ScrollbarRounding = 0.0f;
    style.TabRounding = 0.0f;
    style.WindowBorderSize = 0.0f;
    style.WindowPadding = ImVec2(6.0f, 6.0f);
    style.FramePadding = ImVec2(4.0f, 3.0f);
    style.ItemSpacing = ImVec2(6.0f, 4.0f);
    imgui_sw::make_style_fast();
}

struct Ui* ui_create(void)
{
    if (!pixel_font_init())
    {
        return nullptr;
    }
    Ui* ui = new Ui;
    ui->context = ImGui::CreateContext();
    ImGui::SetCurrentContext(ui->context);
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;   // nothing written next to the frontend
    io.LogFilename = nullptr;
    io.BackendPlatformName = "test_libretro";
    ui->font = pixel_font_imgui_add();
    if (ui->font == nullptr)
    {
        ui_destroy(ui);
        return nullptr;
    }
    set_style();
    imgui_sw::bind_imgui_painting();
    return ui;
}

void ui_destroy(struct Ui* ui)
{
    if (ui == nullptr)
    {
        return;
    }
    if (ui->context != nullptr)
    {
        ImGui::SetCurrentContext(ui->context);
        imgui_sw::unbind_imgui_painting();
        ImGui::DestroyContext(ui->context);
    }
    std::free(ui->canvas);
    delete ui;
}

// The key events the keyboard callback queued: typed text and the keys a
// text field needs.
static void feed_keys(ImGuiIO& io, const UiInput* input)
{
    for (unsigned i = 0; i < input->key_count; i++)
    {
        const KeyEvent* event = &input->keys[i];
        if (event->down && event->character >= 32 && event->character != 127)
        {
            io.AddInputCharacter(event->character);
        }
        ImGuiKey key = ImGuiKey_None;
        switch (event->keycode)
        {
            case RETROK_BACKSPACE: key = ImGuiKey_Backspace; break;
            case RETROK_DELETE:    key = ImGuiKey_Delete; break;
            case RETROK_LEFT:      key = ImGuiKey_LeftArrow; break;
            case RETROK_RIGHT:     key = ImGuiKey_RightArrow; break;
            case RETROK_HOME:      key = ImGuiKey_Home; break;
            case RETROK_END:       key = ImGuiKey_End; break;
            case RETROK_RETURN:    key = ImGuiKey_Enter; break;
            default:               break;
        }
        if (key != ImGuiKey_None)
        {
            io.AddKeyEvent(key, event->down);
        }
    }
}

void ui_begin_frame(struct Ui* ui, const Framebuffer* framebuffer, const UiInput* input)
{
    ImGui::SetCurrentContext(ui->context);
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(static_cast<float>(framebuffer->width), static_cast<float>(framebuffer->height));
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
    io.DeltaTime = input->delta_seconds > 0.0f ? input->delta_seconds : 1.0f / 60.0f;
    if (input->pointer_valid)
    {
        io.AddMousePosEvent(static_cast<float>(input->pointer_x), static_cast<float>(input->pointer_y));
        io.AddMouseButtonEvent(ImGuiMouseButton_Left, input->pointer_pressed);
    }
    else
    {
        io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
    }
    feed_keys(io, input);
    ImGui::NewFrame();
    ImGui::PushFont(ui->font, PIXEL_FONT_IMGUI_UNIT);
}

void ui_end_frame(struct Ui* ui, Framebuffer* framebuffer)
{
    ImGui::PopFont();
    ImGui::Render();
    size_t pixels = static_cast<size_t>(framebuffer->width) * framebuffer->height;
    if (pixels != ui->canvas_pixels)
    {
        std::free(ui->canvas);
        ui->canvas = static_cast<uint32_t*>(std::malloc(pixels * sizeof(uint32_t)));
        ui->canvas_pixels = ui->canvas != nullptr ? pixels : 0;
        if (ui->canvas == nullptr)
        {
            return;
        }
    }
    for (size_t i = 0; i < pixels; i++)
    {
        ui->canvas[i] = BACKGROUND;
    }
    imgui_sw::paint_imgui(ui->canvas, ImGui::GetDrawData());
    video_put_canvas(framebuffer, ui->canvas);
}

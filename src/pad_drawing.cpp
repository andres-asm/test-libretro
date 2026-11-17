#include "pad_drawing.h"

#include "imgui.h"

#include "controllers.h"

// A button: a square block (the software renderer draws circles poorly), no label.
struct PadPart
{
    unsigned id;               // RETRO_DEVICE_ID_JOYPAD_*
    float    x0, y0, x1, y1;   // in a 100 x 50 box
};

// The RetroPad (and the base for the others): shoulders, triggers, d-pad,
// SELECT / START, the four face buttons by position.
static const PadPart RETROPAD_PARTS[] = {
    {RETRO_DEVICE_ID_JOYPAD_L2, 14, 0, 28, 4},
    {RETRO_DEVICE_ID_JOYPAD_R2, 72, 0, 86, 4},
    {RETRO_DEVICE_ID_JOYPAD_L, 12, 5, 30, 9},
    {RETRO_DEVICE_ID_JOYPAD_R, 70, 5, 88, 9},
    {RETRO_DEVICE_ID_JOYPAD_UP, 19, 14, 25, 19},
    {RETRO_DEVICE_ID_JOYPAD_DOWN, 19, 25, 25, 30},
    {RETRO_DEVICE_ID_JOYPAD_LEFT, 13, 19, 19, 25},
    {RETRO_DEVICE_ID_JOYPAD_RIGHT, 25, 19, 31, 25},
    {RETRO_DEVICE_ID_JOYPAD_SELECT, 39, 17, 47, 20},
    {RETRO_DEVICE_ID_JOYPAD_START, 53, 17, 61, 20},
    {RETRO_DEVICE_ID_JOYPAD_X, 75, 12, 81, 18},
    {RETRO_DEVICE_ID_JOYPAD_B, 75, 26, 81, 32},
    {RETRO_DEVICE_ID_JOYPAD_Y, 68, 19, 74, 25},
    {RETRO_DEVICE_ID_JOYPAD_A, 82, 19, 88, 25},
};

static const PadPart NES_PARTS[] = {
    {RETRO_DEVICE_ID_JOYPAD_UP, 19, 14, 25, 19},
    {RETRO_DEVICE_ID_JOYPAD_DOWN, 19, 25, 25, 30},
    {RETRO_DEVICE_ID_JOYPAD_LEFT, 13, 19, 19, 25},
    {RETRO_DEVICE_ID_JOYPAD_RIGHT, 25, 19, 31, 25},
    {RETRO_DEVICE_ID_JOYPAD_SELECT, 39, 24, 47, 27},
    {RETRO_DEVICE_ID_JOYPAD_START, 53, 24, 61, 27},
    {RETRO_DEVICE_ID_JOYPAD_B, 67, 21, 75, 29},
    {RETRO_DEVICE_ID_JOYPAD_A, 79, 21, 87, 29},
};

static const PadPart SNES_PARTS[] = {
    {RETRO_DEVICE_ID_JOYPAD_L, 12, 5, 30, 9},
    {RETRO_DEVICE_ID_JOYPAD_R, 70, 5, 88, 9},
    {RETRO_DEVICE_ID_JOYPAD_UP, 19, 14, 25, 19},
    {RETRO_DEVICE_ID_JOYPAD_DOWN, 19, 25, 25, 30},
    {RETRO_DEVICE_ID_JOYPAD_LEFT, 13, 19, 19, 25},
    {RETRO_DEVICE_ID_JOYPAD_RIGHT, 25, 19, 31, 25},
    {RETRO_DEVICE_ID_JOYPAD_SELECT, 39, 20, 47, 23},
    {RETRO_DEVICE_ID_JOYPAD_START, 53, 20, 61, 23},
    {RETRO_DEVICE_ID_JOYPAD_X, 75, 12, 81, 18},
    {RETRO_DEVICE_ID_JOYPAD_B, 75, 26, 81, 32},
    {RETRO_DEVICE_ID_JOYPAD_Y, 68, 19, 74, 25},
    {RETRO_DEVICE_ID_JOYPAD_A, 82, 19, 88, 25},
};

// PlayStation's buttons on the RetroPad's positions: Cross bottom (B), Circle
// right (A), Square left (Y), Triangle top (X).
static const PadPart DUALSHOCK_PARTS[] = {
    {RETRO_DEVICE_ID_JOYPAD_L2, 14, 0, 28, 4},
    {RETRO_DEVICE_ID_JOYPAD_R2, 72, 0, 86, 4},
    {RETRO_DEVICE_ID_JOYPAD_L, 12, 5, 30, 9},
    {RETRO_DEVICE_ID_JOYPAD_R, 70, 5, 88, 9},
    {RETRO_DEVICE_ID_JOYPAD_UP, 19, 14, 25, 19},
    {RETRO_DEVICE_ID_JOYPAD_DOWN, 19, 25, 25, 30},
    {RETRO_DEVICE_ID_JOYPAD_LEFT, 13, 19, 19, 25},
    {RETRO_DEVICE_ID_JOYPAD_RIGHT, 25, 19, 31, 25},
    {RETRO_DEVICE_ID_JOYPAD_SELECT, 39, 17, 47, 20},
    {RETRO_DEVICE_ID_JOYPAD_START, 53, 17, 61, 20},
    {RETRO_DEVICE_ID_JOYPAD_X, 75, 12, 81, 18},
    {RETRO_DEVICE_ID_JOYPAD_B, 75, 26, 81, 32},
    {RETRO_DEVICE_ID_JOYPAD_Y, 68, 19, 74, 25},
    {RETRO_DEVICE_ID_JOYPAD_A, 82, 19, 88, 25},
};

struct PadLayout
{
    const PadPart* parts;
    unsigned       count;
    bool           sticks;
    float          body_x0, body_y0, body_x1, body_y1;
};

static PadLayout layout_for(PadStyle style)
{
    PadLayout layout = {RETROPAD_PARTS, sizeof(RETROPAD_PARTS) / sizeof(RETROPAD_PARTS[0]), true, 8, 10, 92, 46};
    if (style == PadStyle::nes)
    {
        layout = {NES_PARTS, sizeof(NES_PARTS) / sizeof(NES_PARTS[0]), false, 6, 12, 94, 38};
    }
    else if (style == PadStyle::snes)
    {
        layout = {SNES_PARTS, sizeof(SNES_PARTS) / sizeof(SNES_PARTS[0]), false, 8, 9, 92, 38};
    }
    else if (style == PadStyle::dualshock)
    {
        layout = {DUALSHOCK_PARTS, sizeof(DUALSHOCK_PARTS) / sizeof(DUALSHOCK_PARTS[0]), true, 8, 10, 92, 46};
    }
    return layout;
}

PadStyle pad_style_for_device(unsigned device)
{
    switch (device)
    {
        case DEVICE_NES:       return PadStyle::nes;
        case DEVICE_SNES:      return PadStyle::snes;
        case DEVICE_DUALSHOCK: return PadStyle::dualshock;
        default:               return PadStyle::retropad;
    }
}

static const ImU32 COLOR_BODY = IM_COL32(62, 62, 78, 255);
static const ImU32 COLOR_PART = IM_COL32(32, 32, 42, 255);
static const ImU32 COLOR_LIT = IM_COL32(255, 220, 80, 255);
static const ImU32 COLOR_DARK = IM_COL32(24, 24, 32, 255);

// A stick: a square base (lit while clicked) and a dot where it is pushed.
static void draw_stick(ImDrawList* draw, ImVec2 center, float half, bool clicked, const int16_t* axes)
{
    draw->AddRectFilled(ImVec2(center.x - half, center.y - half), ImVec2(center.x + half, center.y + half),
                        clicked ? COLOR_LIT : COLOR_PART);
    float x = static_cast<float>(axes[0]) / 32768.0f;
    float y = static_cast<float>(axes[1]) / 32768.0f;
    ImVec2 dot(center.x + x * half * 0.75f, center.y + y * half * 0.75f);
    float size = half * 0.22f;
    draw->AddRectFilled(ImVec2(dot.x - size, dot.y - size), ImVec2(dot.x + size, dot.y + size),
                        clicked ? COLOR_DARK : COLOR_LIT);
}

void pad_draw(PadStyle style, uint16_t held, const int16_t analog[2][2], float width)
{
    PadLayout layout = layout_for(style);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    float available = ImGui::GetContentRegionAvail().x;
    float unit = width / 100.0f;
    ImVec2 origin = ImGui::GetCursorScreenPos();
    origin.x += static_cast<float>(static_cast<int>((available - width) * 0.5f));

    draw->AddRectFilled(ImVec2(origin.x + layout.body_x0 * unit, origin.y + layout.body_y0 * unit),
                        ImVec2(origin.x + layout.body_x1 * unit, origin.y + layout.body_y1 * unit), COLOR_BODY);
    // The d-pad's middle.
    draw->AddRectFilled(ImVec2(origin.x + 19.0f * unit, origin.y + 19.0f * unit),
                        ImVec2(origin.x + 25.0f * unit, origin.y + 25.0f * unit), COLOR_PART);

    for (unsigned p = 0; p < layout.count; p++)
    {
        const PadPart* part = &layout.parts[p];
        bool lit = (held & (1u << part->id)) != 0;
        draw->AddRectFilled(ImVec2(origin.x + part->x0 * unit, origin.y + part->y0 * unit),
                            ImVec2(origin.x + part->x1 * unit, origin.y + part->y1 * unit), lit ? COLOR_LIT : COLOR_PART);
    }

    if (layout.sticks)
    {
        float half = 5.0f * unit;
        draw_stick(draw, ImVec2(origin.x + 36.0f * unit, origin.y + 35.0f * unit), half,
                   (held & (1u << RETRO_DEVICE_ID_JOYPAD_L3)) != 0, analog[0]);
        draw_stick(draw, ImVec2(origin.x + 64.0f * unit, origin.y + 35.0f * unit), half,
                   (held & (1u << RETRO_DEVICE_ID_JOYPAD_R3)) != 0, analog[1]);
    }

    ImGui::Dummy(ImVec2(available, 50.0f * unit));
}

#include "pad_drawing.h"

#include "imgui.h"

#include "controllers.h"

// How a part looks.
enum class PartShape
{
    rect,
    round,      // a round face button with its label
    triangle,   // PlayStation's face buttons: round, with the symbol
    circle,
    cross,
    square,
};

struct PadPart
{
    unsigned    id;               // RETRO_DEVICE_ID_JOYPAD_*
    float       x0, y0, x1, y1;   // in a 100 x 50 box
    const char* label;            // drawn inside, or under it when it doesn't fit ("" = none)
    PartShape   shape;
};

// The RetroPad (and the base for the others): shoulders, triggers, d-pad,
// SELECT / START, the four face buttons by position.
static const PadPart RETROPAD_PARTS[] = {
    {RETRO_DEVICE_ID_JOYPAD_L2, 14, 0, 28, 4, "L2", PartShape::rect},
    {RETRO_DEVICE_ID_JOYPAD_R2, 72, 0, 86, 4, "R2", PartShape::rect},
    {RETRO_DEVICE_ID_JOYPAD_L, 12, 5, 30, 9, "L", PartShape::rect},
    {RETRO_DEVICE_ID_JOYPAD_R, 70, 5, 88, 9, "R", PartShape::rect},
    {RETRO_DEVICE_ID_JOYPAD_UP, 19, 14, 25, 19, "", PartShape::rect},
    {RETRO_DEVICE_ID_JOYPAD_DOWN, 19, 25, 25, 30, "", PartShape::rect},
    {RETRO_DEVICE_ID_JOYPAD_LEFT, 13, 19, 19, 25, "", PartShape::rect},
    {RETRO_DEVICE_ID_JOYPAD_RIGHT, 25, 19, 31, 25, "", PartShape::rect},
    {RETRO_DEVICE_ID_JOYPAD_SELECT, 39, 17, 47, 20, "SELECT", PartShape::rect},
    {RETRO_DEVICE_ID_JOYPAD_START, 53, 17, 61, 20, "START", PartShape::rect},
    {RETRO_DEVICE_ID_JOYPAD_X, 75, 12, 81, 18, "X", PartShape::round},
    {RETRO_DEVICE_ID_JOYPAD_B, 75, 26, 81, 32, "B", PartShape::round},
    {RETRO_DEVICE_ID_JOYPAD_Y, 68, 19, 74, 25, "Y", PartShape::round},
    {RETRO_DEVICE_ID_JOYPAD_A, 82, 19, 88, 25, "A", PartShape::round},
};

static const PadPart NES_PARTS[] = {
    {RETRO_DEVICE_ID_JOYPAD_UP, 19, 14, 25, 19, "", PartShape::rect},
    {RETRO_DEVICE_ID_JOYPAD_DOWN, 19, 25, 25, 30, "", PartShape::rect},
    {RETRO_DEVICE_ID_JOYPAD_LEFT, 13, 19, 19, 25, "", PartShape::rect},
    {RETRO_DEVICE_ID_JOYPAD_RIGHT, 25, 19, 31, 25, "", PartShape::rect},
    {RETRO_DEVICE_ID_JOYPAD_SELECT, 39, 24, 47, 27, "SELECT", PartShape::rect},
    {RETRO_DEVICE_ID_JOYPAD_START, 53, 24, 61, 27, "START", PartShape::rect},
    {RETRO_DEVICE_ID_JOYPAD_B, 67, 21, 75, 29, "B", PartShape::round},
    {RETRO_DEVICE_ID_JOYPAD_A, 79, 21, 87, 29, "A", PartShape::round},
};

static const PadPart SNES_PARTS[] = {
    {RETRO_DEVICE_ID_JOYPAD_L, 12, 5, 30, 9, "L", PartShape::rect},
    {RETRO_DEVICE_ID_JOYPAD_R, 70, 5, 88, 9, "R", PartShape::rect},
    {RETRO_DEVICE_ID_JOYPAD_UP, 19, 14, 25, 19, "", PartShape::rect},
    {RETRO_DEVICE_ID_JOYPAD_DOWN, 19, 25, 25, 30, "", PartShape::rect},
    {RETRO_DEVICE_ID_JOYPAD_LEFT, 13, 19, 19, 25, "", PartShape::rect},
    {RETRO_DEVICE_ID_JOYPAD_RIGHT, 25, 19, 31, 25, "", PartShape::rect},
    {RETRO_DEVICE_ID_JOYPAD_SELECT, 39, 20, 47, 23, "SELECT", PartShape::rect},
    {RETRO_DEVICE_ID_JOYPAD_START, 53, 20, 61, 23, "START", PartShape::rect},
    {RETRO_DEVICE_ID_JOYPAD_X, 75, 12, 81, 18, "X", PartShape::round},
    {RETRO_DEVICE_ID_JOYPAD_B, 75, 26, 81, 32, "B", PartShape::round},
    {RETRO_DEVICE_ID_JOYPAD_Y, 68, 19, 74, 25, "Y", PartShape::round},
    {RETRO_DEVICE_ID_JOYPAD_A, 82, 19, 88, 25, "A", PartShape::round},
};

// PlayStation names on the RetroPad's positions: Cross bottom (B), Circle
// right (A), Square left (Y), Triangle top (X).
static const PadPart DUALSHOCK_PARTS[] = {
    {RETRO_DEVICE_ID_JOYPAD_L2, 14, 0, 28, 4, "L2", PartShape::rect},
    {RETRO_DEVICE_ID_JOYPAD_R2, 72, 0, 86, 4, "R2", PartShape::rect},
    {RETRO_DEVICE_ID_JOYPAD_L, 12, 5, 30, 9, "L1", PartShape::rect},
    {RETRO_DEVICE_ID_JOYPAD_R, 70, 5, 88, 9, "R1", PartShape::rect},
    {RETRO_DEVICE_ID_JOYPAD_UP, 19, 14, 25, 19, "", PartShape::rect},
    {RETRO_DEVICE_ID_JOYPAD_DOWN, 19, 25, 25, 30, "", PartShape::rect},
    {RETRO_DEVICE_ID_JOYPAD_LEFT, 13, 19, 19, 25, "", PartShape::rect},
    {RETRO_DEVICE_ID_JOYPAD_RIGHT, 25, 19, 31, 25, "", PartShape::rect},
    {RETRO_DEVICE_ID_JOYPAD_SELECT, 39, 17, 47, 20, "SELECT", PartShape::rect},
    {RETRO_DEVICE_ID_JOYPAD_START, 53, 17, 61, 20, "START", PartShape::rect},
    {RETRO_DEVICE_ID_JOYPAD_X, 75, 12, 81, 18, "", PartShape::triangle},
    {RETRO_DEVICE_ID_JOYPAD_B, 75, 26, 81, 32, "", PartShape::cross},
    {RETRO_DEVICE_ID_JOYPAD_Y, 68, 19, 74, 25, "", PartShape::square},
    {RETRO_DEVICE_ID_JOYPAD_A, 82, 19, 88, 25, "", PartShape::circle},
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
static const ImU32 COLOR_LABEL = IM_COL32(215, 215, 225, 255);
static const ImU32 COLOR_LABEL_LIT = IM_COL32(24, 24, 32, 255);

// PlayStation's symbols, centered on `c`, `size` from the center to the edge.
static void draw_symbol(ImDrawList* draw, PartShape shape, ImVec2 c, float size, ImU32 color)
{
    float thickness = size > 6.0f ? 2.0f : 1.0f;
    switch (shape)
    {
        case PartShape::triangle:
            draw->AddTriangle(ImVec2(c.x, c.y - size), ImVec2(c.x - size * 0.9f, c.y + size * 0.6f),
                              ImVec2(c.x + size * 0.9f, c.y + size * 0.6f), color, thickness);
            break;
        case PartShape::circle:
            draw->AddCircle(c, size * 0.8f, color, 0, thickness);
            break;
        case PartShape::cross:
            draw->AddLine(ImVec2(c.x - size * 0.7f, c.y - size * 0.7f), ImVec2(c.x + size * 0.7f, c.y + size * 0.7f), color,
                          thickness);
            draw->AddLine(ImVec2(c.x + size * 0.7f, c.y - size * 0.7f), ImVec2(c.x - size * 0.7f, c.y + size * 0.7f), color,
                          thickness);
            break;
        case PartShape::square:
            draw->AddRect(ImVec2(c.x - size * 0.7f, c.y - size * 0.7f), ImVec2(c.x + size * 0.7f, c.y + size * 0.7f), color, 0.0f,
                          0, thickness);
            break;
        default:
            break;
    }
}

// A label inside the part, or under it when it doesn't fit.
static void draw_label(ImDrawList* draw, const char* label, ImVec2 min, ImVec2 max, bool lit)
{
    if (label[0] == '\0')
    {
        return;
    }
    ImVec2 size = ImGui::CalcTextSize(label);
    float width = max.x - min.x;
    float height = max.y - min.y;
    if (size.x + 2.0f <= width && size.y <= height + 2.0f)
    {
        ImVec2 at(min.x + (width - size.x) * 0.5f, min.y + (height - size.y) * 0.5f);
        draw->AddText(ImVec2(static_cast<float>(static_cast<int>(at.x)), static_cast<float>(static_cast<int>(at.y))),
                      lit ? COLOR_LABEL_LIT : COLOR_LABEL, label);
    }
    else
    {
        ImVec2 at(min.x + (width - size.x) * 0.5f, max.y + 2.0f);
        draw->AddText(ImVec2(static_cast<float>(static_cast<int>(at.x)), static_cast<float>(static_cast<int>(at.y))),
                      COLOR_LABEL, label);
    }
}

// A stick: its base (lit while clicked), a ring, and a dot where it is pushed.
static void draw_stick(ImDrawList* draw, ImVec2 center, float radius, bool clicked, const int16_t* axes, const char* label)
{
    draw->AddCircleFilled(center, radius, clicked ? COLOR_LIT : COLOR_PART);
    draw->AddCircle(center, radius * 0.55f, clicked ? COLOR_LABEL_LIT : COLOR_BODY);
    float x = static_cast<float>(axes[0]) / 32768.0f;
    float y = static_cast<float>(axes[1]) / 32768.0f;
    ImVec2 dot(center.x + x * radius * 0.8f, center.y + y * radius * 0.8f);
    float half = radius * 0.18f;
    draw->AddRectFilled(ImVec2(dot.x - half, dot.y - half), ImVec2(dot.x + half, dot.y + half),
                        clicked ? COLOR_LABEL_LIT : COLOR_LIT);
    ImVec2 size = ImGui::CalcTextSize(label);
    draw->AddText(ImVec2(static_cast<float>(static_cast<int>(center.x - size.x * 0.5f)),
                         static_cast<float>(static_cast<int>(center.y + radius + 2.0f))),
                  COLOR_LABEL, label);
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
        ImU32 fill = lit ? COLOR_LIT : COLOR_PART;
        ImVec2 min(origin.x + part->x0 * unit, origin.y + part->y0 * unit);
        ImVec2 max(origin.x + part->x1 * unit, origin.y + part->y1 * unit);
        if (part->shape == PartShape::rect)
        {
            draw->AddRectFilled(min, max, fill);
            draw_label(draw, part->label, min, max, lit);
            continue;
        }
        ImVec2 center((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f);
        float radius = (max.x - min.x) * 0.5f;
        draw->AddCircleFilled(center, radius, fill);
        if (part->shape == PartShape::round)
        {
            draw_label(draw, part->label, min, max, lit);
        }
        else
        {
            draw_symbol(draw, part->shape, center, radius * 0.6f, lit ? COLOR_LABEL_LIT : COLOR_LABEL);
        }
    }

    if (layout.sticks)
    {
        float radius = 5.0f * unit;
        draw_stick(draw, ImVec2(origin.x + 36.0f * unit, origin.y + 34.0f * unit), radius,
                   (held & (1u << RETRO_DEVICE_ID_JOYPAD_L3)) != 0, analog[0], "L3");
        draw_stick(draw, ImVec2(origin.x + 64.0f * unit, origin.y + 34.0f * unit), radius,
                   (held & (1u << RETRO_DEVICE_ID_JOYPAD_R3)) != 0, analog[1], "R3");
    }

    ImGui::Dummy(ImVec2(available, 50.0f * unit));
}

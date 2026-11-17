#pragma once

// A gamepad drawn with square blocks (the software renderer draws circles
// poorly): every button of its style, lit while held, and the sticks with a
// dot where they are pushed. No labels: the shape tells which button it is.

#include <cstdint>

enum class PadStyle
{
    retropad,
    nes,
    snes,
    dualshock,
};

// The style that draws a controller type (controllers.h), RetroPad for any other.
PadStyle pad_style_for_device(unsigned device);

// At the cursor, centered in the available width, `width` pixels wide (and
// half as tall). `held`: bit N = RETRO_DEVICE_ID_JOYPAD_N. `analog`:
// [left / right stick][x / y], -0x8000..0x7fff.
void pad_draw(PadStyle style, uint16_t held, const int16_t analog[2][2], float width);

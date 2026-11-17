#ifndef TEST_LIBRETRO_UI_H
#define TEST_LIBRETRO_UI_H

/*
 * The core's UI: Dear ImGui with piccolo's pixel font, drawn by a software
 * renderer (imgui_sw) into a canvas of 32-bit pixels, then copied into the
 * framebuffer in whatever pixel format and pitch it has (video_put_canvas).
 *
 * Between ui_begin_frame() and ui_end_frame() the pages build their windows
 * with ImGui calls (pages.cpp). ImGui lives inside the core with hidden
 * symbols, so it never meets the frontend's own ImGui, if it has one.
 */

#include <stdbool.h>

#include "core.h"

#ifdef __cplusplus
extern "C" {
#endif

struct Ui;

/* What the UI gets from the frontend each frame. */
typedef struct UiInput
{
    bool            pointer_valid;
    int             pointer_x;        /* framebuffer pixels */
    int             pointer_y;
    bool            pointer_pressed;
    const KeyEvent* keys;             /* typed text and editing keys, since the last frame */
    unsigned        key_count;
    float           delta_seconds;
} UiInput;

/* NULL if ImGui or the font couldn't be set up. */
struct Ui* ui_create(void);
void       ui_destroy(struct Ui* ui);

/* Starts an ImGui frame the size of the framebuffer. */
void ui_begin_frame(struct Ui* ui, const Framebuffer* framebuffer, const UiInput* input);

/* Draws the frame into the framebuffer (replacing what was there). */
void ui_end_frame(struct Ui* ui, Framebuffer* framebuffer);

#ifdef __cplusplus
}
#endif

#endif

#ifndef TEST_WIZARD_VIDEO_H
#define TEST_WIZARD_VIDEO_H

/*
 * The core's software framebuffer, in whichever libretro pixel format the
 * frontend accepted, plus the Nuklear software renderer drawing into it.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "libretro.h"

struct rawfb_context;
struct nk_context;

/* Fixed output size. */
#define VIDEO_WIDTH  640u
#define VIDEO_HEIGHT 480u

/*
 * Each row is this many bytes LONGER than width * bytes_per_pixel, on purpose:
 * a frontend that ignores `pitch` shows a skewed image, which the video page
 * makes obvious.
 */
#define VIDEO_PITCH_PADDING 64u

typedef struct Framebuffer
{
    uint8_t*                pixels;
    unsigned                width;
    unsigned                height;
    size_t                  pitch;           /* bytes per row, including padding */
    unsigned                bytes_per_pixel;
    enum retro_pixel_format format;

    struct rawfb_context*   rawfb;           /* Nuklear software renderer */
    uint8_t*                font_scratch;    /* memory rawfb bakes its font into */
} Framebuffer;

bool video_create(Framebuffer* framebuffer, enum retro_pixel_format format);
void video_destroy(Framebuffer* framebuffer);

struct nk_context* video_ui_context(Framebuffer* framebuffer);

/* Draws the Nuklear UI built this frame into the framebuffer (clearing it first). */
void video_ui_render(Framebuffer* framebuffer);

/* Direct drawing, clipped to the framebuffer. Colors are 8-bit per channel. */
void video_fill_rect(Framebuffer* framebuffer, int x, int y, int width, int height,
                     uint8_t red, uint8_t green, uint8_t blue);

const char* video_pixel_format_name(enum retro_pixel_format format);

#endif

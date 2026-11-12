#ifndef TEST_LIBRETRO_VIDEO_H
#define TEST_LIBRETRO_VIDEO_H

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

/* Output size. The width can change at runtime (core option libretro_test_width);
 * VIDEO_MAX_WIDTH is what retro_get_system_av_info reports as max_width. */
#define VIDEO_MAX_WIDTH  720u
#define VIDEO_MAX_HEIGHT 720u

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

/* width 1..VIDEO_MAX_WIDTH, height 1..VIDEO_MAX_HEIGHT. */
bool video_create(Framebuffer* framebuffer, enum retro_pixel_format format, unsigned width, unsigned height);
void video_destroy(Framebuffer* framebuffer);

struct nk_context* video_ui_context(Framebuffer* framebuffer);

/* Draws the Nuklear UI built this frame into the framebuffer (clearing it first). */
void video_ui_render(Framebuffer* framebuffer);

/* Direct drawing, clipped to the framebuffer. Colors are 8-bit per channel. */
void video_fill_rect(Framebuffer* framebuffer, int x, int y, int width, int height,
                     uint8_t red, uint8_t green, uint8_t blue);

/*
 * The UI is drawn with square pixels, so the picture's aspect ratio is simply
 * width / height: 4:3 at 640x480, 3:2 at 720x480. Reporting anything else
 * would make the frontend squeeze or stretch the text.
 */
float video_aspect_ratio(unsigned width, unsigned height);

const char* video_pixel_format_name(enum retro_pixel_format format);

#endif

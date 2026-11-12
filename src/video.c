#include "video.h"

#include <stdlib.h>
#include <string.h>

#include "nuklear_config.h"

/* rawfb bakes Nuklear's default font into this; 512x512 is what its demos use. */
#define FONT_SCRATCH_SIZE (512u * 512u)

/*
 * How each libretro pixel format is laid out, in rawfb's terms:
 * channel bit position (shift) and how many low bits are dropped (loss).
 * aloss = 8 means "no alpha channel".
 */
static struct rawfb_pl pixel_layout(enum retro_pixel_format format)
{
    struct rawfb_pl layout;
    memset(&layout, 0, sizeof(layout));

    switch (format)
    {
        case RETRO_PIXEL_FORMAT_XRGB8888:
            layout.bytesPerPixel = 4;
            layout.rshift = 16;
            layout.gshift = 8;
            layout.bshift = 0;
            layout.rloss = 0;
            layout.gloss = 0;
            layout.bloss = 0;
            layout.aloss = 8;
            break;

        case RETRO_PIXEL_FORMAT_RGB565:
            layout.bytesPerPixel = 2;
            layout.rshift = 11;
            layout.gshift = 5;
            layout.bshift = 0;
            layout.rloss = 3;
            layout.gloss = 2;
            layout.bloss = 3;
            layout.aloss = 8;
            break;

        case RETRO_PIXEL_FORMAT_0RGB1555:
        default:
            layout.bytesPerPixel = 2;
            layout.rshift = 10;
            layout.gshift = 5;
            layout.bshift = 0;
            layout.rloss = 3;
            layout.gloss = 3;
            layout.bloss = 3;
            layout.aloss = 8;
            break;
    }
    return layout;
}

const char* video_pixel_format_name(enum retro_pixel_format format)
{
    switch (format)
    {
        case RETRO_PIXEL_FORMAT_XRGB8888: return "XRGB8888";
        case RETRO_PIXEL_FORMAT_RGB565:   return "RGB565";
        case RETRO_PIXEL_FORMAT_0RGB1555: return "0RGB1555";
        default:                          return "unknown";
    }
}

float video_aspect_ratio(unsigned width)
{
    return (float)width / (float)VIDEO_HEIGHT;
}

bool video_create(Framebuffer* framebuffer, enum retro_pixel_format format, unsigned width)
{
    struct rawfb_pl layout = pixel_layout(format);

    memset(framebuffer, 0, sizeof(*framebuffer));
    if (width == 0 || width > VIDEO_MAX_WIDTH)
    {
        return false;
    }
    framebuffer->width = width;
    framebuffer->height = VIDEO_HEIGHT;
    framebuffer->format = format;
    framebuffer->bytes_per_pixel = layout.bytesPerPixel;
    framebuffer->pitch = (size_t)width * layout.bytesPerPixel + VIDEO_PITCH_PADDING;

    framebuffer->pixels = (uint8_t*)calloc(framebuffer->pitch * VIDEO_HEIGHT, 1);
    framebuffer->font_scratch = (uint8_t*)calloc(FONT_SCRATCH_SIZE, 1);
    if (framebuffer->pixels == NULL || framebuffer->font_scratch == NULL)
    {
        video_destroy(framebuffer);
        return false;
    }

    framebuffer->rawfb = nk_rawfb_init(framebuffer->pixels, framebuffer->font_scratch,
                                       width, VIDEO_HEIGHT, (unsigned)framebuffer->pitch, layout);
    if (framebuffer->rawfb == NULL)
    {
        video_destroy(framebuffer);
        return false;
    }
    return true;
}

void video_destroy(Framebuffer* framebuffer)
{
    if (framebuffer->rawfb != NULL)
    {
        nuklear_rawfb_shutdown(framebuffer->rawfb);
    }
    free(framebuffer->font_scratch);
    free(framebuffer->pixels);
    memset(framebuffer, 0, sizeof(*framebuffer));
}

struct nk_context* video_ui_context(Framebuffer* framebuffer)
{
    return nuklear_rawfb_get_context(framebuffer->rawfb);
}

void video_ui_render(Framebuffer* framebuffer)
{
    struct nk_color clear = nk_rgb(24, 24, 32);
    nk_rawfb_render(framebuffer->rawfb, clear, 1);
}

/* Packs an 8-bit-per-channel color into the framebuffer's pixel format. */
static uint32_t pack_color(const Framebuffer* framebuffer, uint8_t red, uint8_t green, uint8_t blue)
{
    switch (framebuffer->format)
    {
        case RETRO_PIXEL_FORMAT_XRGB8888:
            return ((uint32_t)red << 16) | ((uint32_t)green << 8) | (uint32_t)blue;

        case RETRO_PIXEL_FORMAT_RGB565:
            return ((uint32_t)(red >> 3) << 11) | ((uint32_t)(green >> 2) << 5) | (uint32_t)(blue >> 3);

        case RETRO_PIXEL_FORMAT_0RGB1555:
        default:
            return ((uint32_t)(red >> 3) << 10) | ((uint32_t)(green >> 3) << 5) | (uint32_t)(blue >> 3);
    }
}

void video_fill_rect(Framebuffer* framebuffer, int x, int y, int width, int height,
                     uint8_t red, uint8_t green, uint8_t blue)
{
    int x_end = x + width;
    int y_end = y + height;
    int row = 0;
    int column = 0;
    uint32_t packed = pack_color(framebuffer, red, green, blue);

    if (x < 0)
    {
        x = 0;
    }
    if (y < 0)
    {
        y = 0;
    }
    if (x_end > (int)framebuffer->width)
    {
        x_end = (int)framebuffer->width;
    }
    if (y_end > (int)framebuffer->height)
    {
        y_end = (int)framebuffer->height;
    }

    for (row = y; row < y_end; row++)
    {
        uint8_t* line = framebuffer->pixels + (size_t)row * framebuffer->pitch;

        for (column = x; column < x_end; column++)
        {
            if (framebuffer->bytes_per_pixel == 4)
            {
                ((uint32_t*)(void*)line)[column] = packed;
            }
            else
            {
                ((uint16_t*)(void*)line)[column] = (uint16_t)packed;
            }
        }
    }
}

#include "video.h"

#include <stdlib.h>
#include <string.h>

/* Bytes per pixel of each libretro pixel format. */
static unsigned format_bytes(enum retro_pixel_format format)
{
    return format == RETRO_PIXEL_FORMAT_XRGB8888 ? 4u : 2u;
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

float video_aspect_ratio(unsigned width, unsigned height)
{
    return (float)width / (float)height;
}

bool video_create(Framebuffer* framebuffer, enum retro_pixel_format format, unsigned width, unsigned height)
{
    memset(framebuffer, 0, sizeof(*framebuffer));
    if (width == 0 || width > VIDEO_MAX_WIDTH || height == 0 || height > VIDEO_MAX_HEIGHT)
    {
        return false;
    }
    framebuffer->width = width;
    framebuffer->height = height;
    framebuffer->format = format;
    framebuffer->bytes_per_pixel = format_bytes(format);
    framebuffer->pitch = (size_t)width * framebuffer->bytes_per_pixel + VIDEO_PITCH_PADDING;

    framebuffer->pixels = (uint8_t*)calloc(framebuffer->pitch * height, 1);
    return framebuffer->pixels != NULL;
}

void video_destroy(Framebuffer* framebuffer)
{
    free(framebuffer->pixels);
    memset(framebuffer, 0, sizeof(*framebuffer));
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

void video_put_canvas(Framebuffer* framebuffer, const uint32_t* canvas)
{
    unsigned row = 0;
    unsigned column = 0;

    for (row = 0; row < framebuffer->height; row++)
    {
        uint8_t* line = framebuffer->pixels + (size_t)row * framebuffer->pitch;
        const uint32_t* source = canvas + (size_t)row * framebuffer->width;

        for (column = 0; column < framebuffer->width; column++)
        {
            uint32_t color = source[column];
            uint32_t packed = pack_color(framebuffer, (uint8_t)(color & 0xFFu), (uint8_t)((color >> 8) & 0xFFu),
                                         (uint8_t)((color >> 16) & 0xFFu));
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

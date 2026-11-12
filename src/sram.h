#ifndef TEST_LIBRETRO_SRAM_H
#define TEST_LIBRETRO_SRAM_H

/*
 * The core's save RAM (RETRO_MEMORY_SAVE_RAM): a small block holding the text
 * typed on the SRAM page, with a magic number and a checksum.
 *
 * The frontend loads save RAM into this block after retro_load_game and
 * writes it out when the game is unloaded. On the first frame the core looks
 * at what it got:
 *   magic + checksum OK  -> restored (shows the text)
 *   all zero             -> empty: first run, or the frontend didn't load it
 *   anything else        -> corrupted (the frontend loaded the wrong bytes)
 */

#include <stdbool.h>
#include <stdint.h>

#define SRAM_MAGIC     0x4D415253u   /* "SRAM" */
#define SRAM_TEXT_SIZE 48u           /* including the terminating zero */

typedef struct SramData
{
    uint32_t magic;
    uint32_t length;                 /* strlen(text) */
    char     text[SRAM_TEXT_SIZE];
    uint32_t checksum;               /* FNV-1a over magic, length and text */
} SramData;

typedef enum SramState
{
    SRAM_NOT_CHECKED,
    SRAM_EMPTY,
    SRAM_RESTORED,
    SRAM_CORRUPTED
} SramState;

/* Looks at what the frontend loaded. Call once, on the first retro_run. */
SramState sram_check(const SramData* sram);

/* After the text changed: makes the block valid (magic, length, checksum). */
void sram_seal(SramData* sram);

const char* sram_state_name(SramState state);

#endif

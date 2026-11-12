#include "sram.h"

#include <stddef.h>
#include <string.h>

static uint32_t sram_checksum(const SramData* sram)
{
    /* FNV-1a: simple, and any changed byte changes the result. */
    const uint8_t* bytes = (const uint8_t*)sram;
    size_t length = offsetof(SramData, checksum);
    uint32_t hash = 2166136261u;
    size_t i = 0;

    for (i = 0; i < length; i++)
    {
        hash ^= bytes[i];
        hash *= 16777619u;
    }
    return hash;
}

SramState sram_check(const SramData* sram)
{
    const uint8_t* bytes = (const uint8_t*)sram;
    bool all_zero = true;
    size_t i = 0;

    for (i = 0; i < sizeof(*sram); i++)
    {
        if (bytes[i] != 0)
        {
            all_zero = false;
            break;
        }
    }
    if (all_zero)
    {
        return SRAM_EMPTY;
    }
    if (sram->magic != SRAM_MAGIC || sram->length >= SRAM_TEXT_SIZE || sram->text[sram->length] != '\0' ||
        sram->checksum != sram_checksum(sram))
    {
        return SRAM_CORRUPTED;
    }
    return SRAM_RESTORED;
}

void sram_seal(SramData* sram)
{
    sram->text[SRAM_TEXT_SIZE - 1] = '\0';
    sram->magic = SRAM_MAGIC;
    sram->length = (uint32_t)strlen(sram->text);
    /* Bytes after the text are zeroed so the checksum only depends on what's visible. */
    memset(sram->text + sram->length, 0, SRAM_TEXT_SIZE - sram->length);
    sram->checksum = sram_checksum(sram);
}

const char* sram_state_name(SramState state)
{
    switch (state)
    {
        case SRAM_NOT_CHECKED: return "not checked yet";
        case SRAM_EMPTY:       return "empty";
        case SRAM_RESTORED:    return "restored";
        case SRAM_CORRUPTED:   return "CORRUPTED";
        default:               return "?";
    }
}

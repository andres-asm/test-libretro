#ifndef TEST_LIBRETRO_ADAPTERS_H
#define TEST_LIBRETRO_ADAPTERS_H

/*
 * Multitap's port adapters extension (libretro_extensions.h, from host-libretro):
 * the core describes its physical ports and the adapters they take, the
 * frontend picks one per port and tells the core where each one's players are.
 *
 * Test libretro offers two physical ports: port 1 takes a Multitap (4
 * players) or a Four Score (4 players over both ports), port 2 a Multitap.
 * Its "game" suggests a Multitap on port 2 (for AUTO). Up to 8 players: the
 * Controllers page shows any of them.
 *
 * Built in only with TEST_LIBRETRO_EXTENSIONS (CMake found the header), like
 * text_input.h.
 */

#include <stdbool.h>

#include "libretro.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ADAPTERS_PHYSICAL_PORTS 2

typedef struct AdaptersState
{
    bool     supported;                              /* the frontend drives the adapters */
    unsigned set_calls;                              /* set_adapters calls */
    unsigned adapter[ADAPTERS_PHYSICAL_PORTS];       /* in use per physical port, 0 = none */
    unsigned first_port[ADAPTERS_PHYSICAL_PORTS];    /* its first player (libretro port), or unused */
    bool     taken[ADAPTERS_PHYSICAL_PORTS];         /* the adapter before took this port */
} AdaptersState;

/* At init: describes the ports (EXT_SET_PORT_ADAPTERS). */
void adapters_offer(AdaptersState* state, retro_environment_t environment);

/* In retro_load_game: the "game" wants a Multitap on port 2 (EXT_SUGGEST_PORT_ADAPTERS). */
void adapters_suggest(const AdaptersState* state, retro_environment_t environment);

/* An adapter's name, for the page ("None" for 0). */
const char* adapters_name(unsigned adapter);

/* Players an adapter gives (1 without one). */
unsigned adapters_players(unsigned adapter);

#ifdef __cplusplus
}
#endif

#endif

#include "adapters.h"

#include <string.h>

#include "controllers.h"

enum
{
    ADAPTER_MULTITAP   = 1,
    ADAPTER_FOUR_SCORE = 2
};

const char* adapters_name(unsigned adapter)
{
    switch (adapter)
    {
        case ADAPTER_MULTITAP:   return "Multitap";
        case ADAPTER_FOUR_SCORE: return "Four Score";
        default:                 return "None";
    }
}

unsigned adapters_players(unsigned adapter)
{
    return adapter == ADAPTER_MULTITAP || adapter == ADAPTER_FOUR_SCORE ? 4u : 1u;
}

#if defined(TEST_LIBRETRO_EXTENSIONS)

#include "libretro_extensions.h"

/* libretro's callbacks carry no pointer back to us; one core, one state. */
static AdaptersState* g_adapters = NULL;

static void set_adapters(const struct retro_ext_adapter_choice* choices, unsigned count)
{
    unsigned p = 0;
    if (g_adapters == NULL)
    {
        return;
    }
    g_adapters->set_calls++;
    for (p = 0; p < ADAPTERS_PHYSICAL_PORTS; p++)
    {
        bool given = p < count;
        g_adapters->taken[p] = given && choices[p].first_port == RETRO_EXT_PORT_UNUSED;
        g_adapters->adapter[p] = given && !g_adapters->taken[p] ? choices[p].adapter : 0;
        g_adapters->first_port[p] = given && !g_adapters->taken[p] ? choices[p].first_port : 0;
    }
}

void adapters_offer(AdaptersState* state, retro_environment_t environment)
{
    /* Sub-ports take the same pads as a port itself; the Four Score only NES-like ones. */
    static const struct retro_controller_description four_score_types[] = {
        {"RetroPad", RETRO_DEVICE_JOYPAD},
        {"NES style gamepad", DEVICE_NES},
    };
    static struct retro_ext_adapter port1_adapters[2];
    static struct retro_ext_adapter port2_adapters[1];
    static struct retro_ext_physical_port ports[ADAPTERS_PHYSICAL_PORTS];
    struct retro_ext_port_adapters description;
    const struct retro_controller_description* types = controllers_types();
    unsigned type_count = controllers_type_count();

    memset(state, 0, sizeof(*state));
    g_adapters = state;
    state->first_port[1] = 1;

    port1_adapters[0].id = ADAPTER_MULTITAP;
    port1_adapters[0].desc = "Multitap";
    port1_adapters[0].sub_ports = 4;
    port1_adapters[0].flags = 0;
    port1_adapters[0].types = types;
    port1_adapters[0].num_types = type_count;
    port1_adapters[1].id = ADAPTER_FOUR_SCORE;
    port1_adapters[1].desc = "Four Score";
    port1_adapters[1].sub_ports = 4;
    port1_adapters[1].flags = RETRO_EXT_ADAPTER_TAKES_NEXT_PORT;
    port1_adapters[1].types = four_score_types;
    port1_adapters[1].num_types = 2;
    port2_adapters[0] = port1_adapters[0];

    ports[0].desc = "Port 1";
    ports[0].types = types;
    ports[0].num_types = type_count;
    ports[0].adapters = port1_adapters;
    ports[0].num_adapters = 2;
    ports[1].desc = "Port 2";
    ports[1].types = types;
    ports[1].num_types = type_count;
    ports[1].adapters = port2_adapters;
    ports[1].num_adapters = 1;

    description.ports = ports;
    description.num_ports = ADAPTERS_PHYSICAL_PORTS;
    description.set_adapters = set_adapters;
    state->supported = environment(RETRO_ENVIRONMENT_EXT_SET_PORT_ADAPTERS, &description);
}

void adapters_suggest(const AdaptersState* state, retro_environment_t environment)
{
    unsigned wanted[ADAPTERS_PHYSICAL_PORTS] = {0, ADAPTER_MULTITAP};
    if (state->supported)
    {
        environment(RETRO_ENVIRONMENT_EXT_SUGGEST_PORT_ADAPTERS, wanted);
    }
}

#else

/* Built without libretro_extensions.h: the frontend is never asked. */
void adapters_offer(AdaptersState* state, retro_environment_t environment)
{
    (void)environment;
    memset(state, 0, sizeof(*state));
}

void adapters_suggest(const AdaptersState* state, retro_environment_t environment)
{
    (void)state;
    (void)environment;
}

#endif

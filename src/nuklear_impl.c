/*
 * The one file that compiles Nuklear and its software (rawfb) renderer.
 * Both are third-party code, so CMake builds this file without our strict
 * warning flags. Our own code never goes in here.
 */

#define NK_IMPLEMENTATION
#define NK_RAWFB_IMPLEMENTATION
#include "nuklear_config.h"

struct nk_context* nuklear_rawfb_get_context(struct rawfb_context* rawfb)
{
    return &rawfb->ctx;
}

/*
 * Upstream nk_rawfb_shutdown() frees the context but not the font atlas that
 * nk_rawfb_init() baked (~50 KB leaked per load; found with LeakSanitizer).
 * deps/ is never edited, so the fix lives here: clear the atlas first.
 */
void nuklear_rawfb_shutdown(struct rawfb_context* rawfb)
{
    if (rawfb == NULL)
    {
        return;
    }
    nk_font_atlas_clear(&rawfb->atlas);
    nk_rawfb_shutdown(rawfb);
}

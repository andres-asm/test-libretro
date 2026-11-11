#ifndef TEST_WIZARD_NUKLEAR_CONFIG_H
#define TEST_WIZARD_NUKLEAR_CONFIG_H

/*
 * Every file that includes nuklear.h must see exactly the same NK_* options,
 * otherwise struct layouts differ between files. So the options live here and
 * nothing includes nuklear.h directly.
 *
 * These are build switches, not code generation (allowed by STYLE.md).
 */
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_INCLUDE_SOFTWARE_FONT

#include "nuklear.h"
#include "nuklear_rawfb.h"

/*
 * struct rawfb_context is only defined inside the rawfb implementation, so
 * other files can't reach its nk_context. nuklear_impl.c provides this.
 */
struct nk_context* nuklear_rawfb_get_context(struct rawfb_context* rawfb);

/* Use instead of nk_rawfb_shutdown(): also frees the font atlas (see nuklear_impl.c). */
void nuklear_rawfb_shutdown(struct rawfb_context* rawfb);

#endif

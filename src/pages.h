#ifndef TEST_LIBRETRO_PAGES_H
#define TEST_LIBRETRO_PAGES_H

/*
 * The test pages and moving between them.
 *
 * Controls on every page:
 *   hold Start  (1 s)    pass this page, go to the next
 *   hold Select (1 s)    fail this page, go to the next
 *   hold L / R  (0.5 s)  the previous / next page (no result)
 * The Pass / Fail / Back buttons can also be clicked with a mouse or pointer.
 */

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum PageId
{
    PAGE_CALLBACKS,
    PAGE_VIDEO,
    PAGE_AUDIO,
    PAGE_RETROPAD,
    PAGE_CONTROLLERS,
    PAGE_KEYBOARD,
    PAGE_MOUSE,
    PAGE_SRAM,
    PAGE_SAVESTATE,
    PAGE_SUMMARY,
    PAGE_COUNT
} PageId;

typedef enum TestResult
{
    RESULT_PENDING,
    RESULT_PASS,
    RESULT_FAIL
} TestResult;

typedef struct Pages
{
    PageId     page;
    TestResult results[PAGE_COUNT];   /* PAGE_SUMMARY's entry is unused */
    bool       waiting_for_release;   /* after a hold triggered: ignore Start/Select until both are released */
    bool       summary_logged;
    bool       shutdown_requested;
    PageId     last_drawn_page;       /* to notice entering a page (focus its text field) */
    bool       drawn_once;
    int        retropad_port;         /* the port the RetroPad page shows (0..7) */
    int        controllers_port;      /* the port the Controllers page shows */
} Pages;

struct CoreState;

/* Called once per retro_run, after input was read. Builds and renders the UI. */
void pages_run_frame(struct CoreState* core);

const char* pages_title(PageId page);

#ifdef __cplusplus
}
#endif

#endif

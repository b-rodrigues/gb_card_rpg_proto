#ifndef UI_MENU_H
#define UI_MENU_H

#include <stdint.h>
#include <stdbool.h>

#define MENU_WIDTH 20

/* A reusable menu frame: a centered title plus a bounded content area.
 * Every menu screen (the quick screen, the shop, future menus/subscreens)
 * draws through this so they all share one layout and a subscreen can be
 * shown inside a frame's content area.
 *
 * IMPORTANT: the caller instantiates the MenuFrame LOCALLY and passes the
 * title (and all text) as direct string literals.  Never store titles in a
 * file-scope `const char *[]` pointer table - the linker can place such a
 * table in a ROM bank that is not mapped when the screen draws, which makes
 * the text render blank (see AGENTS.md 54.6). */
typedef struct {
    const char *title;
    uint8_t title_row;
    uint8_t top_row;      /* first content row (inclusive) */
    uint8_t bottom_row;   /* one past the last content row */
} MenuFrame;

/* Clear the screen and draw the frame: centered title at title_row with a
 * separator line directly below it. */
void menu_draw_frame(const MenuFrame *frame);

/* Draw one content line: idx 0 is top_row; out-of-range idx is ignored. */
void menu_draw_content(const MenuFrame *frame, uint8_t idx, const char *text);

/* Absolute row for a content index (top_row + idx).  Callers pass indices
 * in [0, bottom_row - top_row); drawing is clamped by menu_draw_content. */
uint8_t menu_row(const MenuFrame *frame, uint8_t idx);

/* Draw text centered on the 20-column line. */
void menu_draw_centered(uint8_t y, const char *text);

#endif /* UI_MENU_H */

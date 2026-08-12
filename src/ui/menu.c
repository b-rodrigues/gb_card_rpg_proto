#include "ui.h"
#include "menu.h"

void menu_draw_frame(const MenuFrame *frame)
{
    if (!frame) return;
    ui_clear_screen();
    menu_draw_centered(frame->title_row, frame->title);
    ui_draw_text_line(0, (uint8_t)(frame->title_row + 1), "--------------------", MENU_WIDTH);
    if (frame->boxed) {
        uint8_t y;
        for (y = frame->top_row; y < frame->bottom_row; y++) {
            ui_draw_text_line(0, y, "|", 1);
            ui_draw_text_line(19, y, "|", 1);
        }
    }
}

void menu_draw_content(const MenuFrame *frame, uint8_t idx, const char *text)
{
    uint8_t y;
    if (!frame) return;
    if (idx >= (uint8_t)(frame->bottom_row - frame->top_row)) return;
    y = (uint8_t)(frame->top_row + idx);
    ui_draw_text_line(0, y, text, MENU_WIDTH);
}

void menu_clear_content(const MenuFrame *frame)
{
    uint8_t y;
    if (!frame) return;
    for (y = frame->top_row; y < frame->bottom_row; y++) {
        ui_draw_text_line(0, y, "", MENU_WIDTH);
    }
}

uint8_t menu_row(const MenuFrame *frame, uint8_t idx)
{
    if (!frame) return 0;
    return (uint8_t)(frame->top_row + idx);
}

void menu_draw_centered(uint8_t y, const char *text)
{
    uint8_t len = 0;
    uint8_t x;
    if (!text) return;
    while (text[len] != '\0' && len < MENU_WIDTH) len++;
    x = (uint8_t)((MENU_WIDTH - len) >> 1);   /* 0 when len == MENU_WIDTH */
    ui_draw_text_line(x, y, text, MENU_WIDTH);
}

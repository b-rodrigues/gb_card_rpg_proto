#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>

#define BOARD_WIDTH  10
#define BOARD_HEIGHT 16
#define BOARD_OFFSET_X 1
#define BOARD_OFFSET_Y 1

extern uint8_t board[BOARD_HEIGHT][BOARD_WIDTH];

void board_init(void);
uint8_t board_is_occupied(int8_t x, int8_t y);
void board_set_cell(uint8_t x, uint8_t y, uint8_t val);
uint8_t board_check_and_clear_lines(void);

#endif /* BOARD_H */

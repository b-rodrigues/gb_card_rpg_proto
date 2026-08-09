#ifndef RENDER_H
#define RENDER_H

#include <stdint.h>
#include "board.h"
#include "piece.h"

void render_init(void);
void render_clear_screen(void);
void render_title_screen(uint8_t menu_index);
void render_title_menu_update(uint8_t menu_index);
void render_options_screen(uint8_t music_enabled);
void render_playfield_layout(void);
void render_board(void);
void render_piece(const Piece *p);
void render_clear_piece(const Piece *p);
void render_next_piece(uint8_t next_type);
void render_hold_piece(uint8_t hold_type);
void render_line_clear_flicker(const uint8_t *full_rows, uint8_t num_full);
void render_ui(uint16_t score, uint16_t lines, uint8_t game_state);
void render_vsync(void);

#endif /* RENDER_H */

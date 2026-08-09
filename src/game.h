#ifndef GAME_H
#define GAME_H

#include <stdint.h>
#include <gb/gb.h>
#include "board.h"
#include "piece.h"
#include "input.h"
#include "render.h"

#define NO_PIECE 255

typedef enum {
    GAME_STATE_TITLE,
    GAME_STATE_OPTIONS,
    GAME_STATE_PLAYING,
    GAME_STATE_PAUSED,
    GAME_STATE_GAME_OVER
} GameState;

typedef struct {
    GameState state;
    uint8_t menu_index;
    Piece current_piece;
    uint8_t next_type;
    uint8_t hold_type;
    uint8_t can_hold;
    uint16_t score;
    uint16_t lines;
    uint8_t fall_timer;
    uint8_t fall_speed;
    uint8_t random_seed;
} Game;

extern Game g_game;

void game_init(void);
void game_start_play(void);
void game_update(void);
void game_render(void);

#endif /* GAME_H */

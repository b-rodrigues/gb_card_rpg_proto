#ifndef PIECE_H
#define PIECE_H

#include <stdint.h>
#include "board.h"

typedef enum {
    PIECE_I = 0,
    PIECE_J,
    PIECE_L,
    PIECE_O,
    PIECE_S,
    PIECE_T,
    PIECE_Z,
    NUM_PIECE_TYPES
} PieceType;

typedef struct {
    int8_t x;
    int8_t y;
    uint8_t type;
    uint8_t rotation;
} Piece;

void piece_spawn(Piece *p, uint8_t type);
void piece_copy(Piece *dest, const Piece *src);
void piece_get_blocks(const Piece *p, int8_t out_x[4], int8_t out_y[4]);
uint8_t piece_collides(const Piece *p);
void piece_lock(const Piece *p);

#endif /* PIECE_H */

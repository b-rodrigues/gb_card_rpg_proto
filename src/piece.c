#include "piece.h"

/* Relative (dx, dy) offsets for 4 blocks per piece type & 4 rotations */
static const int8_t PIECE_OFFSETS[NUM_PIECE_TYPES][4][4][2] = {
    /* PIECE_I */
    {
        { {0, 1}, {1, 1}, {2, 1}, {3, 1} },
        { {2, 0}, {2, 1}, {2, 2}, {2, 3} },
        { {0, 2}, {1, 2}, {2, 2}, {3, 2} },
        { {1, 0}, {1, 1}, {1, 2}, {1, 3} }
    },
    /* PIECE_J */
    {
        { {0, 0}, {0, 1}, {1, 1}, {2, 1} },
        { {1, 0}, {2, 0}, {1, 1}, {1, 2} },
        { {0, 1}, {1, 1}, {2, 1}, {2, 2} },
        { {1, 0}, {1, 1}, {0, 2}, {1, 2} }
    },
    /* PIECE_L */
    {
        { {2, 0}, {0, 1}, {1, 1}, {2, 1} },
        { {1, 0}, {1, 1}, {1, 2}, {2, 2} },
        { {0, 1}, {1, 1}, {2, 1}, {0, 2} },
        { {0, 0}, {1, 0}, {1, 1}, {1, 2} }
    },
    /* PIECE_O */
    {
        { {1, 0}, {2, 0}, {1, 1}, {2, 1} },
        { {1, 0}, {2, 0}, {1, 1}, {2, 1} },
        { {1, 0}, {2, 0}, {1, 1}, {2, 1} },
        { {1, 0}, {2, 0}, {1, 1}, {2, 1} }
    },
    /* PIECE_S */
    {
        { {1, 0}, {2, 0}, {0, 1}, {1, 1} },
        { {1, 0}, {1, 1}, {2, 1}, {2, 2} },
        { {1, 1}, {2, 1}, {0, 2}, {1, 2} },
        { {0, 0}, {0, 1}, {1, 1}, {1, 2} }
    },
    /* PIECE_T */
    {
        { {1, 0}, {0, 1}, {1, 1}, {2, 1} },
        { {1, 0}, {1, 1}, {2, 1}, {1, 2} },
        { {0, 1}, {1, 1}, {2, 1}, {1, 2} },
        { {1, 0}, {0, 1}, {1, 1}, {1, 2} }
    },
    /* PIECE_Z */
    {
        { {0, 0}, {1, 0}, {1, 1}, {2, 1} },
        { {2, 0}, {1, 1}, {2, 1}, {1, 2} },
        { {0, 1}, {1, 1}, {1, 2}, {2, 2} },
        { {1, 0}, {0, 1}, {1, 1}, {0, 2} }
    }
};

void piece_spawn(Piece *p, uint8_t type) {
    p->type = type % NUM_PIECE_TYPES;
    p->rotation = 0;
    p->x = (BOARD_WIDTH / 2) - 2;
    p->y = 0;
}

void piece_copy(Piece *dest, const Piece *src) {
    dest->x = src->x;
    dest->y = src->y;
    dest->type = src->type;
    dest->rotation = src->rotation;
}

void piece_get_blocks(const Piece *p, int8_t out_x[4], int8_t out_y[4]) {
    uint8_t i;
    for (i = 0; i < 4; i++) {
        out_x[i] = p->x + PIECE_OFFSETS[p->type][p->rotation][i][0];
        out_y[i] = p->y + PIECE_OFFSETS[p->type][p->rotation][i][1];
    }
}

uint8_t piece_collides(const Piece *p) {
    int8_t bx[4], by[4];
    uint8_t i;
    piece_get_blocks(p, bx, by);
    for (i = 0; i < 4; i++) {
        if (board_is_occupied(bx[i], by[i])) {
            return 1;
        }
    }
    return 0;
}

void piece_lock(const Piece *p) {
    int8_t bx[4], by[4];
    uint8_t i;
    piece_get_blocks(p, bx, by);
    for (i = 0; i < 4; i++) {
        if (by[i] >= 0 && by[i] < BOARD_HEIGHT && bx[i] >= 0 && bx[i] < BOARD_WIDTH) {
            board_set_cell(bx[i], by[i], p->type + 1);
        }
    }
}

#ifndef WORLD_H
#define WORLD_H

#include "entity.h"
#include <stdbool.h>

#define WORLD_WIDTH  20
#define WORLD_HEIGHT 14

typedef enum {
    TILE_FLOOR,
    TILE_WALL
} TileType;

typedef struct {
    uint8_t width;
    uint8_t height;
    TileType map[WORLD_HEIGHT][WORLD_WIDTH];
    Entity player;
    Entity enemy;
    bool encounter_triggered;
} World;

void world_init(World *w);
bool world_is_walkable(const World *w, uint8_t x, uint8_t y);
void world_move_player(World *w, int8_t dx, int8_t dy);
void world_reset_encounter(World *w);

#endif /* WORLD_H */

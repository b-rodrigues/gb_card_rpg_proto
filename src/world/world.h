#ifndef WORLD_H
#define WORLD_H

#include "entity.h"
#include <stdbool.h>

#define WORLD_WIDTH  20
#define WORLD_HEIGHT 14

typedef enum {
    MAP_FIELD
} MapId;

typedef enum {
    TILE_FLOOR,
    TILE_WALL
} TileType;

typedef struct {
    uint8_t width;
    uint8_t height;
    bool encounter_triggered;
    Entity player;
    Entity enemy;
    uint8_t map[WORLD_HEIGHT][WORLD_WIDTH];
} World;

void world_init(World *w);
bool world_is_walkable(const World *w, uint8_t x, uint8_t y);
void world_move_player(World *w, int8_t dx, int8_t dy);
void world_on_battle_end(World *w, bool victory);
void world_set_player_pos(World *w, uint8_t x, uint8_t y);
void world_set_enemy_pos(World *w, uint8_t x, uint8_t y);

#endif /* WORLD_H */

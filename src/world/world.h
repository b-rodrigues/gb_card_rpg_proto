#ifndef WORLD_H
#define WORLD_H

#include "entity.h"
#include <stdbool.h>

#define WORLD_WIDTH  20
#define WORLD_HEIGHT 12

typedef enum {
    MOVE_RESULT_NONE        = 0,
    MOVE_RESULT_BLOCKED     = 1,
    MOVE_RESULT_MOVED       = 2,
    MOVE_RESULT_MAP_CHANGED = 3,
    MOVE_RESULT_ENCOUNTER   = 4
} WorldMoveResult;

typedef enum {
    MAP_FIELD         = 0,
    MAP_TOWN          = 1,
    MAP_FOREST        = 2,
    MAP_MOUNTAIN_PASS = 3,
    MAP_CASTLE        = 4
} MapId;

typedef enum {
    TILE_FLOOR              = 0,
    TILE_WALL               = 1,
    TILE_FIELD_EXIT         = 2,
    TILE_TOWN_EXIT          = 3,
    TILE_BUILDING           = 4,
    TILE_EXIT_FIELD_FOREST  = 5,
    TILE_EXIT_FOREST_FIELD  = 6,
    TILE_EXIT_FOREST_MOUNTAIN = 7,
    TILE_EXIT_MOUNTAIN_FOREST = 8,
    TILE_EXIT_MOUNTAIN_CASTLE = 9,
    TILE_EXIT_CASTLE_MOUNTAIN = 10
} TileType;

typedef struct {
    uint8_t width;
    uint8_t height;
    MapId map_id;
    bool encounter_triggered;
    bool map_changed;
    Entity player;
    Entity enemy;
    uint8_t map[WORLD_HEIGHT][WORLD_WIDTH];
} World;

void world_init(World *w);
void world_load_map(World *w, MapId map_id);
void world_change_map(World *w, MapId map_id, uint8_t spawn_x, uint8_t spawn_y);
bool world_is_walkable(const World *w, uint8_t x, uint8_t y);
WorldMoveResult world_move_player(World *w, int8_t dx, int8_t dy);
void world_on_battle_end(World *w, bool victory);
void world_set_player_pos(World *w, uint8_t x, uint8_t y);
void world_set_enemy_pos(World *w, uint8_t x, uint8_t y);
void world_set_player_facing(World *w, Direction facing);

#endif /* WORLD_H */

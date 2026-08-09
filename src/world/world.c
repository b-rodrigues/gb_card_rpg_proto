#include "world.h"
#include "telemetry.h"
#include "game.h"

extern Game g_game;

void world_load_map(World *w, MapId map_id)
{
    uint8_t x, y;
    if (!w) return;

    w->width = WORLD_WIDTH;
    w->height = WORLD_HEIGHT;
    w->map_id = map_id;
    w->encounter_triggered = false;

    for (y = 0; y < WORLD_HEIGHT; y++) {
        for (x = 0; x < WORLD_WIDTH; x++) {
            if (y == 0 || y == WORLD_HEIGHT - 1 || x == 0 || x == WORLD_WIDTH - 1) {
                w->map[y][x] = TILE_WALL;
            } else {
                w->map[y][x] = TILE_FLOOR;
            }
        }
    }

    if (map_id == MAP_FIELD) {
        /* Exit gate to Town on East wall (18, 7) */
        w->map[7][18] = TILE_FIELD_EXIT;
        /* Enable enemy slime_01 on Field */
        w->enemy.active = true;
        w->enemy.position.x = 14;
        w->enemy.position.y = 8;
    } else if (map_id == MAP_TOWN) {
        /* Exit gate to Field on West wall (1, 7) */
        w->map[7][1] = TILE_TOWN_EXIT;
        /* Town buildings */
        for (y = 3; y <= 6; y++) {
            for (x = 4; x <= 8; x++) {
                w->map[y][x] = TILE_BUILDING;
            }
            for (x = 12; x <= 16; x++) {
                w->map[y][x] = TILE_BUILDING;
            }
        }
        /* Deactivate wild enemy in Town */
        w->enemy.active = false;
    }
}

void world_init(World *w)
{
    if (!w) return;
    world_load_map(w, MAP_FIELD);
    entity_init(&w->player, ENTITY_PLAYER, "player", 4, 4, 10, 10);
    entity_init(&w->enemy, ENTITY_ENEMY, "slime_01", 14, 8, 5, 5);
}

void world_change_map(World *w, MapId map_id, uint8_t spawn_x, uint8_t spawn_y)
{
    MapId old_map;
    if (!w) return;
    old_map = w->map_id;
    world_load_map(w, map_id);
    w->player.position.x = spawn_x;
    w->player.position.y = spawn_y;
    telemetry_emit(EVENT_MAP_CHANGED, (uint8_t)old_map, (uint8_t)map_id, spawn_x, spawn_y);

    if (map_id == MAP_TOWN && !(g_game.story_flags & STORY_FLAG_ARRIVED_TOWN)) {
        g_game.story_flags |= STORY_FLAG_ARRIVED_TOWN;
        telemetry_emit(EVENT_STORY_FLAG_SET, 0, 0, 0, 0);
    }
}

bool world_is_walkable(const World *w, uint8_t x, uint8_t y)
{
    uint8_t tile;
    if (!w) return false;
    if (x >= WORLD_WIDTH || y >= WORLD_HEIGHT) return false;
    tile = w->map[y][x];
    if (tile == TILE_WALL || tile == TILE_BUILDING) {
        return false;
    }
    return true;
}

void world_move_player(World *w, int8_t dx, int8_t dy)
{
    uint8_t old_x, old_y;
    uint8_t target_x, target_y;
    uint8_t target_tile;

    if (!w) return;

    if (dy < 0) w->player.facing = DIRECTION_UP;
    else if (dy > 0) w->player.facing = DIRECTION_DOWN;
    else if (dx < 0) w->player.facing = DIRECTION_LEFT;
    else if (dx > 0) w->player.facing = DIRECTION_RIGHT;

    target_x = (uint8_t)((int16_t)w->player.position.x + dx);
    target_y = (uint8_t)((int16_t)w->player.position.y + dy);

    if (!world_is_walkable(w, target_x, target_y)) {
        return;
    }

    target_tile = w->map[target_y][target_x];

    if (target_tile == TILE_FIELD_EXIT) {
        world_change_map(w, MAP_TOWN, 2, 7);
        ui_draw_world_full(w);
        return;
    } else if (target_tile == TILE_TOWN_EXIT) {
        world_change_map(w, MAP_FIELD, 17, 7);
        ui_draw_world_full(w);
        return;
    }

    if (w->enemy.active && target_x == w->enemy.position.x && target_y == w->enemy.position.y) {
        telemetry_emit(EVENT_COLLISION, target_x, target_y, 0, 0);
        telemetry_emit(EVENT_ENCOUNTER_STARTED, w->enemy.type, 0, 0, 0);
        w->encounter_triggered = true;
        return;
    }

    old_x = w->player.position.x;
    old_y = w->player.position.y;
    w->player.position.x = target_x;
    w->player.position.y = target_y;
    telemetry_emit(EVENT_PLAYER_MOVED, old_x, old_y, target_x, target_y);
}

void world_on_battle_end(World *w, bool victory)
{
    if (!w) return;
    w->encounter_triggered = false;
    if (victory) {
        w->enemy.active = false;
        w->enemy.hp = 0;
        telemetry_emit(EVENT_ENTITY_DEFEATED, 0, 0, 0, 0);
    }
}

void world_set_player_pos(World *w, uint8_t x, uint8_t y)
{
    if (!w) return;
    w->player.position.x = x;
    w->player.position.y = y;
}

void world_set_enemy_pos(World *w, uint8_t x, uint8_t y)
{
    if (!w) return;
    w->enemy.position.x = x;
    w->enemy.position.y = y;
}

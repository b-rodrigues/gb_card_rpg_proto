#include "world.h"
#include "telemetry.h"
#include "actor.h"

void world_load_map(World *w, MapId map_id)
{
    uint8_t x, y;
    if (!w) return;

    w->width = WORLD_WIDTH;
    w->height = WORLD_HEIGHT;
    w->map_id = map_id;
    w->encounter_triggered = false;
    w->map_changed = false;

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
    } else if (map_id == MAP_FOREST) {
        /* Forest: open floor with scattered tree clusters. */
        static const uint8_t trees[][2] = {{4,2},{5,2},{4,3},{10,6},{11,6},{9,7},
                                           {14,2},{15,2},{3,8},{8,9},{13,9}};
        uint8_t i;
        for (i = 0; i < sizeof(trees)/sizeof(trees[0]); i++) {
            w->map[trees[i][1]][trees[i][0]] = TILE_WALL;
        }
    } else if (map_id == MAP_MOUNTAIN_PASS) {
        /* Narrow winding pass with rock walls on both sides. */
        for (y = 0; y < WORLD_HEIGHT; y++) {
            for (x = 0; x < WORLD_WIDTH; x++) {
                if (x <= 3 || x >= 16) {
                    w->map[y][x] = TILE_WALL;
                } else if (y % 4 == 2 && (x == 9 || x == 10)) {
                    w->map[y][x] = TILE_WALL;  /* pinch point */
                }
            }
        }
    } else if (map_id == MAP_CASTLE) {
        /* Castle interior: buildings flanking a central hall. */
        for (y = 3; y <= 8; y++) {
            for (x = 3; x <= 6; x++) {
                w->map[y][x] = TILE_BUILDING;
            }
            for (x = 13; x <= 16; x++) {
                w->map[y][x] = TILE_BUILDING;
            }
        }
    }

    /* Scene data determines what actors exist here. */
    actor_load_scene(w, map_id);
}

void world_init(World *w)
{
    if (!w) return;
    entity_init(&w->player, ENTITY_ID_PLAYER, 4, 4, 10, 10);
    world_load_map(w, MAP_FIELD);
}

void world_change_map(World *w, MapId map_id, uint8_t spawn_x, uint8_t spawn_y)
{
    MapId old_map;
    if (!w) return;
    old_map = w->map_id;
    world_load_map(w, map_id);
    w->player.position.x = spawn_x;
    w->player.position.y = spawn_y;
    w->map_changed = true;
    telemetry_emit(EVENT_MAP_CHANGED, (uint8_t)old_map, (uint8_t)map_id, spawn_x, spawn_y);
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

WorldMoveResult world_move_player(World *w, int8_t dx, int8_t dy)
{
    uint8_t old_x, old_y;
    uint8_t target_x, target_y;
    uint8_t target_tile;
    const WorldActorDefinition *actor;

    if (!w) return MOVE_RESULT_NONE;

    if (dy < 0) w->player.facing = DIRECTION_UP;
    else if (dy > 0) w->player.facing = DIRECTION_DOWN;
    else if (dx < 0) w->player.facing = DIRECTION_LEFT;
    else if (dx > 0) w->player.facing = DIRECTION_RIGHT;

    target_x = (uint8_t)((int16_t)w->player.position.x + dx);
    target_y = (uint8_t)((int16_t)w->player.position.y + dy);

    if (!world_is_walkable(w, target_x, target_y)) {
        return MOVE_RESULT_BLOCKED;
    }

    target_tile = w->map[target_y][target_x];

    if (target_tile == TILE_FIELD_EXIT) {
        world_change_map(w, MAP_TOWN, 2, 7);
        return MOVE_RESULT_MAP_CHANGED;
    } else if (target_tile == TILE_TOWN_EXIT) {
        world_change_map(w, MAP_FIELD, 17, 7);
        return MOVE_RESULT_MAP_CHANGED;
    }

    /* Generic World Actor collision: the movement code does not care
     * whether this is a Mayor, a Guard, a Slime or a Bat. */
    actor = actor_find_at(w, target_x, target_y);
    if (actor) {
        telemetry_emit(EVENT_ACTOR_COLLISION, target_x, target_y, (uint8_t)actor->id, 0);
        if (actor->flags & ACTOR_FLAG_HOSTILE) {
            telemetry_emit(EVENT_ENCOUNTER_STARTED, (uint8_t)actor->id, 0, 0, 0);
            w->encounter_triggered = true;
            return MOVE_RESULT_ENCOUNTER;
        }
        return MOVE_RESULT_BLOCKED;
    }

    old_x = w->player.position.x;
    old_y = w->player.position.y;
    w->player.position.x = target_x;
    w->player.position.y = target_y;
    telemetry_emit(EVENT_PLAYER_MOVED, old_x, old_y, target_x, target_y);
    return MOVE_RESULT_MOVED;
}

void world_on_battle_end(World *w, bool victory)
{
    if (!w) return;
    w->encounter_triggered = false;
    if (victory) {
        w->enemy.active = false;
        w->enemy.hp = 0;
        telemetry_emit(EVENT_ENTITY_DEFEATED, (uint8_t)w->enemy.id, 0, 0, 0);
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

void world_set_player_facing(World *w, Direction facing)
{
    if (!w) return;
    w->player.facing = facing;
}

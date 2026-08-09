#include "world.h"
#include "telemetry.h"

void world_init(World *w)
{
    uint8_t x, y;
    if (!w) return;

    w->width = WORLD_WIDTH;
    w->height = WORLD_HEIGHT;
    w->encounter_triggered = false;

    /* Fill map: perimeter walls, interior floor */
    for (y = 0; y < WORLD_HEIGHT; y++) {
        for (x = 0; x < WORLD_WIDTH; x++) {
            if (y == 0 || y == WORLD_HEIGHT - 1 || x == 0 || x == WORLD_WIDTH - 1) {
                w->map[y][x] = TILE_WALL;
            } else {
                w->map[y][x] = TILE_FLOOR;
            }
        }
    }

    /* Initialize entities */
    entity_init(&w->player, ENTITY_PLAYER, "player", 4, 4, 10, 10);
    entity_init(&w->enemy, ENTITY_ENEMY, "slime_01", 14, 8, 5, 5);
}

bool world_is_walkable(const World *w, uint8_t x, uint8_t y)
{
    if (!w) return false;
    if (x >= WORLD_WIDTH || y >= WORLD_HEIGHT) return false;
    return w->map[y][x] == TILE_FLOOR;
}

void world_move_player(World *w, int8_t dx, int8_t dy)
{
    uint8_t old_x, old_y;
    uint8_t target_x, target_y;
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

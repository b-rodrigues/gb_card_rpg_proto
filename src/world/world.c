#include "world.h"

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
    entity_init(&w->player, ENTITY_PLAYER, 4, 4, 10, 10);
    entity_init(&w->enemy, ENTITY_ENEMY, 14, 8, 5, 5);
}

bool world_is_walkable(const World *w, uint8_t x, uint8_t y)
{
    if (!w) return false;
    if (x >= WORLD_WIDTH || y >= WORLD_HEIGHT) return false;
    return w->map[y][x] == TILE_FLOOR;
}

void world_move_player(World *w, int8_t dx, int8_t dy)
{
    uint8_t target_x, target_y;
    if (!w) return;

    target_x = (uint8_t)((int16_t)w->player.position.x + dx);
    target_y = (uint8_t)((int16_t)w->player.position.y + dy);

    if (!world_is_walkable(w, target_x, target_y)) {
        return;
    }

    if (w->enemy.active && target_x == w->enemy.position.x && target_y == w->enemy.position.y) {
        w->encounter_triggered = true;
        return;
    }

    w->player.position.x = target_x;
    w->player.position.y = target_y;
}

void world_reset_encounter(World *w)
{
    if (!w) return;
    w->encounter_triggered = false;
    w->player.position.x = 4;
    w->player.position.y = 4;
    w->enemy.hp = 5;
    w->enemy.active = true;
}

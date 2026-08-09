#include "entity.h"

void entity_init(Entity *e, EntityType type, const char *id, uint8_t x, uint8_t y, uint8_t hp, uint8_t max_hp)
{
    if (!e) return;
    e->id = id;
    e->position.x = x;
    e->position.y = y;
    e->facing = DIRECTION_DOWN;
    e->type = type;
    e->hp = hp;
    e->max_hp = max_hp;
    e->active = true;
}

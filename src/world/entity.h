#ifndef ENTITY_H
#define ENTITY_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t x;
    uint8_t y;
} Position;

typedef enum {
    DIRECTION_UP,
    DIRECTION_DOWN,
    DIRECTION_LEFT,
    DIRECTION_RIGHT
} Direction;

typedef enum {
    ENTITY_ID_NONE = 0,
    ENTITY_ID_PLAYER = 1,
    ENTITY_ID_SLIME = 2,
    ENTITY_ID_MAYOR = 3,
    ENTITY_ID_GUARD = 4
} EntityId;

typedef enum {
    ENTITY_PLAYER,
    ENTITY_ENEMY,
    ENTITY_NPC
} EntityType;

typedef struct {
    Position position;
    uint8_t hp;
    uint8_t max_hp;
    bool active;
    EntityType type;
    Direction facing;
    const char *id;
} Entity;

void entity_init(Entity *e, EntityType type, const char *id, uint8_t x, uint8_t y, uint8_t hp, uint8_t max_hp);

#endif /* ENTITY_H */

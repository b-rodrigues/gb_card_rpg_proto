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

/* Entity identity.  The engine defines only the sentinels it needs plus the
 * start of the per-game content range; each game names its own entity types
 * in src/game/game_ids.h using values >= ENTITY_ID_FIRST_GAME. */
typedef uint8_t EntityId;

#define ENTITY_ID_NONE       0
#define ENTITY_ID_PLAYER     1
#define ENTITY_ID_FIRST_GAME 0x80

typedef struct {
    Position position;
    uint8_t hp;
    uint8_t max_hp;
    bool active;
    Direction facing;
    EntityId id;
} Entity;

void entity_init(Entity *e, EntityId id, uint8_t x, uint8_t y, uint8_t hp, uint8_t max_hp);

#endif /* ENTITY_H */

#ifndef NPC_H
#define NPC_H

#include <stdint.h>
#include <stdbool.h>
#include "world.h"
#include "entity.h"
#include "dialogue.h"

typedef struct {
    EntityId id;
    const char *name;
    MapId map_id;
    uint8_t x;
    uint8_t y;
    bool active;
    DialogueId dialogue_id;
} NpcDef;

const NpcDef *npc_find_at(MapId map_id, uint8_t x, uint8_t y);
const NpcDef *npc_get_by_id(EntityId id);

#endif /* NPC_H */

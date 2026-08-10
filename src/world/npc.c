#include "npc.h"
#include <stddef.h>

static const NpcDef g_npc_defs[] = {
    {
        ENTITY_ID_MAYOR,
        "Mayor",
        MAP_TOWN,
        10, 5,
        true,
        DIALOGUE_ID_MAYOR_GREETING
    },
    {
        ENTITY_ID_GUARD,
        "Guard",
        MAP_TOWN,
        10, 8,
        true,
        DIALOGUE_ID_GUARD_GREETING
    }
};

#define NUM_NPC_DEFS (sizeof(g_npc_defs) / sizeof(g_npc_defs[0]))

const NpcDef *npc_find_at(MapId map_id, uint8_t x, uint8_t y)
{
    uint8_t i;
    for (i = 0; i < NUM_NPC_DEFS; i++) {
        if (g_npc_defs[i].active &&
            g_npc_defs[i].map_id == map_id &&
            g_npc_defs[i].x == x &&
            g_npc_defs[i].y == y) {
            return &g_npc_defs[i];
        }
    }
    return NULL;
}

const NpcDef *npc_get_by_id(EntityId id)
{
    uint8_t i;
    for (i = 0; i < NUM_NPC_DEFS; i++) {
        if (g_npc_defs[i].id == id) {
            return &g_npc_defs[i];
        }
    }
    return NULL;
}

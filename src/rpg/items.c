#include "rpg/items.h"
#include "rpg/inventory.h"
#include "telemetry.h"
#include <stddef.h>

static const ItemDefinition g_item_defs[] = {
    { ITEM_POTION, "POTION", 20, 5 },
    { ITEM_BOMB,   "BOMB",   50, 0 },
    { ITEM_ETHER,  "ETHER",  40, 0 }
};

#define NUM_ITEM_DEFS (sizeof(g_item_defs) / sizeof(g_item_defs[0]))

const ItemDefinition *item_get_def(ItemId id)
{
    uint8_t i;
    for (i = 0; i < NUM_ITEM_DEFS; i++) {
        if (g_item_defs[i].id == id) {
            return &g_item_defs[i];
        }
    }
    return NULL;
}

bool item_use(GameState *state, ItemId id)
{
    const ItemDefinition *def;
    CharacterState *hero;
    uint8_t healed;
    uint8_t max_heal;

    if (!state) return false;
    def = item_get_def(id);
    if (!def || def->heal == 0) return false;
    if (inventory_count(&state->inventory, id) == 0) return false;

    hero = &state->party.members[0];
    if (hero->hp >= hero->max_hp) return false;

    max_heal = (uint8_t)(hero->max_hp - hero->hp);
    healed = (def->heal < max_heal) ? def->heal : max_heal;
    hero->hp = (uint8_t)(hero->hp + healed);

    inventory_remove(&state->inventory, id, 1);
    telemetry_emit(EVENT_HEALED, healed, (uint8_t)id, 0, 0);
    return true;
}

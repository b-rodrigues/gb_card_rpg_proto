#include "rpg/items.h"
#include "rpg/state.h"

/* ── Item catalog (game content) ─────────────────────────────────── */
static const ItemDefinition g_items[] = {
    { ITEM_POTION, "POTION", 20, ITEM_KIND_CONSUMABLE, ITEM_EFFECT_HEAL_HP, 5, 0 },
    { ITEM_BOMB,   "BOMB",   50, ITEM_KIND_CONSUMABLE, ITEM_EFFECT_NONE,    0, 0 },
    { ITEM_ETHER,  "ETHER",  40, ITEM_KIND_CONSUMABLE, ITEM_EFFECT_NONE,    0, 0 },
    { ITEM_SWORD,  "SWORD",   0, ITEM_KIND_WEAPON,     ITEM_EFFECT_NONE,    0, 3 },
    { ITEM_AMULET, "AMULET",  0, ITEM_KIND_KEY,        ITEM_EFFECT_NONE,    0, 0 },
    { ITEM_NUT,    "NUT",    10, ITEM_KIND_CONSUMABLE, ITEM_EFFECT_HEAL_HP, 5, 0 }
};

void game_items_register(void)
{
    item_register_defs(g_items, (uint8_t)(sizeof(g_items) / sizeof(g_items[0])));
}

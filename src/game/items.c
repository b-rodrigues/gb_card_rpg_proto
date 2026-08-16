#include "rpg/items.h"
#include "game_ids.h"

extern const ItemDefinition g_items[];
#define GAME_ITEM_COUNT 6

void game_items_register(void)
{
    item_register_defs(g_items, GAME_ITEM_COUNT, GAME_CONTENT_BANK);
}

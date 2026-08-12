#include "shops.h"
#include <stddef.h>

/* ── Shop stock lists (game content) ───────────────────────────────
 * Shop 1: the town shopkeeper (POTION).
 * Shop 2: the Lost Merchant -- reachable only after the amulet is returned
 * (the merchant's quest dialogue events block the fallback SHOP interaction
 * until then), sells cheap NUTS. */
static const ShopDefinition g_shops[] = {
    { 1, 1, { ITEM_POTION } },
    { 2, 1, { ITEM_NUT } }
};

const ShopDefinition *game_shop_for_id(uint8_t id)
{
    uint8_t i;
    for (i = 0; i < (uint8_t)(sizeof(g_shops) / sizeof(g_shops[0])); i++) {
        if (g_shops[i].id == id) {
            return &g_shops[i];
        }
    }
    return NULL;
}

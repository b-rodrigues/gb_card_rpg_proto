#include "shops.h"
#include "game_ids.h"
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
    return (id == 2) ? &g_shops[1] : ((id == 1) ? &g_shops[0] : NULL);
}

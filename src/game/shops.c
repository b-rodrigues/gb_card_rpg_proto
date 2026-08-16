#include "shops.h"
#include "game_ids.h"
#include "banked.h"
#include <stddef.h>

extern const ShopDefinition g_shops[];

static ShopDefinition s_shop_scratch;

const ShopDefinition *game_shop_for_id(uint8_t id)
{
    uint8_t idx = (id == 2) ? 1 : ((id == 1) ? 0 : 0xFF);
    if (idx == 0xFF) return NULL;
    banked_copy(GAME_CONTENT_BANK, &s_shop_scratch, &g_shops[idx], sizeof(ShopDefinition));
    return &s_shop_scratch;
}

#ifndef GAME_SHOPS_H
#define GAME_SHOPS_H

#include "rpg/state.h"

#define SHOP_MAX_ITEMS 4

/* A per-shop stock list (game content).  id matches the WorldActorDefinition
 * shop_id of the actor that runs the shop.  Items not sold carry price 0 in
 * the item catalog and are never purchasable. */
typedef struct {
    uint8_t id;
    uint8_t count;
    ItemId items[SHOP_MAX_ITEMS];
} ShopDefinition;

/* Look up a shop's stock by its id, or NULL if unknown. */
const ShopDefinition *game_shop_for_id(uint8_t id);

#endif /* GAME_SHOPS_H */

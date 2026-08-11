#ifndef RPG_ITEMS_H
#define RPG_ITEMS_H

#include <stdint.h>
#include <stdbool.h>
#include "rpg/state.h"

/* Item catalog.  Identity lives in ItemId (state.h); this table adds the
 * gameplay-facing properties (display name, shop price, generic effect).
 * Static data - no mutable state, no per-item code in the mechanics. */

typedef enum {
    ITEM_EFFECT_NONE = 0,
    ITEM_EFFECT_HEAL_HP = 1
} ItemEffectType;

typedef struct {
    ItemId id;
    const char *name;
    uint16_t price;   /* gold cost in the shop (0 = not sold) */
    ItemEffectType effect;
    uint8_t effect_amount;
} ItemDefinition;

typedef enum {
    ITEM_PURCHASE_OK = 0,
    ITEM_PURCHASE_NO_GOLD = 1,
    ITEM_PURCHASE_NO_CAPACITY = 2,
    ITEM_PURCHASE_NOT_SOLD = 3
} ItemPurchaseResult;

const ItemDefinition *item_get_def(ItemId id);

/* True if the item is owned and its effect can currently apply to the
 * target party member. */
bool item_can_use(const GameState *state, ItemId id, CharacterId target);

/* Use a consumable item from GameState.inventory on a target party member.
 * Applies the generic effect (HEAL_HP for now), consuming one unit only if
 * the use succeeds.  Emits ITEM_USED / ITEM_USE_FAILED (plus HEALED and
 * ITEM_REMOVED on success). */
bool item_use(GameState *state, ItemId id, CharacterId target);

/* Atomic shop purchase: checks gold + inventory capacity, then spends
 * currency and adds the item.  On failure the state is unchanged and
 * ITEM_PURCHASE_FAILED is emitted (ITEM_PURCHASED + CURRENCY_SPENT +
 * ITEM_ADDED on success). */
ItemPurchaseResult item_purchase(GameState *state, ItemId id);

#endif /* RPG_ITEMS_H */

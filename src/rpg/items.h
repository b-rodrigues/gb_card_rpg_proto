#ifndef RPG_ITEMS_H
#define RPG_ITEMS_H

#include <stdint.h>
#include <stdbool.h>
#include "rpg/state.h"

/* Item catalog.  Identity lives in ItemId (state.h); this table adds the
 * gameplay-facing properties (display name, shop price, consumable effect).
 * Static data - no mutable state. */
typedef struct {
    ItemId id;
    const char *name;
    uint16_t price;   /* gold cost in the shop (0 = not sold) */
    uint8_t heal;     /* HP restored when used (0 = not a healing consumable) */
} ItemDefinition;

const ItemDefinition *item_get_def(ItemId id);

/* Use a consumable item from GameState.inventory: applies its effect to
 * party member 0 (healing capped at max_hp) and consumes one unit.  Emits
 * EVENT_HEALED and EVENT_ITEM_REMOVED.  Returns false if the item is not
 * owned or cannot be used right now (e.g. already at full HP). */
bool item_use(GameState *state, ItemId id);

#endif /* RPG_ITEMS_H */

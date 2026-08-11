#ifndef RPG_INVENTORY_H
#define RPG_INVENTORY_H

#include <stdint.h>
#include <stdbool.h>
#include "rpg/state.h"

bool inventory_add(InventoryState *inventory, ItemId item_id, uint8_t quantity);
bool inventory_remove(InventoryState *inventory, ItemId item_id, uint8_t quantity);
uint8_t inventory_count(const InventoryState *inventory, ItemId item_id);

#endif /* RPG_INVENTORY_H */

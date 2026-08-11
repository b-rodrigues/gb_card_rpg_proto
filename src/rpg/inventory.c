#include "rpg/inventory.h"
#include <stddef.h>
#include "telemetry.h"

static InventoryEntry *find_entry(InventoryState *inventory, ItemId item_id)
{
    uint8_t i;
    for (i = 0; i < inventory->count; i++) {
        if (inventory->entries[i].item_id == item_id) {
            return &inventory->entries[i];
        }
    }
    return NULL;
}

bool inventory_add(InventoryState *inventory, ItemId item_id, uint8_t quantity)
{
    InventoryEntry *e;
    if (!inventory || item_id == ITEM_NONE || quantity == 0) return false;

    e = find_entry(inventory, item_id);
    if (e) {
        e->quantity = (uint8_t)(e->quantity + quantity);
        telemetry_emit(EVENT_ITEM_ADDED, (uint8_t)item_id, quantity, 0, 0);
        return true;
    }

    if (inventory->count >= MAX_INVENTORY_ITEMS) return false;
    e = &inventory->entries[inventory->count];
    e->item_id = item_id;
    e->quantity = quantity;
    inventory->count++;
    telemetry_emit(EVENT_ITEM_ADDED, (uint8_t)item_id, quantity, 0, 0);
    return true;
}

bool inventory_remove(InventoryState *inventory, ItemId item_id, uint8_t quantity)
{
    InventoryEntry *e;
    uint8_t i;
    if (!inventory || item_id == ITEM_NONE || quantity == 0) return false;

    e = find_entry(inventory, item_id);
    if (!e || e->quantity < quantity) return false;

    if (e->quantity == quantity) {
        /* Remove the slot entirely: swap with last and shrink. */
        for (i = 0; i < inventory->count; i++) {
            if (&inventory->entries[i] == e) {
                inventory->entries[i] = inventory->entries[inventory->count - 1];
                break;
            }
        }
        inventory->count--;
    } else {
        e->quantity = (uint8_t)(e->quantity - quantity);
    }
    telemetry_emit(EVENT_ITEM_REMOVED, (uint8_t)item_id, quantity, 0, 0);
    return true;
}

uint8_t inventory_count(const InventoryState *inventory, ItemId item_id)
{
    InventoryEntry *e;
    if (!inventory) return 0;
    e = find_entry((InventoryState *)inventory, item_id);
    return e ? e->quantity : 0;
}

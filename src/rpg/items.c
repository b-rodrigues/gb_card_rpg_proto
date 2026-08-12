#include "rpg/items.h"
#include "rpg/inventory.h"
#include "rpg/currency.h"
#include "rpg/party.h"
#include "telemetry.h"
#include <stddef.h>

/* The item catalog is game content, registered at boot via
 * item_register_defs() (see src/game/items.c). */
static const ItemDefinition *g_items = NULL;
static uint8_t g_item_count = 0;

void item_register_defs(const ItemDefinition *table, uint8_t count)
{
    g_items = table;
    g_item_count = count;
}

uint8_t item_def_count(void)
{
    return g_item_count;
}

const ItemDefinition *item_get_def_at(uint8_t idx)
{
    if (!g_items || idx >= g_item_count) return NULL;
    return &g_items[idx];
}

const ItemDefinition *item_get_def(ItemId id)
{
    uint8_t i;
    if (!g_items) return NULL;
    for (i = 0; i < g_item_count; i++) {
        if (g_items[i].id == id) {
            return &g_items[i];
        }
    }
    return NULL;
}

static CharacterState *item_target_member(GameState *state, CharacterId target)
{
    CharacterState *member;
    if (!state || state->party.count == 0) return NULL;

    member = party_get_member(&state->party, target);
    if (member) return member;

    /* Fall back to the first member when the target id is not present. */
    return &state->party.members[0];
}

bool item_can_use(const GameState *state, ItemId id, CharacterId target)
{
    const ItemDefinition *def;
    const CharacterState *member;
    if (!state) return false;

    def = item_get_def(id);
    if (!def || def->kind != ITEM_KIND_CONSUMABLE) return false;
    if (def->effect == ITEM_EFFECT_NONE) return false;
    if (inventory_count(&state->inventory, id) == 0) return false;

    member = party_get_member_const(&state->party, target);
    if (!member) member = &state->party.members[0];
    if (member->hp >= member->max_hp) return false;
    return true;
}

bool item_equip(GameState *state, ItemId id)
{
    const ItemDefinition *def;
    if (!state) return false;
    def = item_get_def(id);
    if (!def || def->kind != ITEM_KIND_WEAPON) return false;

    state->equipment.weapon = id;
    telemetry_emit(EVENT_ITEM_EQUIPPED, (uint8_t)id, 0, 0, 0);
    return true;
}

bool item_use(GameState *state, ItemId id, CharacterId target)
{
    const ItemDefinition *def;
    CharacterState *member;
    uint8_t healed;
    uint8_t max_heal;
    bool ok = false;

    if (!state) return false;
    def = item_get_def(id);
    if (!def) {
        telemetry_emit(EVENT_ITEM_USE_FAILED, (uint8_t)id, 0, 0, 0);
        return false;
    }
    if (inventory_count(&state->inventory, id) == 0) {
        telemetry_emit(EVENT_ITEM_USE_FAILED, (uint8_t)id, 1, 0, 0);
        return false;
    }

    switch (def->effect) {
        case ITEM_EFFECT_HEAL_HP:
            member = item_target_member(state, target);
            if (!member || member->hp >= member->max_hp) {
                telemetry_emit(EVENT_ITEM_USE_FAILED, (uint8_t)id, 2, 0, 0);
                return false;
            }
            max_heal = (uint8_t)(member->max_hp - member->hp);
            healed = (def->effect_amount < max_heal) ? def->effect_amount : max_heal;
            member->hp = (uint8_t)(member->hp + healed);
            telemetry_emit(EVENT_HEALED, healed, (uint8_t)id, 0, 0);
            ok = true;
            break;
        default:
            telemetry_emit(EVENT_ITEM_USE_FAILED, (uint8_t)id, 3, 0, 0);
            return false;
    }

    inventory_remove(&state->inventory, id, 1);
    telemetry_emit(EVENT_ITEM_USED, (uint8_t)id, (uint8_t)target, 0, 0);
    return ok;
}

ItemPurchaseResult item_purchase(GameState *state, ItemId id)
{
    const ItemDefinition *def;
    int16_t price;

    if (!state) return ITEM_PURCHASE_NOT_SOLD;
    def = item_get_def(id);
    if (!def || def->price == 0 || def->currency == 0) {
        telemetry_emit(EVENT_ITEM_PURCHASE_FAILED, (uint8_t)id,
                       (uint8_t)ITEM_PURCHASE_NOT_SOLD, 0, 0);
        return ITEM_PURCHASE_NOT_SOLD;
    }
    price = (int16_t)def->price;

    /* A new slot is only needed when the item is not already held; an
     * existing entry stacks (inventory_add never needs a new slot). */
    if (inventory_count(&state->inventory, id) == 0 &&
        state->inventory.count >= MAX_INVENTORY_ITEMS) {
        telemetry_emit(EVENT_ITEM_PURCHASE_FAILED, (uint8_t)id,
                       (uint8_t)ITEM_PURCHASE_NO_CAPACITY, 0, 0);
        return ITEM_PURCHASE_NO_CAPACITY;
    }
    if (currency_get(state, def->currency) < price) {
        telemetry_emit(EVENT_ITEM_PURCHASE_FAILED, (uint8_t)id,
                       (uint8_t)ITEM_PURCHASE_NO_GOLD, 0, 0);
        return ITEM_PURCHASE_NO_GOLD;
    }

    /* Commit (both can no longer fail after the checks above). */
    currency_add(state, def->currency, -price);
    inventory_add(&state->inventory, id, 1);
    telemetry_emit(EVENT_ITEM_PURCHASED, (uint8_t)id, (uint8_t)price, 0, 0);
    return ITEM_PURCHASE_OK;
}

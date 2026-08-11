#include "game.h"
#include "screen.h"
#include "telemetry.h"
#include "rpg/items.h"
#include "rpg/inventory.h"

static void item_draw(Game *g)
{
    uint8_t i;
    char qty[7];
    const InventoryState *inv = &g->state.inventory;

    ui_clear_screen();
    ui_draw_text_line(0, 1, "ITEMS", 20);

    if (inv->count == 0) {
        ui_draw_text_line(0, 3, "(no items)", 20);
    }
    for (i = 0; i < inv->count && i < MAX_INVENTORY_ITEMS; i++) {
        const ItemDefinition *def = item_get_def(inv->entries[i].item_id);
        const char *name = def ? def->name : "???";
        ui_format_int((int16_t)inv->entries[i].quantity, qty);
        ui_draw_text_line(0, (uint8_t)(3 + i), (g->item_menu_index == i) ? ">" : " ", 1);
        ui_draw_text_line(1, (uint8_t)(3 + i), name, 8);
        ui_draw_text_line(10, (uint8_t)(3 + i), "x", 1);
        ui_draw_text_line(11, (uint8_t)(3 + i), qty, 5);
    }
    ui_draw_text_line(0, 16, "[A] Use  [B] Close", 20);
}

void item_screen_update(Game *g)
{
    ItemId id;

    if (!g) return;

    if (g->state.inventory.count > 0) {
        if (input_pressed(INPUT_UP)) {
            if (g->item_menu_index > 0) {
                g->item_menu_index--;
            }
            g->render_cache.valid = false;
        }
        if (input_pressed(INPUT_DOWN)) {
            if ((uint8_t)(g->item_menu_index + 1) < g->state.inventory.count) {
                g->item_menu_index++;
            }
            g->render_cache.valid = false;
        }
        if (input_pressed(INPUT_A)) {
            id = g->state.inventory.entries[g->item_menu_index].item_id;
            if (item_use(&g->state, id)) {
                /* In battle, using an item heals the battle player and
                 * consumes the turn, matching the attack cadence. */
                if (g->prev_screen == SCREEN_BATTLE) {
                    g->battle.player.hp = g->state.party.members[0].hp;
                    g->battle.turn = BATTLE_TURN_ENEMY_DELAY;
                    g->battle.delay_timer = 20;
                }
                if (g->item_menu_index >= g->state.inventory.count) {
                    if (g->state.inventory.count > 0) {
                        g->item_menu_index = (uint8_t)(g->state.inventory.count - 1);
                    } else {
                        g->item_menu_index = 0;
                    }
                }
                g->render_cache.valid = false;
            }
        }
    }

    if (input_pressed(INPUT_B) || input_pressed(INPUT_START)) {
        g->item_menu_index = 0;
        screen_change(g, g->prev_screen);
    }
}

void item_screen_render(Game *g)
{
    RenderCache *rc;

    if (!g) return;
    rc = &g->render_cache;

    if (!rc->valid || rc->prev_screen != SCREEN_ITEM) {
        item_draw(g);
        telemetry_emit(EVENT_RENDER_SCREEN, (uint8_t)SCREEN_ITEM, 0, 0, 0);
        rc->valid = true;
        rc->prev_screen = SCREEN_ITEM;
    }
}

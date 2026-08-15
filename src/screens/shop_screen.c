#include "game.h"
#include "screen.h"
#include "telemetry.h"
#include "rpg/items.h"
#include "rpg/currency.h"
#include "menu.h"
#include "shops.h"
#include "game_ids.h"

#define SHOP_MSG_NONE 0
#define SHOP_MSG_BOUGHT 1
#define SHOP_MSG_NO_GOLD 2

/* The active shop is chosen by the actor that was engaged (g->shop_id).
 * Cursor state lives in g->item_menu_index. */
static const ShopDefinition *shop_active(const Game *g)
{
    return game_shop_for_id(g ? g->shop_id : 1);
}

static void shop_draw(Game *g)
{
    const ShopDefinition *def = shop_active(g);
    MenuFrame frame = { "SHOP", 0, 3, 12 };
    char str[7];
    uint8_t i, y;

    menu_draw_frame(&frame);
    y = menu_row(&frame, 0);
    ui_draw_text_line(0, y, "GOLD:", 5);
    ui_format_int(currency_get(&g->state, CURRENCY_ID_GOLD), str);
    ui_draw_text_line(5, y, str, 14);

    if (!def) {
        menu_draw_content(&frame, 2, "(nothing)");
        ui_draw_text_line(0, menu_row(&frame, 4), "[B] Leave", 9);
        return;
    }

    for (i = 0; i < def->count; i++) {
        const ItemDefinition *item = item_get_def(def->items[i]);
        y = menu_row(&frame, 1 + i);
        ui_draw_text_line(0, y, (g->item_menu_index == i) ? ">" : " ", 1);
        ui_format_int((int16_t)(item ? item->price : 0), str);
        ui_draw_text_line(1, y, item ? item->name : "?", 8);
        ui_draw_text_line(9, y, str, 4);
        ui_draw_text_line(14, y, "G", 1);
    }

    ui_draw_text_line(0, menu_row(&frame, 3 + def->count), "[A] Buy  [B] Leave", 18);
    if (g->shop_message == SHOP_MSG_BOUGHT) {
        menu_draw_content(&frame, 5 + def->count, "Bought!");
    } else if (g->shop_message == SHOP_MSG_NO_GOLD) {
        menu_draw_content(&frame, 5 + def->count, "Not enough gold!");
    }
}

void shop_screen_update(Game *g)
{
    const ShopDefinition *def;
    const ItemDefinition *item;

    if (!g) return;
    def = shop_active(g);

    if (def && g->item_menu_index >= def->count && def->count > 0) {
        g->item_menu_index = (uint8_t)(def->count - 1);
    }

    if (input_pressed(INPUT_UP)) {
        if (g->item_menu_index > 0) g->item_menu_index--;
        g->render_cache.valid = false;
    }
    if (input_pressed(INPUT_DOWN)) {
        if (def && def->count > 0 && (uint8_t)(g->item_menu_index + 1) < def->count) {
            g->item_menu_index++;
        }
        g->render_cache.valid = false;
    }

    if (input_pressed(INPUT_A)) {
        if (def && def->count > 0) {
            item = item_get_def(def->items[g->item_menu_index]);
            if (item) {
                ItemPurchaseResult res = item_purchase(&g->state, item->id);
                switch (res) {
                    case ITEM_PURCHASE_OK:
                        g->shop_message = SHOP_MSG_BOUGHT;
                        break;
                    default:
                        g->shop_message = SHOP_MSG_NO_GOLD;
                        break;
                }
            }
        }
        g->render_cache.valid = false;
    }
    if (input_pressed(INPUT_B) || input_pressed(INPUT_START)) {
        g->shop_message = SHOP_MSG_NONE;
        g->item_menu_index = 0;
        screen_change(g, SCREEN_OVERWORLD);
    }
}

void shop_screen_render(Game *g)
{
    RenderCache *rc;

    if (!g) return;
    rc = &g->render_cache;

    if (!rc->valid || rc->prev_screen != SCREEN_SHOP) {
        shop_draw(g);
        telemetry_emit(EVENT_RENDER_SCREEN, (uint8_t)SCREEN_SHOP, 0, 0, 0);
        rc->valid = true;
        rc->prev_screen = SCREEN_SHOP;
    }
}

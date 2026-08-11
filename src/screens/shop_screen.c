#include "game.h"
#include "screen.h"
#include "telemetry.h"
#include "rpg/items.h"
#include "rpg/currency.h"

#define SHOP_MSG_NONE 0
#define SHOP_MSG_BOUGHT 1
#define SHOP_MSG_NO_GOLD 2

static void shop_draw(Game *g)
{
    const ItemDefinition *potion = item_get_def(ITEM_POTION);
    int16_t gold = currency_get(&g->state, CURRENCY_ID_GOLD);
    char gold_str[7];
    char price_str[7];

    ui_format_int(gold, gold_str);
    ui_format_int((int16_t)potion->price, price_str);

    ui_clear_screen();
    ui_draw_text_line(0, 1, "SHOP", 20);
    ui_draw_text_line(0, 2, "GOLD:", 5);
    ui_draw_text_line(5, 2, gold_str, 14);

    ui_draw_text_line(0, 4, "POTION", 6);
    ui_draw_text_line(7, 4, price_str, 4);
    ui_draw_text_line(12, 4, "G", 1);

    ui_draw_text_line(0, 6, "[A] Buy  [B] Leave", 20);

    switch (g->shop_message) {
        case SHOP_MSG_BOUGHT:
            ui_draw_text_line(0, 8, "Bought a POTION!", 20);
            break;
        case SHOP_MSG_NO_GOLD:
            ui_draw_text_line(0, 8, "Not enough gold!", 20);
            break;
        default:
            ui_draw_text_line(0, 8, "", 20);
            break;
    }
}

void shop_screen_update(Game *g)
{
    if (!g) return;

    if (input_pressed(INPUT_A)) {
        ItemPurchaseResult res = item_purchase(&g->state, ITEM_POTION);
        switch (res) {
            case ITEM_PURCHASE_OK:
                g->shop_message = SHOP_MSG_BOUGHT;
                break;
            default:
                g->shop_message = SHOP_MSG_NO_GOLD;
                break;
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

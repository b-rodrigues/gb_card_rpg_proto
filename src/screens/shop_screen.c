#include "game.h"
#include "screen.h"
#include "telemetry.h"
#include "rpg/items.h"
#include "rpg/currency.h"
#include "menu.h"

#define SHOP_MSG_NONE 0
#define SHOP_MSG_BOUGHT 1
#define SHOP_MSG_NO_GOLD 2

static void shop_draw(Game *g)
{
    const ItemDefinition *potion = item_get_def(ITEM_POTION);
    int16_t gold = currency_get(&g->state, CURRENCY_ID_GOLD);
    MenuFrame frame;
    char gold_str[7];
    char price_str[7];
    uint8_t y;

    ui_format_int(gold, gold_str);
    ui_format_int((int16_t)potion->price, price_str);

    frame.title = "SHOP";
    frame.title_row = 0;
    frame.top_row = 3;
    frame.bottom_row = 12;
    frame.boxed = false;

    menu_draw_frame(&frame);

    y = menu_row(&frame, 0);
    ui_draw_text_line(0, y, "GOLD:", 5);
    ui_draw_text_line(5, y, gold_str, 14);

    y = menu_row(&frame, 1);
    ui_draw_text_line(0, y, "POTION", 6);
    ui_draw_text_line(7, y, price_str, 4);
    ui_draw_text_line(12, y, "G", 1);

    y = menu_row(&frame, 3);
    ui_draw_text_line(0, y, "[A] Buy  [B] Leave", 20);

    switch (g->shop_message) {
        case SHOP_MSG_BOUGHT:
            menu_draw_content(&frame, 5, "Bought a POTION!");
            break;
        case SHOP_MSG_NO_GOLD:
            menu_draw_content(&frame, 5, "Not enough gold!");
            break;
        default:
            menu_draw_content(&frame, 5, "");
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

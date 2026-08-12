#include "game.h"
#include "screen.h"
#include "telemetry.h"
#include "rpg/items.h"
#include "rpg/inventory.h"
#include "rpg/currency.h"
#include "rpg/progression.h"

#define MENU_TAB_ITEM   0
#define MENU_TAB_EQUIP  1
#define MENU_TAB_STATUS 2
#define MENU_TAB_COUNT  3

static void menu_draw(Game *g)
{
    const CharacterState *hero = &g->state.party.members[0];
    const InventoryState *inv = &g->state.inventory;
    int16_t gold = currency_get(&g->state, CURRENCY_ID_GOLD);
    char buf[7];
    uint8_t i;

    ui_clear_screen();

    /* Header: always-visible hero state. */
    ui_draw_text_line(0, 1, "HP:", 3);
    ui_format_int((int16_t)hero->hp, buf);
    ui_draw_text_line(4, 1, buf, 4);
    ui_draw_text_line(8, 1, "/", 1);
    ui_format_int((int16_t)hero->max_hp, buf);
    ui_draw_text_line(9, 1, buf, 4);
    ui_draw_text_line(15, 1, "G:", 2);
    ui_format_int(gold, buf);
    ui_draw_text_line(17, 1, buf, 3);

    /* Tab row. */
    ui_draw_text_line(0, 3, (g->item_menu_tab == MENU_TAB_ITEM) ? ">ITEM" : " ITEM", 6);
    ui_draw_text_line(7, 3, (g->item_menu_tab == MENU_TAB_EQUIP) ? ">EQUIP" : " EQUIP", 7);
    ui_draw_text_line(16, 3, (g->item_menu_tab == MENU_TAB_STATUS) ? ">STATUS" : " STATUS", 8);
    ui_draw_text_line(0, 4, "--------------------", 20);

    if (g->item_menu_tab == MENU_TAB_STATUS) {
        ProgressionTarget t;
        ProgressionState *ps;
        t.type = PROG_TYPE_HERO;
        t.id = 1;
        ps = progression_get(&g->state, t);
        ui_draw_text_line(0, 6, "HERO", 20);
        ui_draw_text_line(0, 8, "LEVEL:", 6);
        ui_format_int((int16_t)(ps ? ps->level : 1), buf);
        ui_draw_text_line(6, 8, buf, 4);
        ui_draw_text_line(0, 9, "PROGRESS:", 9);
        ui_format_int((int16_t)(ps ? ps->progress : 0), buf);
        ui_draw_text_line(10, 9, buf, 6);
        ui_draw_text_line(0, 10, "WEAPON:", 7);
        if (g->state.equipment.weapon != ITEM_NONE) {
            const ItemDefinition *wd = item_get_def(g->state.equipment.weapon);
            ui_draw_text_line(8, 10, wd ? wd->name : "?", 8);
        } else {
            ui_draw_text_line(8, 10, "NONE", 8);
        }
        ui_draw_text_line(0, 16, "[SELECT] TABS  [B] CLOSE", 20);
        return;
    }

    if (inv->count == 0) {
        ui_draw_text_line(0, 6, "(no items)", 20);
    }
    for (i = 0; i < inv->count && i < MAX_INVENTORY_ITEMS; i++) {
        const ItemDefinition *def = item_get_def(inv->entries[i].item_id);
        const char *name = def ? def->name : "???";
        bool equipped = (g->state.equipment.weapon == inv->entries[i].item_id);
        ui_draw_text_line(0, (uint8_t)(6 + i), (g->item_menu_index == i) ? ">" : " ", 1);
        ui_draw_text_line(1, (uint8_t)(6 + i), name, 8);
        if (def && def->kind == ITEM_KIND_CONSUMABLE) {
            ui_format_int((int16_t)inv->entries[i].quantity, buf);
            ui_draw_text_line(10, (uint8_t)(6 + i), "x", 1);
            ui_draw_text_line(11, (uint8_t)(6 + i), buf, 4);
        } else if (equipped) {
            ui_draw_text_line(10, (uint8_t)(6 + i), "EQUIPPED", 8);
        } else {
            ui_draw_text_line(10, (uint8_t)(6 + i), "EQUIP", 5);
        }
    }

    if (g->item_menu_tab == MENU_TAB_ITEM) {
        ui_draw_text_line(0, 16, "[A] USE  [SELECT] TABS  [B] CLOSE", 20);
    } else {
        ui_draw_text_line(0, 16, "[A] EQUIP  [SELECT] TABS  [B] CLOSE", 20);
    }
}

void item_screen_update(Game *g)
{
    ItemId id;

    if (!g) return;

    if (g->item_menu_tab_focus) {
        /* Tab-focus mode: arrows move the tab, A confirms, B cancels. */
        if (input_pressed(INPUT_LEFT)) {
            if (g->item_menu_tab > MENU_TAB_ITEM) g->item_menu_tab--;
            g->render_cache.valid = false;
        }
        if (input_pressed(INPUT_RIGHT)) {
            if ((uint8_t)(g->item_menu_tab + 1) < MENU_TAB_COUNT) g->item_menu_tab++;
            g->render_cache.valid = false;
        }
        if (input_pressed(INPUT_A) || input_pressed(INPUT_START)) {
            g->item_menu_tab_focus = 0;
            g->item_menu_index = 0;
            g->render_cache.valid = false;
        }
        if (input_pressed(INPUT_B) || input_pressed(INPUT_SELECT)) {
            g->item_menu_tab_focus = 0;
            g->render_cache.valid = false;
        }
        return;
    }

    if (input_pressed(INPUT_SELECT)) {
        g->item_menu_tab_focus = 1;
        g->render_cache.valid = false;
        return;
    }

    if (g->item_menu_tab == MENU_TAB_STATUS) {
        if (input_pressed(INPUT_B) || input_pressed(INPUT_START)) {
            g->item_menu_index = 0;
            g->item_menu_tab = MENU_TAB_ITEM;
            screen_change(g, g->prev_screen);
        }
        return;
    }

    if (g->state.inventory.count > 0) {
        if (input_pressed(INPUT_UP)) {
            if (g->item_menu_index > 0) g->item_menu_index--;
            g->render_cache.valid = false;
        }
        if (input_pressed(INPUT_DOWN)) {
            if ((uint8_t)(g->item_menu_index + 1) < g->state.inventory.count) g->item_menu_index++;
            g->render_cache.valid = false;
        }
        if (input_pressed(INPUT_A)) {
            id = g->state.inventory.entries[g->item_menu_index].item_id;
            if (g->item_menu_tab == MENU_TAB_ITEM) {
                if (item_use(&g->state, id, CHARACTER_HERO)) {
                    if (g->prev_screen == SCREEN_BATTLE) {
                        g->battle.player.hp = g->state.party.members[0].hp;
                        g->battle.turn = BATTLE_TURN_ENEMY_DELAY;
                        g->battle.delay_timer = 20;
                    }
                }
            } else if (item_equip(&g->state, id)) {
                game_on_equip(g, id);
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

    if (input_pressed(INPUT_B) || input_pressed(INPUT_START)) {
        g->item_menu_index = 0;
        g->item_menu_tab = MENU_TAB_ITEM;
        screen_change(g, g->prev_screen);
    }
}

void item_screen_render(Game *g)
{
    RenderCache *rc;

    if (!g) return;
    rc = &g->render_cache;

    if (!rc->valid || rc->prev_screen != SCREEN_ITEM) {
        menu_draw(g);
        telemetry_emit(EVENT_RENDER_SCREEN, (uint8_t)SCREEN_ITEM, 0, 0, 0);
        rc->valid = true;
        rc->prev_screen = SCREEN_ITEM;
    }
}

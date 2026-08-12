#include "game.h"
#include "screen.h"
#include "telemetry.h"
#include "rpg/items.h"
#include "rpg/inventory.h"
#include "rpg/currency.h"
#include "rpg/progression.h"

#define MENU_TAB_ITEM   0
#define MENU_TAB_EQUIP  1
#define MENU_TAB_QUEST  2
#define MENU_TAB_STATUS 3
#define MENU_TAB_COUNT  4

static const char *g_tab_labels[MENU_TAB_COUNT] = {"ITEM", "EQUIP", "QUEST", "STAT"};
static const uint8_t g_tab_x[MENU_TAB_COUNT] = {0, 5, 10, 16};

static void menu_draw_status(Game *g, char *buf)
{
    const CharacterState *hero = &g->state.party.members[0];
    ProgressionTarget t;
    ProgressionState *ps;
    int16_t gold = currency_get(&g->state, CURRENCY_ID_GOLD);

    t.type = PROG_TYPE_HERO;
    t.id = 1;
    ps = progression_get(&g->state, t);

    ui_draw_text_line(0, 4, "HERO", 20);
    ui_draw_text_line(0, 5, "HP:", 3);
    ui_format_int((int16_t)hero->hp, buf);
    ui_draw_text_line(4, 5, buf, 4);
    ui_draw_text_line(8, 5, "/", 1);
    ui_format_int((int16_t)hero->max_hp, buf);
    ui_draw_text_line(9, 5, buf, 4);

    ui_draw_text_line(0, 6, "GOLD:", 6);
    ui_format_int(gold, buf);
    ui_draw_text_line(6, 6, buf, 12);

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
}

static void menu_draw_quest(Game *g, char *buf)
{
    int16_t quest = game_variable_get(&g->state, VARIABLE_ID_QUEST_MONSTER_HUNT);
    int16_t slain = game_variable_get(&g->state, VARIABLE_ID_MONSTERS_DEFEATED);

    ui_draw_text_line(0, 4, "QUESTS", 20);
    ui_draw_text_line(0, 6, "MONSTER HUNT", 20);
    if (quest == 1) {
        ui_draw_text_line(0, 7, "monsters: ", 10);
        ui_format_int(slain, buf);
        ui_draw_text_line(10, 7, buf, 1);
        ui_draw_text_line(11, 7, "/3", 2);
    } else if (quest == 2) {
        ui_draw_text_line(0, 7, "complete - SWORD", 20);
    } else {
        ui_draw_text_line(0, 7, "not started", 20);
    }
}

static void menu_draw(Game *g)
{
    const InventoryState *inv = &g->state.inventory;
    char buf[7];
    uint8_t i;
    uint8_t t;

    ui_clear_screen();

    /* Tab row (top).  HP/gold live in the STATUS tab only. */
    for (t = 0; t < MENU_TAB_COUNT; t++) {
        ui_draw_text_line(g_tab_x[t], 1, (g->item_menu_tab == t) ? ">" : " ", 1);
        ui_draw_text_line((uint8_t)(g_tab_x[t] + 1), 1, g_tab_labels[t], 5);
    }
    ui_draw_text_line(0, 2, "--------------------", 20);

    if (g->item_menu_tab == MENU_TAB_STATUS) {
        menu_draw_status(g, buf);
        ui_draw_text_line(0, 16, "[B] CLOSE", 20);
        return;
    }
    if (g->item_menu_tab == MENU_TAB_QUEST) {
        menu_draw_quest(g, buf);
        ui_draw_text_line(0, 16, "[B] CLOSE", 20);
        return;
    }

    if (inv->count == 0) {
        ui_draw_text_line(0, 4, "(no items)", 20);
    }
    for (i = 0; i < inv->count && i < MAX_INVENTORY_ITEMS; i++) {
        const ItemDefinition *def = item_get_def(inv->entries[i].item_id);
        const char *name = def ? def->name : "???";
        bool equipped = (g->state.equipment.weapon == inv->entries[i].item_id);
        ui_draw_text_line(0, (uint8_t)(4 + i), (g->item_menu_index == i) ? ">" : " ", 1);
        ui_draw_text_line(1, (uint8_t)(4 + i), name, 8);
        if (def && def->kind == ITEM_KIND_CONSUMABLE) {
            ui_format_int((int16_t)inv->entries[i].quantity, buf);
            ui_draw_text_line(10, (uint8_t)(4 + i), "x", 1);
            ui_draw_text_line(11, (uint8_t)(4 + i), buf, 4);
        } else if (equipped) {
            ui_draw_text_line(10, (uint8_t)(4 + i), "EQUIPPED", 8);
        } else {
            ui_draw_text_line(10, (uint8_t)(4 + i), "EQUIP", 5);
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
        if (input_pressed(INPUT_A)) {
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

    if (g->item_menu_tab == MENU_TAB_STATUS || g->item_menu_tab == MENU_TAB_QUEST) {
        if (input_pressed(INPUT_B)) {
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

    if (input_pressed(INPUT_B)) {
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

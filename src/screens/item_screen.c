#include "game.h"
#include "screen.h"
#include "telemetry.h"
#include "rpg/items.h"
#include "rpg/inventory.h"
#include "rpg/currency.h"
#include "rpg/progression.h"
#include "menu.h"
#include "game_ids.h"

#define MENU_TAB_ITEM   0
#define MENU_TAB_EQUIP  1
#define MENU_TAB_QUEST  2
#define MENU_TAB_STATUS 3
#define MENU_TAB_COUNT  4

static const uint8_t g_tab_x[MENU_TAB_COUNT] = {0, 5, 10, 15};

/* Per-tab item visibility: the ITEM tab shows consumables only, the EQUIP
 * tab shows weapons only. */
static bool menu_item_visible(uint8_t tab, const ItemDefinition *def)
{
    if (!def) return false;
    if (tab == MENU_TAB_ITEM) return def->kind == ITEM_KIND_CONSUMABLE;
    return def->kind == ITEM_KIND_WEAPON;   /* MENU_TAB_EQUIP */
}

static uint8_t menu_visible_count(const InventoryState *inv, uint8_t tab)
{
    uint8_t i;
    uint8_t n = 0;
    for (i = 0; i < inv->count && i < MAX_INVENTORY_ITEMS; i++) {
        if (menu_item_visible(tab, item_get_def(inv->entries[i].item_id))) n++;
    }
    return n;
}

static uint8_t menu_visible_entry(const InventoryState *inv, uint8_t tab, uint8_t idx)
{
    uint8_t i;
    uint8_t n = 0;
    for (i = 0; i < inv->count && i < MAX_INVENTORY_ITEMS; i++) {
        if (menu_item_visible(tab, item_get_def(inv->entries[i].item_id))) {
            if (n == idx) return i;
            n++;
        }
    }
    return 0xFF;
}

static void menu_draw_tab_row(Game *g)
{
    /* Full labels (direct literals), active tab marked on the row below. */
    ui_draw_text_line(g_tab_x[0], 2, "ITEM", 5);
    ui_draw_text_line(g_tab_x[1], 2, "EQUIP", 5);
    ui_draw_text_line(g_tab_x[2], 2, "QUEST", 5);
    ui_draw_text_line(g_tab_x[3], 2, "STAT", 5);
    ui_draw_text_line(g_tab_x[g->item_menu_tab], 3, "^", 1);
    ui_draw_text_line(0, 4, "--------------------", 20);
}

static void menu_draw_status(Game *g, const MenuFrame *frame, char *buf)
{
    const CharacterState *hero = &g->state.party.members[0];
    ProgressionTarget t;
    ProgressionState *ps;
    int16_t gold = currency_get(&g->state, CURRENCY_ID_GOLD);
    uint8_t y;

    t.type = PROG_TYPE_HERO;
    t.id = 1;
    ps = progression_get(&g->state, t);

    menu_draw_content(frame, 0, "HERO");
    y = menu_row(frame, 1);
    ui_draw_text_line(0, y, "HP:", 3);
    ui_format_int((int16_t)hero->hp, buf);
    ui_draw_text_line(4, y, buf, 4);
    ui_draw_text_line(8, y, "/", 1);
    ui_format_int((int16_t)hero->max_hp, buf);
    ui_draw_text_line(9, y, buf, 4);

    y = menu_row(frame, 2);
    ui_draw_text_line(0, y, "GOLD:", 6);
    ui_format_int(gold, buf);
    ui_draw_text_line(6, y, buf, 12);

    y = menu_row(frame, 4);
    ui_draw_text_line(0, y, "LEVEL:", 6);
    ui_format_int((int16_t)(ps ? ps->level : 1), buf);
    ui_draw_text_line(6, y, buf, 4);

    y = menu_row(frame, 5);
    ui_draw_text_line(0, y, "PROGRESS:", 9);
    ui_format_int((int16_t)(ps ? ps->progress : 0), buf);
    ui_draw_text_line(10, y, buf, 6);

    y = menu_row(frame, 6);
    ui_draw_text_line(0, y, "WEAPON:", 7);
    if (g->state.equipment.weapon != ITEM_NONE) {
        const ItemDefinition *wd = item_get_def(g->state.equipment.weapon);
        ui_draw_text_line(8, y, wd ? wd->name : "?", 8);
    } else {
        ui_draw_text_line(8, y, "NONE", 8);
    }
}

static void menu_draw_quest(Game *g, const MenuFrame *frame, char *buf)
{
    int16_t quest = game_variable_get(&g->state, VARIABLE_ID_QUEST_MONSTER_HUNT);
    int16_t slain = game_variable_get(&g->state, VARIABLE_ID_MONSTERS_DEFEATED);
    uint8_t y;

    menu_draw_content(frame, 2, "MONSTER HUNT");
    y = menu_row(frame, 3);
    if (quest == 1) {
        ui_draw_text_line(0, y, "monsters: ", 10);
        ui_format_int(slain, buf);
        ui_draw_text_line(10, y, buf, 1);
        ui_draw_text_line(11, y, "/3", 2);
    } else if (quest == 2) {
        ui_draw_text_line(0, y, "complete - SWORD", 20);
    } else {
        ui_draw_text_line(0, y, "not started", 20);
    }
}

static void menu_draw(Game *g)
{
    const InventoryState *inv = &g->state.inventory;
    MenuFrame frame;
    char buf[7];
    uint8_t i;
    uint8_t y;
    uint8_t vis_count;

    frame.title_row = 0;
    frame.top_row = 5;
    frame.bottom_row = 17;
    frame.boxed = false;
    switch (g->item_menu_tab) {
        case MENU_TAB_ITEM:   frame.title = "ITEMS"; break;
        case MENU_TAB_EQUIP:  frame.title = "EQUIP"; break;
        case MENU_TAB_QUEST:  frame.title = "QUESTS"; break;
        default:              frame.title = "STATUS"; break;
    }

    menu_draw_frame(&frame);
    menu_draw_tab_row(g);

    if (g->item_menu_tab == MENU_TAB_STATUS) {
        menu_draw_status(g, &frame, buf);
        menu_draw_content(&frame, 11, "[B] CLOSE");
        return;
    }
    if (g->item_menu_tab == MENU_TAB_QUEST) {
        menu_draw_quest(g, &frame, buf);
        menu_draw_content(&frame, 11, "[B] CLOSE");
        return;
    }

    vis_count = menu_visible_count(inv, g->item_menu_tab);
    if (vis_count == 0) {
        menu_draw_content(&frame, 0, "(no items)");
    }
    for (i = 0; i < vis_count; i++) {
        uint8_t ei = menu_visible_entry(inv, g->item_menu_tab, i);
        const ItemDefinition *def = item_get_def(inv->entries[ei].item_id);
        const char *name = def ? def->name : "???";
        bool equipped = (g->state.equipment.weapon == inv->entries[ei].item_id);
        y = menu_row(&frame, i);
        ui_draw_text_line(0, y, (g->item_menu_index == i) ? ">" : " ", 1);
        ui_draw_text_line(1, y, name, 8);
        if (def && def->kind == ITEM_KIND_CONSUMABLE) {
            ui_format_int((int16_t)inv->entries[ei].quantity, buf);
            ui_draw_text_line(10, y, "x", 1);
            ui_draw_text_line(11, y, buf, 4);
        } else if (equipped) {
            ui_draw_text_line(10, y, "EQUIPPED", 8);
        } else {
            ui_draw_text_line(10, y, "EQUIP", 5);
        }
    }

    if (g->item_menu_tab == MENU_TAB_ITEM) {
        menu_draw_content(&frame, 11, "[A] USE  [B] CLOSE");
    } else {
        menu_draw_content(&frame, 11, "[A] EQUIP [B] CLOSE");
    }
}

void item_screen_update(Game *g)
{
    ItemId id;
    uint8_t tab_changed = 0;

    if (!g) return;

    /* Tab navigation: SELECT (and LEFT/RIGHT) directly cycle the active
     * tab, with the ">" marker giving immediate visible feedback. */
    if (input_pressed(INPUT_SELECT) || input_pressed(INPUT_RIGHT)) {
        if ((uint8_t)(g->item_menu_tab + 1) < MENU_TAB_COUNT) {
            g->item_menu_tab++;
        } else {
            g->item_menu_tab = MENU_TAB_ITEM;
        }
        g->item_menu_index = 0;
        tab_changed = 1;
    }
    if (input_pressed(INPUT_LEFT)) {
        if (g->item_menu_tab > MENU_TAB_ITEM) {
            g->item_menu_tab--;
        } else {
            g->item_menu_tab = MENU_TAB_STATUS;
        }
        g->item_menu_index = 0;
        tab_changed = 1;
    }
    if (tab_changed) {
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

    if (menu_visible_count(&g->state.inventory, g->item_menu_tab) > 0) {
        if (input_pressed(INPUT_UP)) {
            if (g->item_menu_index > 0) g->item_menu_index--;
            g->render_cache.valid = false;
        }
        if (input_pressed(INPUT_DOWN)) {
            if ((uint8_t)(g->item_menu_index + 1) < menu_visible_count(&g->state.inventory, g->item_menu_tab)) {
                g->item_menu_index++;
            }
            g->render_cache.valid = false;
        }
        if (input_pressed(INPUT_A)) {
            uint8_t ei = menu_visible_entry(&g->state.inventory, g->item_menu_tab, g->item_menu_index);
            id = g->state.inventory.entries[ei].item_id;
            if (g->item_menu_tab == MENU_TAB_ITEM) {
                if (item_use(&g->state, id, CHARACTER_HERO)) {
                    if (g->prev_screen == SCREEN_BATTLE) {
                        g->battle.player.hp = g->state.party.members[0].hp;
                        g->battle.turn = BATTLE_TURN_ENEMY_DELAY;
                        g->battle.delay_timer = 20;
                    }
                }
            } else {
                item_equip(&g->state, id);
            }
            if (g->item_menu_index >= menu_visible_count(&g->state.inventory, g->item_menu_tab)) {
                uint8_t vc = menu_visible_count(&g->state.inventory, g->item_menu_tab);
                if (vc > 0) {
                    g->item_menu_index = (uint8_t)(vc - 1);
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

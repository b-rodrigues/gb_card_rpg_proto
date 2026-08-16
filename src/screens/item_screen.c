#include "game.h"
#include "screen.h"
#include "telemetry.h"
#include "rpg/items.h"
#include "rpg/inventory.h"
#include "rpg/currency.h"
#include "rpg/progression.h"
#include "menu.h"
#include "game_ids.h"
#include "quest.h"

#define MENU_TAB_ITEM   0
#define MENU_TAB_EQUIP  1
#define MENU_TAB_QUEST  2
#define MENU_TAB_STATUS 3
#define MENU_TAB_COUNT  4

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
    uint8_t i, n = 0;
    for (i = 0; i < inv->count && i < MAX_INVENTORY_ITEMS; i++) {
        const ItemDefinition *def = item_get_def(inv->entries[i].item_id);
        if (menu_item_visible(tab, def)) n++;
    }
    return n;
}

static uint8_t get_selected_inventory_index(const InventoryState *inv, uint8_t tab, uint8_t sel_idx)
{
    uint8_t i, n = 0;
    for (i = 0; i < inv->count && i < MAX_INVENTORY_ITEMS; i++) {
        const ItemDefinition *def = item_get_def(inv->entries[i].item_id);
        if (menu_item_visible(tab, def)) {
            if (n == sel_idx) return i;
            n++;
        }
    }
    return 0xFF;
}

static void menu_draw_tabs(Game *g)
{
    ui_draw_text_line(0, 2, "ITEM EQUIPQUESTSTAT ", 20);
    ui_draw_text_line((uint8_t)(g->item_menu_tab * 5), 3, "^", 1);
    ui_draw_text_line(0, 4, "--------------------", 20);
}

static void menu_draw_status(Game *g, char *buf)
{
    const CharacterState *hero = &g->state.party.members[0];
    ProgressionTarget t = { PROG_TYPE_HERO, 1 };
    ProgressionState *ps = progression_get(&g->state, t);
    const ItemDefinition *wd;

    ui_draw_text_line(0, 5, "HERO", 4);
    ui_draw_text_line(0, 6, "HP:", 3);
    ui_format_int((int16_t)hero->hp, buf);
    ui_draw_text_line(4, 6, buf, 4);
    ui_draw_text_line(8, 6, "/", 1);
    ui_format_int((int16_t)hero->max_hp, buf);
    ui_draw_text_line(9, 6, buf, 4);

    ui_draw_text_line(0, 7, "GOLD:", 5);
    ui_format_int(currency_get(&g->state, CURRENCY_ID_GOLD), buf);
    ui_draw_text_line(6, 7, buf, 12);

    ui_draw_text_line(0, 9, "LEVEL:", 6);
    ui_format_int((int16_t)(ps ? ps->level : 1), buf);
    ui_draw_text_line(6, 9, buf, 4);

    ui_draw_text_line(0, 10, "PROGRESS:", 9);
    ui_format_int((int16_t)(ps ? ps->progress : 0), buf);
    ui_draw_text_line(10, 10, buf, 6);

    ui_draw_text_line(0, 11, "WEAPON:", 7);
    wd = (g->state.equipment.weapon != ITEM_NONE) ? item_get_def(g->state.equipment.weapon) : NULL;
    ui_draw_text_line(8, 11, wd ? wd->name : "NONE", 8);
}

static void quest_draw_status(Game *g, const QuestDefinition *q, uint8_t y, char *buf)
{
    QuestStatus st;
    if (!q) return;
    st = quest_status(&g->state, q);

    if (st == QUEST_STATUS_NOT_STARTED) {
        ui_draw_text_line(0, y, "not started", 11);
    } else if (st == QUEST_STATUS_ACTIVE) {
        if (q->progress_variable != 0) {
            uint8_t len;
            ui_draw_text_line(0, y, q->progress_label, 8);
            ui_draw_text_line(8, y, ": ", 2);
            ui_format_int(game_variable_get(&g->state, q->progress_variable), buf);
            ui_draw_text_line(10, y, buf, 4);
            len = (uint8_t)((buf[1] == '\0') ? 11 : ((buf[2] == '\0') ? 12 : 13));
            ui_draw_text_line(len++, y, "/", 1);
            ui_format_int(q->progress_target, buf);
            ui_draw_text_line(len, y, buf, 4);
        } else {
            ui_draw_text_line(0, y, "active", 6);
        }
    } else {
        ui_draw_text_line(0, y, "complete - ", 11);
        ui_draw_text_line(11, y, q->complete_note ? q->complete_note : "", 9);
    }
}

static void menu_draw_quest(Game *g, char *buf)
{
    uint8_t i, y = 7;
    const QuestDefinition *q;

    for (i = 0; i < quest_count() && y < 15; i++) {
        q = quest_at(i);
        if (!q) break;
        ui_draw_text_line(0, y++, q->name, 20);
        quest_draw_status(g, q, y++, buf);
    }
}

static const char * const s_tab_titles[4] = { "ITEMS", "EQUIP", "QUESTS", "STATUS" };

static void menu_draw(Game *g)
{
    const InventoryState *inv = &g->state.inventory;
    MenuFrame frame;
    char buf[7];
    uint8_t i, y, vis_count = 0;

    frame.title_row = 0;
    frame.top_row = 5;
    frame.bottom_row = 17;
    frame.title = s_tab_titles[g->item_menu_tab];

    menu_draw_frame(&frame);
    menu_draw_tabs(g);

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

    uint8_t scroll = 0;
    if (g->item_menu_index >= 8) {
        scroll = (uint8_t)(g->item_menu_index - 7);
    }

    for (i = 0; i < inv->count && i < MAX_INVENTORY_ITEMS; i++) {
        const ItemDefinition *def = item_get_def(inv->entries[i].item_id);
        if (menu_item_visible(g->item_menu_tab, def)) {
            if (vis_count >= scroll && (uint8_t)(vis_count - scroll) < 8) {
                y = (uint8_t)(5 + (vis_count - scroll));
                ui_draw_text_line(0, y, (g->item_menu_index == vis_count) ? ">" : " ", 1);
                ui_draw_text_line(1, y, def->name, 8);
                if (def->kind == ITEM_KIND_CONSUMABLE) {
                    ui_format_int((int16_t)inv->entries[i].quantity, buf);
                    ui_draw_text_line(10, y, "x", 1);
                    ui_draw_text_line(11, y, buf, 4);
                } else if (g->state.equipment.weapon == inv->entries[i].item_id) {
                    ui_draw_text_line(10, y, "EQUIPPED", 8);
                } else {
                    ui_draw_text_line(10, y, "EQUIP", 5);
                }
            }
            vis_count++;
        }
    }
    if (vis_count == 0) {
        ui_draw_text_line(0, 5, "(no items)", 20);
    }
    ui_draw_text_line(0, 16, (g->item_menu_tab == MENU_TAB_ITEM) ? "[A] USE  [B] CLOSE" : "[A] EQUIP [B] CLOSE", 20);
}

void item_screen_update(Game *g)
{
    uint8_t vis_count;
    if (!g) return;

    if (input_pressed(INPUT_SELECT) || input_pressed(INPUT_RIGHT)) {
        g->item_menu_tab = (uint8_t)((g->item_menu_tab + 1) & 3);
        g->item_menu_index = 0;
        g->render_cache.valid = false;
        return;
    }
    if (input_pressed(INPUT_LEFT)) {
        g->item_menu_tab = (uint8_t)((g->item_menu_tab - 1) & 3);
        g->item_menu_index = 0;
        g->render_cache.valid = false;
        return;
    }

    if (input_pressed(INPUT_B)) {
        g->item_menu_index = 0;
        g->item_menu_tab = MENU_TAB_ITEM;
        screen_change(g, g->prev_screen);
        return;
    }

    if (g->item_menu_tab >= MENU_TAB_QUEST) return;

    vis_count = menu_visible_count(&g->state.inventory, g->item_menu_tab);
    if (vis_count > 0) {
        if (input_pressed(INPUT_UP) && g->item_menu_index > 0) {
            g->item_menu_index--;
            g->render_cache.valid = false;
        }
        if (input_pressed(INPUT_DOWN) && (uint8_t)(g->item_menu_index + 1) < vis_count) {
            g->item_menu_index++;
            g->render_cache.valid = false;
        }
        if (input_pressed(INPUT_A)) {
            uint8_t ei = get_selected_inventory_index(&g->state.inventory, g->item_menu_tab, g->item_menu_index);
            if (ei != 0xFF) {
                ItemId id = g->state.inventory.entries[ei].item_id;
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
                vis_count = menu_visible_count(&g->state.inventory, g->item_menu_tab);
                if (g->item_menu_index >= vis_count) {
                    g->item_menu_index = (uint8_t)(vis_count > 0 ? vis_count - 1 : 0);
                }
            }
            g->render_cache.valid = false;
        }
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

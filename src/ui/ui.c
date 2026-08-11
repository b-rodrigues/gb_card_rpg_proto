#include "ui.h"
#include "npc.h"
#include <gb/gb.h>
#include <gb/cgb.h>
#include <gbdk/font.h>
#include <gbdk/console.h>
#include <stdio.h>

char g_ui_screen_buf[18][21];

static font_t ibm_font;

static const palette_color_t cgb_palette[4] = {
    RGB8(255, 255, 255),
    RGB8(170, 170, 170),
    RGB8(85, 85, 85),
    RGB8(0, 0, 0)
};

void ui_init(void)
{
    /* Turn the LCD off before loading the font so that display_off() inside
     * set_bkg_data() returns immediately (it waits for VBlank otherwise).
     * The CRT0 boot already leaves the LCD off; the harness (which skips
     * CRT0) needs this too, and mGBA's debugger does not advance VBlank
     * during single-stepping. */
    LCDC_REG &= ~0x80;

    font_init();
    ibm_font = font_load(font_ibm);
    font_set(ibm_font);

    /* Disable GBDK console auto-scroll.  When putchar() advances the cursor
     * past the bottom-right tile, the console normally scrolls the whole
     * screen up one line.  M_NO_SCROLL (bit 2 of the console .mode byte at
     * 0xC0A2) makes it reset the cursor instead. */
    *((volatile uint8_t *)0xC0A2) |= M_NO_SCROLL;

    /* Set DMG palettes: 0xE4 = 11 10 01 00 (Lightest to Darkest) */
    BGP_REG = 0xE4;
    OBP0_REG = 0xE4;
    OBP1_REG = 0xE4;

    /* Set CGB Palette 0 if running on CGB hardware */
    if (_cpu == CGB_TYPE) {
        set_bkg_palette(0, 1, cgb_palette);
    }

    SHOW_BKG;
    DISPLAY_ON;
}

void ui_clear_screen(void)
{
    uint8_t x, y;
    for (y = 0; y < 18; y++) {
        for (x = 0; x < 20; x++) {
            gotoxy(x, y);
            putchar(' ');
            g_ui_screen_buf[y][x] = ' ';
        }
        g_ui_screen_buf[y][20] = '\0';
    }
}

void ui_draw_text_line(uint8_t x, uint8_t y, const char *text, uint8_t max_chars)
{
    uint8_t i = 0;
    char ch;
    gotoxy(x, y);
    if (text) {
        while (text[i] != '\0' && i < max_chars) {
            ch = text[i];
            putchar((int)ch);
            if (y < 18 && (x + i) < 20) {
                g_ui_screen_buf[y][x + i] = ch;
            }
            i++;
        }
    }
    while (i < max_chars) {
        putchar(' ');
        if (y < 18 && (x + i) < 20) {
            g_ui_screen_buf[y][x + i] = ' ';
        }
        i++;
    }
}

void ui_draw_world_map(const World *world)
{
    uint8_t x, y;
    uint8_t t;
    char tile_ch;
    const NpcDef *npc_def;

    if (!world) return;

    for (y = 0; y < WORLD_VIEW_HEIGHT; y++) {
        for (x = 0; x < WORLD_WIDTH; x++) {
            npc_def = npc_find_at(world->map_id, x, y);
            if (world->player.position.x == x && world->player.position.y == y) {
                tile_ch = '@';
            } else if (world->enemy.active && world->enemy.position.x == x && world->enemy.position.y == y) {
                tile_ch = 'E';
            } else if (npc_def) {
                tile_ch = (npc_def->id == ENTITY_ID_GUARD) ? 'G' : 'M';
            } else {
                t = world->map[y][x];
                if (t == TILE_WALL) tile_ch = '#';
                else if (t == TILE_BUILDING) tile_ch = 'B';
                else if (t == TILE_FIELD_EXIT) tile_ch = '>';
                else if (t == TILE_TOWN_EXIT) tile_ch = '<';
                else tile_ch = '.';
            }
            gotoxy(x, y);
            putchar(tile_ch);
            g_ui_screen_buf[y][x] = tile_ch;
        }
    }
}

void ui_draw_overworld_hud(const World *world)
{
    if (!world) return;

    ui_draw_text_line(0, 12, "====================", 20);
    if (world->map_id == MAP_TOWN) {
        ui_draw_text_line(0, 13, " MAP: TOWN | HP:10/10", 20);
    } else {
        ui_draw_text_line(0, 13, " MAP: FIELD| HP:10/10", 20);
    }
    ui_draw_text_line(0, 14, "", 20);
    ui_draw_text_line(0, 15, "", 20);
    ui_draw_text_line(0, 16, "", 20);
    ui_draw_text_line(0, 17, " [D-PAD] MOVE HERO", 20);
}

void ui_draw_world_full(const World *world)
{
    if (!world) return;
    ui_clear_screen();
    ui_draw_world_map(world);
    ui_draw_overworld_hud(world);
}

void ui_update_player_position(const World *world, uint8_t old_x, uint8_t old_y, uint8_t new_x, uint8_t new_y)
{
    char old_ch;
    uint8_t t;
    const NpcDef *npc;

    if (!world || (old_x == new_x && old_y == new_y)) return;

    npc = npc_find_at(world->map_id, old_x, old_y);
    if (world->enemy.active && world->enemy.position.x == old_x && world->enemy.position.y == old_y) {
        old_ch = 'E';
    } else if (npc) {
        old_ch = (npc->id == ENTITY_ID_GUARD) ? 'G' : 'M';
    } else {
        t = world->map[old_y][old_x];
        if (t == TILE_WALL) old_ch = '#';
        else if (t == TILE_BUILDING) old_ch = 'B';
        else if (t == TILE_FIELD_EXIT) old_ch = '>';
        else if (t == TILE_TOWN_EXIT) old_ch = '<';
        else old_ch = '.';
    }

    gotoxy(old_x, old_y);
    putchar(old_ch);
    g_ui_screen_buf[old_y][old_x] = old_ch;

    gotoxy(new_x, new_y);
    putchar('@');
    g_ui_screen_buf[new_y][new_x] = '@';
}

static void ui_put_char(uint8_t x, uint8_t y, char ch)
{
    gotoxy(x, y);
    putchar((int)ch);
    if (y < 18 && x < 20) {
        g_ui_screen_buf[y][x] = ch;
    }
}

/* Draw a value right-aligned in a 2-wide field (space padded) at (x,y). */
static void ui_draw_num2(uint8_t x, uint8_t y, uint8_t val)
{
    char c;
    if (val >= 10) {
        c = '0' + (val / 10);
    } else {
        c = ' ';
    }
    ui_put_char(x, y, c);
    ui_put_char(x + 1, y, '0' + (val % 10));
}

/* Draw a battle HP line: "  HP: <hp>/<max>" at row y. */
static void ui_draw_hp_row(uint8_t y, uint8_t hp, uint8_t max_hp)
{
    ui_draw_text_line(0, y, "  HP: ", 6);
    ui_draw_num2(6, y, hp);
    ui_put_char(8, y, '/');
    ui_draw_num2(9, y, max_hp);
}

void ui_draw_battle_full(const Battle *battle)
{
    if (!battle) return;

    ui_clear_screen();

    ui_draw_text_line(0, 0, "====================", 20);
    ui_draw_text_line(0, 1, "    BATTLE ENCOUNTER", 20);
    ui_draw_text_line(0, 2, "====================", 20);

    ui_draw_text_line(0, 5, "  ENEMY: SLIME [E]", 20);
    ui_draw_hp_row(6, battle->enemy.hp, battle->enemy.max_hp);

    ui_draw_text_line(0, 9, "  HERO:  HERO [@]", 20);
    ui_draw_hp_row(10, battle->player.hp, battle->player.max_hp);

    ui_draw_text_line(0, 13, "--------------------", 20);

    ui_update_battle(battle);
}

void ui_update_battle(const Battle *battle)
{
    if (!battle) return;

    /* Keep HP readouts current as damage is dealt/received */
    ui_draw_hp_row(6, battle->enemy.hp, battle->enemy.max_hp);
    ui_draw_hp_row(10, battle->player.hp, battle->player.max_hp);

    if (battle->result == BATTLE_RESULT_VICTORY) {
        ui_draw_text_line(0, 15, " VICTORY! PRESS [A]", 20);
    } else if (battle->result == BATTLE_RESULT_DEFEAT) {
        ui_draw_text_line(0, 15, " DEFEATED! PRESS [A]", 20);
    } else if (battle->turn == BATTLE_TURN_PLAYER) {
        ui_draw_text_line(0, 15, " [A] ATTACK  [B] RUN", 20);
    } else {
        ui_draw_text_line(0, 15, " ENEMY TURN...", 20);
    }
}

void ui_draw_dialogue(const DialogueState *dialogue)
{
    if (!dialogue || !dialogue->active) return;
    if (dialogue->current_line >= dialogue->line_count) return;

    /* Dialogue box occupies dedicated modal overlay region: rows 12-17 (6 rows, 20 columns) */
    ui_draw_text_line(0, 12, "+------------------+", 20);

    gotoxy(0, 13);
    putchar('|');
    g_ui_screen_buf[13][0] = '|';
    ui_draw_text_line(1, 13, dialogue->speaker ? dialogue->speaker : "", 18);
    gotoxy(19, 13);
    putchar('|');
    g_ui_screen_buf[13][19] = '|';

    gotoxy(0, 14);
    putchar('|');
    g_ui_screen_buf[14][0] = '|';
    ui_draw_text_line(1, 14, dialogue->lines[dialogue->current_line], 18);
    gotoxy(19, 14);
    putchar('|');
    g_ui_screen_buf[14][19] = '|';

    gotoxy(0, 15);
    putchar('|');
    g_ui_screen_buf[15][0] = '|';
    ui_draw_text_line(1, 15, "", 18);
    gotoxy(19, 15);
    putchar('|');
    g_ui_screen_buf[15][19] = '|';

    gotoxy(0, 16);
    putchar('|');
    g_ui_screen_buf[16][0] = '|';
    ui_draw_text_line(1, 16, " [A] CONTINUE", 18);
    gotoxy(19, 16);
    putchar('|');
    g_ui_screen_buf[16][19] = '|';

    ui_draw_text_line(0, 17, "+------------------+", 20);
}

void ui_draw_game_over(uint8_t choice)
{
    ui_clear_screen();
    ui_draw_text_line(0, 3, "    GAME OVER", 20);
    ui_draw_text_line(0, 8, "   CONTINUE?", 20);
    if (choice == 0) {
        ui_draw_text_line(0, 11, "> YES", 20);
        ui_draw_text_line(0, 12, "  NO", 20);
    } else {
        ui_draw_text_line(0, 11, "  YES", 20);
        ui_draw_text_line(0, 12, "> NO", 20);
    }
    ui_draw_text_line(0, 16, " [A] CONFIRM", 20);
}

void ui_draw_thanks(void)
{
    ui_clear_screen();
    ui_draw_text_line(0, 8, " THANKS FOR PLAYING!", 20);
}

void ui_draw_font_test(void)
{
    ui_clear_screen();
    ui_draw_text_line(0, 0, "=== FONT TEST ===", 20);
    ui_draw_text_line(0, 2, "ABCDEFGHIJKLMNOPQRST", 20);
    ui_draw_text_line(0, 4, "UVWXYZ", 20);
    ui_draw_text_line(0, 6, "abcdefghijklmnopqrst", 20);
    ui_draw_text_line(0, 8, "uvwxyz", 20);
    ui_draw_text_line(0, 10, "0123456789", 20);
    ui_draw_text_line(0, 12, "!?.,:-'[]+=", 20);
}

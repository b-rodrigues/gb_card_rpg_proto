#include "ui.h"
#include "npc.h"
#include <gb/gb.h>
#include <gb/cgb.h>
#include <gbdk/font.h>
#include <gbdk/console.h>
#include <stdio.h>

static font_t ibm_font;

static const palette_color_t cgb_palette[4] = {
    RGB8(255, 255, 255),
    RGB8(170, 170, 170),
    RGB8(85, 85, 85),
    RGB8(0, 0, 0)
};

void ui_init(void)
{
    font_init();
    ibm_font = font_load(font_ibm);
    font_set(ibm_font);

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
            setchar(' ');
        }
    }
}

void ui_draw_world_full(const World *world)
{
    uint8_t x, y;
    uint8_t t;
    char tile_ch;
    const NpcDef *npc_def;

    if (!world) return;

    ui_clear_screen();

    for (y = 0; y < WORLD_HEIGHT; y++) {
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
            setchar(tile_ch);
        }
    }

    gotoxy(0, 15);
    if (world->map_id == MAP_TOWN) {
        printf("TOWN | HP: %2d/%2d", world->player.hp, world->player.max_hp);
    } else {
        printf("FIELD| HP: %2d/%2d", world->player.hp, world->player.max_hp);
    }
    gotoxy(0, 17);
    printf("[D-PAD] MOVE HERO  ");
}

void ui_update_player_position(uint8_t old_x, uint8_t old_y, uint8_t new_x, uint8_t new_y)
{
    if (old_x == new_x && old_y == new_y) return;

    gotoxy(old_x, old_y);
    setchar('.');

    gotoxy(new_x, new_y);
    setchar('@');
}

void ui_draw_battle_full(const Battle *battle)
{
    if (!battle) return;

    ui_clear_screen();

    gotoxy(0, 0);
    printf("====================");
    gotoxy(4, 1);
    printf("BATTLE ENCOUNTER");
    gotoxy(0, 2);
    printf("====================");

    gotoxy(2, 5);
    printf("ENEMY: %s [E]", battle->enemy.name);
    gotoxy(2, 6);
    printf("HP: %2d/%2d", battle->enemy.hp, battle->enemy.max_hp);

    gotoxy(2, 9);
    printf("HERO:  %s [@]", battle->player.name);
    gotoxy(2, 10);
    printf("HP: %2d/%2d", battle->player.hp, battle->player.max_hp);

    gotoxy(0, 13);
    printf("--------------------");

    ui_update_battle(battle);
}

void ui_update_battle(const Battle *battle)
{
    if (!battle) return;

    gotoxy(6, 6);
    printf("%2d/%2d", battle->enemy.hp, battle->enemy.max_hp);

    gotoxy(6, 10);
    printf("%2d/%2d", battle->player.hp, battle->player.max_hp);

    gotoxy(1, 15);
    if (battle->result == BATTLE_RESULT_VICTORY) {
        printf("VICTORY! PRESS [A] ");
    } else if (battle->result == BATTLE_RESULT_DEFEAT) {
        printf("DEFEATED! PRESS [A]");
    } else if (battle->turn == BATTLE_TURN_PLAYER) {
        printf("[A] ATTACK  [B] RUN ");
    } else {
        printf("ENEMY TURN...       ");
    }
}

void ui_draw_dialogue(const DialogueState *dialogue)
{
    if (!dialogue || !dialogue->active) return;

    gotoxy(0, 13);
    printf("====================");
    gotoxy(0, 14);
    printf(" %-18s ", dialogue->speaker ? dialogue->speaker : "");
    gotoxy(0, 15);
    printf(" %-18s ", dialogue->lines[dialogue->current_line]);
    gotoxy(0, 16);
    printf("                    ");
    gotoxy(0, 17);
    printf(" [A] CONTINUE       ");
}

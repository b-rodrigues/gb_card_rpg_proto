#include "ui.h"
#include <gb/gb.h>
#include <gb/cgb.h>
#include <gbdk/font.h>
#include <gbdk/console.h>
#include <stdio.h>

static font_t min_font;

static const palette_color_t cgb_palette[4] = {
    RGB8(255, 255, 255),
    RGB8(170, 170, 170),
    RGB8(85, 85, 85),
    RGB8(0, 0, 0)
};

void ui_init(void)
{
    font_init();
    min_font = font_load(font_ibm);
    font_set(min_font);

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

void ui_draw_world(const World *world)
{
    uint8_t x, y;
    char tile_ch;

    if (!world) return;

    for (y = 0; y < WORLD_HEIGHT; y++) {
        for (x = 0; x < WORLD_WIDTH; x++) {
            if (world->player.position.x == x && world->player.position.y == y) {
                tile_ch = '@';
            } else if (world->enemy.active && world->enemy.position.x == x && world->enemy.position.y == y) {
                tile_ch = 'E';
            } else if (world->map[y][x] == TILE_WALL) {
                tile_ch = '#';
            } else {
                tile_ch = '.';
            }
            gotoxy(x, y);
            setchar(tile_ch);
        }
    }

    gotoxy(0, 15);
    printf("HERO HP: %2d/%2d", world->player.hp, world->player.max_hp);
    gotoxy(0, 17);
    printf("[D-PAD] MOVE HERO  ");
}

void ui_draw_battle(const Battle *battle)
{
    if (!battle) return;

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

    if (battle->result == BATTLE_RESULT_VICTORY) {
        gotoxy(1, 15);
        printf("VICTORY! PRESS [A] ");
    } else if (battle->result == BATTLE_RESULT_DEFEAT) {
        gotoxy(1, 15);
        printf("DEFEATED! PRESS [A]");
    } else if (battle->turn == BATTLE_TURN_PLAYER) {
        gotoxy(1, 15);
        printf("[A] ATTACK  [B] RUN ");
    } else {
        gotoxy(1, 15);
        printf("ENEMY TURN...       ");
    }
}

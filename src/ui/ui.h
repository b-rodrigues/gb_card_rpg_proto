#ifndef UI_H
#define UI_H

#include "world.h"
#include "battle.h"
#include "dialogue.h"
#include <stdint.h>

#define WORLD_VIEW_HEIGHT 12

void ui_init(void);
void ui_clear_screen(void);

void ui_draw_world_map(const World *world);
void ui_draw_overworld_hud(const World *world);
void ui_draw_world_full(const World *world);
void ui_update_player_position(const World *world, uint8_t old_x, uint8_t old_y, uint8_t new_x, uint8_t new_y);
void ui_draw_text_line(uint8_t x, uint8_t y, const char *text, uint8_t max_chars);

void ui_draw_battle_full(const Battle *battle);
void ui_update_battle(const Battle *battle);

void ui_draw_dialogue(const DialogueState *dialogue);

#endif /* UI_H */

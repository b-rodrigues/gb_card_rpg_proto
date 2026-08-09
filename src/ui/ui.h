#ifndef UI_H
#define UI_H

#include "world.h"
#include "battle.h"
#include <stdint.h>

void ui_init(void);
void ui_clear_screen(void);

void ui_draw_world_full(const World *world);
void ui_update_player_position(uint8_t old_x, uint8_t old_y, uint8_t new_x, uint8_t new_y);

void ui_draw_battle_full(const Battle *battle);
void ui_update_battle(const Battle *battle);

#endif /* UI_H */

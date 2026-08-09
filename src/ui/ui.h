#ifndef UI_H
#define UI_H

#include "world.h"
#include "battle.h"

void ui_init(void);
void ui_clear_screen(void);
void ui_draw_world(const World *world);
void ui_draw_battle(const Battle *battle);

#endif /* UI_H */

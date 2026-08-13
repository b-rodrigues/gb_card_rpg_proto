#ifndef UI_H
#define UI_H

#include "world.h"
#include "battle.h"
#include "dialogue.h"
#include <stdint.h>

#define WORLD_VIEW_HEIGHT 12

extern char g_ui_screen_buf[18][21];

void ui_init(void);
void ui_clear_screen(void);

/* Player rendered as a real OAM sprite.  Background stays console-font
 * ASCII; only the player is a hardware sprite.  Deliberately scoped to the
 * ui layer: no other module needs to know the player is a sprite instead
 * of a printed '@'. */
void ui_sprite_init(void);
void ui_sprite_move(uint8_t px, uint8_t py);
void ui_sprite_hide(void);
void ui_sprite_begin_transition(void);
void ui_sprite_commit(void);

void ui_draw_world_map(const World *world);
void ui_draw_overworld_hud(const World *world);
void ui_draw_world_full(const World *world);
void ui_update_player_position(const World *world, uint8_t old_px, uint8_t old_py, uint8_t new_px, uint8_t new_py);
void ui_draw_text_line(uint8_t x, uint8_t y, const char *text, uint8_t max_chars);

/* Write value as a decimal string into out (at least 7 bytes).  Avoids the
 * stdio/console chain so _HOME stays under 0x8000. */
void ui_format_int(int16_t value, char *out);

void ui_draw_battle_full(const Battle *battle);
void ui_update_battle(const Battle *battle);

void ui_draw_dialogue(const DialogueState *dialogue);
void ui_draw_game_over(uint8_t choice);
void ui_draw_thanks(void);
void ui_draw_font_test(void);

#endif /* UI_H */

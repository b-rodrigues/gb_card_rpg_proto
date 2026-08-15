#ifndef UI_H
#define UI_H

#include "world.h"
#include "battle.h"
#include "dialogue.h"
#include <stdint.h>

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
void oam_dma_init(void);

/* Toggle LCDC bit 7 directly (harness-safe: no GBDK display_off VBlank
 * wait).  Full-screen redraws span several display sweeps and cannot fit
 * in one VBlank, so every full redraw runs with the LCD off and all VRAM
 * writes land deterministically. */
void ui_lcd_off(void);
void ui_lcd_on(void);

void ui_draw_world_map(const World *world);
void ui_draw_overworld_hud(const World *world);
void ui_draw_world_full(const World *world);
void ui_draw_actors_sprites(const World *world);
void ui_draw_text_line(uint8_t x, uint8_t y, const char *text, uint8_t max_chars);

/* Set SCX/SCY from the overworld camera pixel position.  Called every
 * overworld frame so the background glides smoothly. */
void ui_update_camera(const World *world);

/* HUD window layer (0x9C00): fixed at the bottom of the overworld display,
 * off-screen elsewhere (the SCX/SCY-scrolled BG map must not carry the HUD). */
void ui_hud_show(void);
void ui_hud_hide(void);

/* Write value as a decimal string into out (at least 7 bytes).  Avoids the
 * stdio/console chain so _HOME stays under 0x8000. */
void ui_format_int(int16_t value, char *out);

void ui_draw_battle_full(const Battle *battle);
void ui_update_battle(const Battle *battle);

void ui_draw_dialogue(const DialogueState *dialogue, uint8_t scroll_x, uint8_t scroll_y);
void ui_draw_game_over(uint8_t choice);
void ui_draw_thanks(void);
void ui_draw_font_test(void);

#endif /* UI_H */

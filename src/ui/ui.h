#ifndef UI_H
#define UI_H

#include "world.h"
#include "battle.h"
#include "dialogue.h"
#include <stdint.h>

extern char g_ui_screen_buf[18][21];

/* Real-tileset VRAM allocation.  Tile ids below WORLD_TILE_BASE are the
 * GBDK console font (0..127); the overworld terrain occupies 128..131.
 * These bases are the first id of each banked tileset loaded at boot by
 * ui_gfx_load() (data is game content in bank 2, see src/game/gfx_content.c). */
#define UI_BATTLE_BG_TILE_BASE  132   /* 3 tiles + 4x2 tilemap */
#define UI_FRAME_TILE_BASE      135   /* 9 tiles + 3x3 tilemap */
#define UI_ENEMY_SLIME_TILE     144   /* 1 tile (OAM sprite) */
#define UI_ENEMY_BAT_TILE       145   /* 1 tile (OAM sprite) */
#define UI_ENEMY_BOSS_TILE_BASE 146   /* 4 tiles (2x2 OAM sprite grid) */
#define UI_NPC_TILE_BASE        150   /* 5 letter placeholder tiles (M/G/S/E/?) */

/* First VRAM tile id occupied by an actor on the overworld (enemy slime).
 * Everything >= this id in the overworld tilemap ring is an actor tile
 * (the ui_refill_semantic decode keys off it). */
#define UI_ACTOR_TILE_FIRST     UI_ENEMY_SLIME_TILE

/* Copy the banked tilesets into VRAM.  Called once at boot from game_init
 * after banked_copy_init(); renderers then read only VRAM.
 * ui_gfx_map() returns the WRAM-staged battle-background tilemap (4x2). */
void ui_gfx_load(void);
const uint8_t *ui_gfx_map(void);

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
void ui_enemy_sprite_show(uint8_t gfx, uint8_t px, uint8_t py);
void ui_enemy_sprite_hide(void);

void ui_draw_world_map(const World *world);
void ui_draw_overworld_hud(const World *world);
void ui_draw_world_full(const World *world);
void ui_update_player_position(const World *world, uint8_t old_px, uint8_t old_py, uint8_t new_px, uint8_t new_py);
void ui_draw_text_line(uint8_t x, uint8_t y, const char *text, uint8_t max_chars);

/* Overworld redraw when the camera crosses a tile boundary.  Incremental:
 * only the tilemap-ring column/row that entered the window is drawn (the
 * ring holds world tiles at wrapped (world & 31) addresses, so cells still
 * in view stay correct), then the DEBUG semantic view is refilled from the
 * ring mirror.  prev_sx/prev_sy are the previous scroll tile origin. */
void ui_draw_world_scroll(const World *world, uint8_t prev_sx, uint8_t prev_sy);

/* Set SCX/SCY from the overworld camera pixel position.  Called every
 * overworld frame so the background glides smoothly between tile redraws. */
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

void ui_draw_dialogue(const DialogueState *dialogue);
void ui_draw_game_over(uint8_t choice);
void ui_draw_thanks(void);
void ui_draw_font_test(void);

#endif /* UI_H */

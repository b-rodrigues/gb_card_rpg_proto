#ifndef SCREEN_H
#define SCREEN_H

#include <stdint.h>
#include <stdbool.h>

/* Forward declaration; the full type is defined in game.h. */
typedef struct Game Game;

/*
 * ScreenId identifies WHAT KIND of screen is currently active.
 * It is distinct from SceneId (which specific location is loaded).
 */
typedef enum {
    SCREEN_OVERWORLD = 0,
    SCREEN_DIALOGUE  = 1,
    SCREEN_BATTLE    = 2,
    SCREEN_GAME_OVER = 3,
    SCREEN_THANKS    = 4
} ScreenId;

/*
 * SceneId identifies a specific overworld location.  Many scenes share the
 * same SCREEN_OVERWORLD screen implementation.
 */
typedef enum {
    SCENE_FIELD         = 0,
    SCENE_TOWN          = 1,
    SCENE_FOREST        = 2,
    SCENE_MOUNTAIN_PASS = 3,
    SCENE_CASTLE        = 4
} SceneId;

/* ── Screen manager ─────────────────────────────────────────────── */

/* Centralised screen transition: exits the old screen, sets the new screen,
 * and emits SCREEN_CHANGED telemetry. */
void screen_change(Game *g, ScreenId screen);
void screen_update(Game *g);
void screen_render(Game *g);
void scene_load(Game *g, SceneId scene, uint8_t spawn_x, uint8_t spawn_y);
void scene_update_from_map(Game *g);
void scene_sync_from_world(Game *g);

/* ── Screen module lifecycle entry points ────────────────────────── */

void overworld_screen_update(Game *g);
void overworld_screen_render(Game *g);
void dialogue_screen_update(Game *g);
void dialogue_screen_render(Game *g);
void battle_screen_update(Game *g);
void battle_screen_render(Game *g);
void game_over_screen_update(Game *g);
void game_over_screen_render(Game *g);
void thanks_screen_update(Game *g);
void thanks_screen_render(Game *g);

#endif /* SCREEN_H */

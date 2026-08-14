#include "game.h"
#include "screen.h"
#include "scene.h"
#include "telemetry.h"
#include "world.h"
#include "ui.h"

#include <gb/hardware.h>

/* Sync Game.scene with the world's current map after a runtime map
 * change (walking through a gate).  Emits SCENE_CHANGED when it differs. */
void scene_update_from_map(Game *g)
{
    SceneId new_scene;
    SceneId old_scene;
    if (!g) return;

    new_scene = map_to_scene_id(g->world.map_id);
    old_scene = g->state.scene.scene_id;
    g->state.scene.scene_id = new_scene;
    if (old_scene != new_scene) {
        telemetry_emit(EVENT_SCENE_CHANGED, (uint8_t)old_scene, (uint8_t)new_scene, 0, 0);
    }
}

void scene_load(Game *g, SceneId scene, uint8_t spawn_x, uint8_t spawn_y)
{
    SceneId old_scene;
    if (!g) return;

    old_scene = g->state.scene.scene_id;
    g->state.scene.scene_id = scene;
    g->state.scene.player_x = spawn_x;
    g->state.scene.player_y = spawn_y;
    world_change_map(&g->world, scene_id_to_map(scene), spawn_x, spawn_y, &g->state);
    if (old_scene != scene) {
        telemetry_emit(EVENT_SCENE_CHANGED, (uint8_t)old_scene, (uint8_t)scene, spawn_x, spawn_y);
    }
    game_render_reset(g);
}

/* Sync the canonical GameState.scene from the runtime World copy.  Called
 * once per frame from game_update(); the World stays the engine copy. */
void scene_sync_from_world(Game *g)
{
    if (!g) return;
    g->state.scene.scene_id = map_to_scene_id(g->world.map_id);
    g->state.scene.player_x = g->world.player.position.x;
    g->state.scene.player_y = g->world.player.position.y;
    g->state.scene.player_facing = (uint8_t)g->world.player.facing;
}

void screen_change(Game *g, ScreenId screen)
{
    ScreenId old_screen;
    if (!g) return;
    if (g->screen == screen) return;

    /* The player sprite only belongs on the overworld (and behind the
     * dialogue box, which redraws the world): hide it for every other
     * screen.  The transition wipe hide for world/dialogue targets is
     * handled centrally in game_render() (any full redraw calls
     * ui_sprite_begin_transition() before the redraw runs). */
    if (screen != SCREEN_OVERWORLD && screen != SCREEN_DIALOGUE) {
        ui_sprite_hide();
    }

    old_screen = g->screen;
    g->prev_screen = old_screen;
    g->screen = screen;

    /* The HUD window layer is only visible on the overworld (the SCX/SCY
     * background map must not carry it). */
    if (screen == SCREEN_OVERWORLD) {
        ui_hud_show();
    } else {
        ui_hud_hide();
    }

    /* SCX/SCY are the overworld camera (written every frame by
     * ui_update_camera).  Battle/menu screens draw full-screen tiles at the
     * (0,0) origin and never call ui_update_camera, so the camera offset
     * must be reset here: a stale SCX>0 makes a non-overworld screen show
     * a shifted background (e.g. cols 12-31 of a 32-wide ring, leaving the
     * left ~60% of the display showing whatever the ring held).  The
     * dialogue screen is exempt: it overlays the frozen overworld and must
     * keep the current camera. */
    if (screen != SCREEN_OVERWORLD && screen != SCREEN_DIALOGUE) {
        SCX_REG = 0;
        SCY_REG = 0;
    }

    telemetry_emit(EVENT_SCREEN_CHANGED, (uint8_t)old_screen, (uint8_t)screen, 0, 0);
    game_render_reset(g);
}

void screen_update(Game *g)
{
    if (!g) return;
    switch (g->screen) {
        case SCREEN_OVERWORLD:
            overworld_screen_update(g);
            break;
        case SCREEN_DIALOGUE:
            dialogue_screen_update(g);
            break;
        case SCREEN_BATTLE:
            battle_screen_update(g);
            break;
        case SCREEN_GAME_OVER:
            game_over_screen_update(g);
            break;
        case SCREEN_THANKS:
            thanks_screen_update(g);
            break;
        case SCREEN_SHOP:
            shop_screen_update(g);
            break;
        case SCREEN_ITEM:
            item_screen_update(g);
            break;
        case SCREEN_ENDING:
            ending_screen_update(g);
            break;
    }
}

void screen_render(Game *g)
{
    if (!g) return;
    switch (g->screen) {
        case SCREEN_OVERWORLD:
            overworld_screen_render(g);
            break;
        case SCREEN_DIALOGUE:
            dialogue_screen_render(g);
            break;
        case SCREEN_BATTLE:
            battle_screen_render(g);
            break;
        case SCREEN_GAME_OVER:
            game_over_screen_render(g);
            break;
        case SCREEN_THANKS:
            thanks_screen_render(g);
            break;
        case SCREEN_SHOP:
            shop_screen_render(g);
            break;
        case SCREEN_ITEM:
            item_screen_render(g);
            break;
        case SCREEN_ENDING:
            ending_screen_render(g);
            break;
    }
}

#include "game.h"
#include "screen.h"
#include "telemetry.h"
#include "world.h"

static MapId scene_to_map(SceneId scene)
{
    switch (scene) {
        case SCENE_TOWN:          return MAP_TOWN;
        case SCENE_FOREST:        return MAP_FOREST;
        case SCENE_MOUNTAIN_PASS: return MAP_MOUNTAIN_PASS;
        case SCENE_CASTLE:        return MAP_CASTLE;
        default:                  return MAP_FIELD;
    }
}

static SceneId map_to_scene(MapId map)
{
    switch (map) {
        case MAP_TOWN:          return SCENE_TOWN;
        case MAP_FOREST:        return SCENE_FOREST;
        case MAP_MOUNTAIN_PASS: return SCENE_MOUNTAIN_PASS;
        case MAP_CASTLE:        return SCENE_CASTLE;
        default:                return SCENE_FIELD;
    }
}

/* Sync Game.scene with the world's current map after a runtime map
 * change (walking through a gate).  Emits SCENE_CHANGED when it differs. */
void scene_update_from_map(Game *g)
{
    SceneId new_scene;
    SceneId old_scene;
    if (!g) return;

    new_scene = map_to_scene(g->world.map_id);
    old_scene = g->scene;
    g->scene = new_scene;
    if (old_scene != new_scene) {
        telemetry_emit(EVENT_SCENE_CHANGED, (uint8_t)old_scene, (uint8_t)new_scene, 0, 0);
    }
}

void scene_load(Game *g, SceneId scene, uint8_t spawn_x, uint8_t spawn_y)
{
    SceneId old_scene;
    if (!g) return;

    old_scene = g->scene;
    g->scene = scene;
    world_change_map(&g->world, scene_to_map(scene), spawn_x, spawn_y);
    if (old_scene != scene) {
        telemetry_emit(EVENT_SCENE_CHANGED, (uint8_t)old_scene, (uint8_t)scene, spawn_x, spawn_y);
    }
    game_render_reset(g);
}

void screen_change(Game *g, ScreenId screen)
{
    ScreenId old_screen;
    if (!g) return;
    if (g->screen == screen) return;

    old_screen = g->screen;
    g->prev_screen = old_screen;
    g->screen = screen;
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
    }
}

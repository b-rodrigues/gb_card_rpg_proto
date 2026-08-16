#include "scene.h"
#include "actor.h"

/* ── Scene data ──────────────────────────────────────────────────── */

static const SceneExit g_all_exits[] = {
    { 31, 7, 2, 7,  SCENE_TOWN,          '>' },
    { 12, 0, 12, 10, SCENE_FOREST,        '>' },
    { 1, 7, 17, 7, SCENE_FIELD, '<' },
    { 12, 11, 12, 1,  SCENE_FIELD,          '<' },
    { 12, 0,  12, 10, SCENE_MOUNTAIN_PASS, '>' },
    { 12, 11, 12, 1,  SCENE_FOREST, '<' },
    { 12, 0,  10, 10, SCENE_CASTLE, '>' },
    { 12, 11, 12, 1, SCENE_MOUNTAIN_PASS, '<' }
};

static const SceneDefinition g_scenes[] = {
    { MAP_FIELD,         MUSIC_OVERWORLD, 32, 18, &g_all_exits[0], 2 },
    { MAP_TOWN,          MUSIC_OVERWORLD, 20, 18, &g_all_exits[2], 1 },
    { MAP_FOREST,        MUSIC_OVERWORLD, 20, 18, &g_all_exits[3], 2 },
    { MAP_MOUNTAIN_PASS, MUSIC_OVERWORLD, 20, 18, &g_all_exits[5], 2 },
    { MAP_CASTLE,        MUSIC_OVERWORLD, 20, 18, &g_all_exits[7], 1 }
};

const SceneDefinition *scene_definition_for_map(MapId map_id)
{
    return (map_id <= MAP_CASTLE) ? &g_scenes[map_id] : NULL;
}

const SceneExit *scene_exit_at(const SceneDefinition *def, uint8_t x, uint8_t y)
{
    uint8_t i;
    if (!def) return NULL;
    for (i = 0; i < def->exit_count; i++) {
        if (def->exits[i].gate_x == x && def->exits[i].gate_y == y) {
            return &def->exits[i];
        }
    }
    return NULL;
}

/* Fill terrain features for a scene.  A direct switch (not a function
 * pointer) so the code stays in bank 0 and works under the harness. */
static const uint8_t s_forest_trees[] = {4,2, 5,2, 4,3, 10,6, 11,6, 9,7, 14,2, 15,2, 3,8, 8,9, 13,9};

static void scene_fill_terrain(World *w, MapId map_id)
{
    uint8_t x, y;

    if (map_id == MAP_TOWN || map_id == MAP_CASTLE) {
        uint8_t y_max = (map_id == MAP_TOWN) ? 6 : 8;
        uint8_t l_max = (map_id == MAP_TOWN) ? 8 : 6;
        uint8_t r_min = (map_id == MAP_TOWN) ? 12 : 13;
        for (y = 3; y <= y_max; y++) {
            for (x = 3; x <= 16; x++) {
                if (x <= l_max || x >= r_min) w->map[y][x] = TILE_BUILDING;
            }
        }
    } else if (map_id == MAP_FOREST) {
        for (y = 0; y < sizeof(s_forest_trees); y += 2) {
            w->map[s_forest_trees[y+1]][s_forest_trees[y]] = TILE_WALL;
        }
    } else if (map_id == MAP_MOUNTAIN_PASS) {
        for (y = 0; y < 18; y++) {
            for (x = 0; x < 20; x++) {
                if (x <= 3 || x >= 16 || ((y & 3) == 2 && (x == 9 || x == 10))) {
                    w->map[y][x] = TILE_WALL;
                }
            }
        }
    }
}

void scene_load_tiles(World *w, MapId map_id)
{
    const SceneDefinition *def;
    uint8_t x, y;

    if (!w) return;
    def = scene_definition_for_map(map_id);
    if (!def) return;

    for (y = 0; y < w->height; y++) {
        for (x = 0; x < w->width; x++) {
            w->map[y][x] = (y == 0 || y == w->height - 1 || x == 0 || x == w->width - 1) ? TILE_WALL : TILE_FLOOR;
        }
    }

    scene_fill_terrain(w, map_id);

    for (x = 0; x < def->exit_count; x++) {
        w->map[def->exits[x].gate_y][def->exits[x].gate_x] = TILE_EXIT;
    }
}

/* MapId and SceneId 1:1 identity mappings (MAP_FIELD==SCENE_FIELD .. MAP_CASTLE==SCENE_CASTLE).
 * Range checked to guarantee valid values within [0..MAP_CASTLE]. */
MapId scene_id_to_map(SceneId scene)
{
    return (scene <= SCENE_CASTLE) ? (MapId)scene : MAP_FIELD;
}

SceneId map_to_scene_id(MapId map)
{
    return (map <= MAP_CASTLE) ? (SceneId)map : SCENE_FIELD;
}

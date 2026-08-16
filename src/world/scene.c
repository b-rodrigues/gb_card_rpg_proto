#include "scene.h"
#include "actor.h"
#include "banked.h"

/* ── Scene data (resident in ROM Bank 2) ─────────────────────────── */

extern const SceneExit g_all_exits[];
extern const SceneDefinition g_scenes[];

static SceneDefinition s_scene_scratch;
static SceneExit s_exit_scratch;

const SceneDefinition *scene_definition_for_map(MapId map_id)
{
    if (map_id > MAP_CASTLE) return NULL;
    banked_copy(2, &s_scene_scratch, &g_scenes[map_id], sizeof(SceneDefinition));
    return &s_scene_scratch;
}

WorldTilesetKind scene_get_tileset(MapId map_id)
{
    const SceneDefinition *def = scene_definition_for_map(map_id);
    return def ? def->tileset : WORLD_TILESET_EXTERIOR;
}

const SceneExit *scene_exit_at(const SceneDefinition *def, uint8_t x, uint8_t y)
{
    uint8_t i;
    if (!def) return NULL;
    for (i = 0; i < def->exit_count; i++) {
        banked_copy(2, &s_exit_scratch, &def->exits[i], sizeof(SceneExit));
        if (s_exit_scratch.gate_x == x && s_exit_scratch.gate_y == y) {
            return &s_exit_scratch;
        }
    }
    return NULL;
}

/* Fill terrain features for a scene. */
extern const uint8_t s_forest_trees[];

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
        uint8_t trees[22];
        banked_copy(2, trees, s_forest_trees, 22);
        for (y = 0; y < 22; y += 2) {
            w->map[trees[y + 1]][trees[y]] = TILE_WALL;
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
        banked_copy(2, &s_exit_scratch, &def->exits[x], sizeof(SceneExit));
        w->map[s_exit_scratch.gate_y][s_exit_scratch.gate_x] = TILE_EXIT;
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

#include "scene.h"
#include "actor.h"
#include "banked.h"

/* ── Scene data (resident in ROM Bank 2) ─────────────────────────── */

extern const SceneExit g_all_exits[];
extern const SceneDefinition g_scenes[];

static SceneDefinition s_scene_scratch;
static SceneExit s_exit_scratch;
static SceneTerrainBlock s_block_scratch;

const SceneDefinition *scene_definition_for_map(MapId map_id)
{
    if (map_id > MAP_CASTLE) return NULL;
    banked_copy(2, &s_scene_scratch, &g_scenes[map_id], sizeof(SceneDefinition));
    return &s_scene_scratch;
}

WorldTilesetKind scene_get_tileset(MapId map_id)
{
    if (map_id == MAP_CASTLE) return WORLD_TILESET_INTERIOR;
    if (map_id == MAP_FOREST) return WORLD_TILESET_FOREST;
    return WORLD_TILESET_EXTERIOR;
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

void scene_load_tiles(World *w, MapId map_id)
{
    const SceneDefinition *def;
    uint8_t i, x, y;

    if (!w) return;
    def = scene_definition_for_map(map_id);
    if (!def) return;

    for (y = 0; y < w->height; y++) {
        for (x = 0; x < w->width; x++) {
            w->map[y][x] = (y == 0 || y == w->height - 1 || x == 0 || x == w->width - 1) ? TILE_WALL : TILE_FLOOR;
        }
    }

    if (def->terrain_blocks) {
        for (i = 0; ; i++) {
            banked_copy(2, &s_block_scratch, &def->terrain_blocks[i], sizeof(SceneTerrainBlock));
            if (s_block_scratch.w == 0) break;
            for (y = s_block_scratch.y; y < s_block_scratch.y + s_block_scratch.h; y++) {
                for (x = s_block_scratch.x; x < s_block_scratch.x + s_block_scratch.w; x++) {
                    if (x < w->width && y < w->height) {
                        w->map[y][x] = s_block_scratch.tile;
                    }
                }
            }
        }
    }

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

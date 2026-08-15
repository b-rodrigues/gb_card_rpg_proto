#include "scene.h"
#include "actor.h"

/* ── Scene data ──────────────────────────────────────────────────── */

static const SceneExit g_field_exits[] = {
    { 31, 7, 2, 7,  SCENE_TOWN,          '>' },   /* east wall -> Town */
    { 12, 0, 12, 10, SCENE_FOREST,        '>' }    /* north wall -> Forest */
};

static const SceneExit g_town_exits[] = {
    { 1, 7, 17, 7, SCENE_FIELD, '<' }              /* west wall -> Field */
};

static const SceneExit g_forest_exits[] = {
    { 12, 11, 12, 1,  SCENE_FIELD,          '<' },  /* south wall -> Field */
    { 12, 0,  12, 10, SCENE_MOUNTAIN_PASS, '>' }    /* north wall -> Mountain Pass */
};

static const SceneExit g_mountain_pass_exits[] = {
    { 12, 11, 12, 1,  SCENE_FOREST, '<' },          /* south wall -> Forest */
    { 12, 0,  10, 10, SCENE_CASTLE, '>' }           /* north wall -> Castle */
};

static const SceneExit g_castle_exits[] = {
    { 12, 11, 12, 1, SCENE_MOUNTAIN_PASS, '<' }     /* south wall -> Mountain Pass */
};

/* Scenes may be larger than the WORLD_VIEW_W x WORLD_VIEW_H overworld
 * camera window: FIELD is 32 wide so the camera scrolls horizontally, the
 * rest are 20 wide (the legacy screen width).  All are 18 rows tall (the
 * camera scrolls vertically below the 12-row window). */
static const SceneDefinition g_scenes[] = {
    { MAP_FIELD,         MUSIC_OVERWORLD, 32, 18,
        g_field_exits, (uint8_t)(sizeof(g_field_exits)/sizeof(g_field_exits[0])) },
    { MAP_TOWN,          MUSIC_OVERWORLD, 20, 18,
        g_town_exits, (uint8_t)(sizeof(g_town_exits)/sizeof(g_town_exits[0])) },
    { MAP_FOREST,        MUSIC_OVERWORLD, 20, 18,
        g_forest_exits, (uint8_t)(sizeof(g_forest_exits)/sizeof(g_forest_exits[0])) },
    { MAP_MOUNTAIN_PASS, MUSIC_OVERWORLD, 20, 18,
        g_mountain_pass_exits, (uint8_t)(sizeof(g_mountain_pass_exits)/sizeof(g_mountain_pass_exits[0])) },
    { MAP_CASTLE,        MUSIC_OVERWORLD, 20, 18,
        g_castle_exits, (uint8_t)(sizeof(g_castle_exits)/sizeof(g_castle_exits[0])) }
};

#define NUM_SCENES (sizeof(g_scenes) / sizeof(g_scenes[0]))

const SceneDefinition *scene_definition_for_map(MapId map_id)
{
    uint8_t i;
    for (i = 0; i < NUM_SCENES; i++) {
        if (g_scenes[i].map_id == map_id) {
            return &g_scenes[i];
        }
    }
    return NULL;
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
static void scene_fill_terrain(World *w, MapId map_id)
{
    uint8_t x, y, i;

    switch (map_id) {
        case MAP_FIELD:
            break;   /* open field */

        case MAP_TOWN:
            for (y = 3; y <= 6; y++) {
                for (x = 4; x <= 16; x++) {
                    if (x <= 8 || x >= 12) w->map[y][x] = TILE_BUILDING;
                }
            }
            break;

        case MAP_FOREST: {
            static const uint8_t trees[][2] = {{4,2},{5,2},{4,3},{10,6},{11,6},
                                               {9,7},{14,2},{15,2},{3,8},{8,9},{13,9}};
            for (i = 0; i < sizeof(trees)/sizeof(trees[0]); i++) {
                w->map[trees[i][1]][trees[i][0]] = TILE_WALL;
            }
            break;
        }

        case MAP_MOUNTAIN_PASS:
            for (y = 0; y < w->height; y++) {
                for (x = 0; x < w->width; x++) {
                    if (x <= 3 || x >= 16 || (y % 4 == 2 && (x == 9 || x == 10))) {
                        w->map[y][x] = TILE_WALL;
                    }
                }
            }
            break;

        case MAP_CASTLE:
            for (y = 3; y <= 8; y++) {
                for (x = 3; x <= 16; x++) {
                    if (x <= 6 || x >= 13) w->map[y][x] = TILE_BUILDING;
                }
            }
            break;

        default:
            break;
    }
}

void scene_load_tiles(World *w, MapId map_id)
{
    const SceneDefinition *def;
    uint8_t x, y, i;

    if (!w) return;
    def = scene_definition_for_map(map_id);
    if (!def) return;

    for (y = 0; y < w->height; y++) {
        for (x = 0; x < w->width; x++) {
            if (y == 0 || y == w->height - 1 || x == 0 || x == w->width - 1) {
                w->map[y][x] = TILE_WALL;
            } else {
                w->map[y][x] = TILE_FLOOR;
            }
        }
    }

    scene_fill_terrain(w, map_id);

    for (i = 0; i < def->exit_count; i++) {
        w->map[def->exits[i].gate_y][def->exits[i].gate_x] = TILE_EXIT;
    }
}

MapId scene_id_to_map(SceneId scene)
{
    return (scene <= SCENE_CASTLE) ? (MapId)scene : MAP_FIELD;
}

SceneId map_to_scene_id(MapId map)
{
    return (map <= MAP_CASTLE) ? (SceneId)map : SCENE_FIELD;
}

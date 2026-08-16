#pragma bank 2

#include "scene.h"

const SceneExit g_all_exits[] = {
    { 31, 7, 2, 7,   SCENE_TOWN,          '>' },
    { 12, 0, 12, 10, SCENE_FOREST,        '>' },
    { 1,  7, 17, 7,  SCENE_FIELD,         '<' },
    { 12, 11, 12, 1, SCENE_FIELD,         '<' },
    { 12, 0, 12, 10, SCENE_MOUNTAIN_PASS, '>' },
    { 12, 11, 12, 1, SCENE_FOREST,        '<' },
    { 12, 0, 10, 10, SCENE_CASTLE,        '>' },
    { 12, 11, 12, 1, SCENE_MOUNTAIN_PASS, '<' }
};

static const SceneTerrainBlock s_town_terrain[] = {
    { 3, 3, 6, 4, TILE_BUILDING },
    { 12, 3, 5, 4, TILE_BUILDING },
    { 0, 0, 0, 0, 0 }
};

static const SceneTerrainBlock s_forest_terrain[] = {
    /* Tree clusters */
    { 4, 2, 2, 1, TILE_WALL },
    { 4, 3, 1, 1, TILE_WALL },
    { 14, 2, 2, 1, TILE_WALL },
    { 10, 6, 2, 1, TILE_WALL },
    { 9, 7, 1, 1, TILE_WALL },
    { 3, 8, 1, 1, TILE_WALL },
    { 8, 9, 1, 1, TILE_WALL },
    { 13, 9, 1, 1, TILE_WALL },
    /* 2x2 Stump A */
    { 5, 5, 1, 1, TILE_STUMP_TL },
    { 6, 5, 1, 1, TILE_STUMP_TR },
    { 5, 6, 1, 1, TILE_STUMP_BL },
    { 6, 6, 1, 1, TILE_STUMP_BR },
    /* 2x2 Stump B */
    { 15, 4, 1, 1, TILE_STUMP_TL },
    { 16, 4, 1, 1, TILE_STUMP_TR },
    { 15, 5, 1, 1, TILE_STUMP_BL },
    { 16, 5, 1, 1, TILE_STUMP_BR },
    { 0, 0, 0, 0, 0 }
};

static const SceneTerrainBlock s_mountain_terrain[] = {
    { 0, 0, 4, 18, TILE_WALL },
    { 16, 0, 4, 18, TILE_WALL },
    { 9, 2, 2, 1, TILE_WALL },
    { 9, 6, 2, 1, TILE_WALL },
    { 9, 10, 2, 1, TILE_WALL },
    { 9, 14, 2, 1, TILE_WALL },
    { 0, 0, 0, 0, 0 }
};

static const SceneTerrainBlock s_castle_terrain[] = {
    { 3, 3, 4, 6, TILE_BUILDING },
    { 13, 3, 4, 6, TILE_BUILDING },
    { 0, 0, 0, 0, 0 }
};

const SceneDefinition g_scenes[] = {
    { MAP_FIELD,         MUSIC_OVERWORLD, 32, 18, &g_all_exits[0], 2, WORLD_TILESET_EXTERIOR, 0 },
    { MAP_TOWN,          MUSIC_OVERWORLD, 20, 18, &g_all_exits[2], 1, WORLD_TILESET_EXTERIOR, s_town_terrain },
    { MAP_FOREST,        MUSIC_OVERWORLD, 20, 18, &g_all_exits[3], 2, WORLD_TILESET_FOREST,   s_forest_terrain },
    { MAP_MOUNTAIN_PASS, MUSIC_OVERWORLD, 20, 18, &g_all_exits[5], 2, WORLD_TILESET_EXTERIOR, s_mountain_terrain },
    { MAP_CASTLE,        MUSIC_OVERWORLD, 20, 18, &g_all_exits[7], 1, WORLD_TILESET_INTERIOR, s_castle_terrain }
};

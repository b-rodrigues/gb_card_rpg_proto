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

const SceneDefinition g_scenes[] = {
    { MAP_FIELD,         MUSIC_OVERWORLD, 32, 18, &g_all_exits[0], 2, WORLD_TILESET_EXTERIOR },
    { MAP_TOWN,          MUSIC_OVERWORLD, 20, 18, &g_all_exits[2], 1, WORLD_TILESET_EXTERIOR },
    { MAP_FOREST,        MUSIC_OVERWORLD, 20, 18, &g_all_exits[3], 2, WORLD_TILESET_FOREST },
    { MAP_MOUNTAIN_PASS, MUSIC_OVERWORLD, 20, 18, &g_all_exits[5], 2, WORLD_TILESET_EXTERIOR },
    { MAP_CASTLE,        MUSIC_OVERWORLD, 20, 18, &g_all_exits[7], 1, WORLD_TILESET_INTERIOR }
};

const uint8_t s_forest_trees[22] = {
    4, 2, 5, 2, 4, 3, 10, 6, 11, 6, 9, 7, 14, 2, 15, 2, 3, 8, 8, 9, 13, 9
};

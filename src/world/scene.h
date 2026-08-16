#ifndef SCENE_H
#define SCENE_H

#include <stdint.h>
#include "screen.h"
#include "world.h"
#include "audio.h"

/* A generic exit from a scene.  The tile at (gate_x, gate_y) is TILE_EXIT;
 * stepping onto it moves the player to target_scene at (spawn_x, spawn_y).
 * tile_char is the rendered glyph ('>' forward, '<' back, etc.). */
typedef struct {
    uint8_t gate_x;
    uint8_t gate_y;
    uint8_t spawn_x;
    uint8_t spawn_y;
    SceneId target_scene;
    char tile_char;
} SceneExit;

typedef enum {
    WORLD_TILESET_EXTERIOR = 0,
    WORLD_TILESET_INTERIOR = 1,
    WORLD_TILESET_FOREST   = 2
} WorldTilesetKind;

/* Data-driven scene definition.  Terrain generation is dispatched inside
 * scene_load_tiles() by map_id (a direct switch, avoiding banked function
 * pointers); exit placement is applied automatically from the exits table.
 * width/height set the scene's tile bounds (<= WORLD_WIDTH/HEIGHT); the
 * overworld camera clamps its view window to them. */
typedef struct {
    MapId map_id;
    MusicTrack music;
    uint8_t width;
    uint8_t height;
    const SceneExit *exits;
    uint8_t exit_count;
    WorldTilesetKind tileset;
} SceneDefinition;

/* Look up a scene definition by its map id. */
const SceneDefinition *scene_definition_for_map(MapId map_id);

/* Get the tileset kind for a given map id. */
WorldTilesetKind scene_get_tileset(MapId map_id);

/* Find the exit whose gate tile sits at (x, y), or NULL. */
const SceneExit *scene_exit_at(const SceneDefinition *def, uint8_t x, uint8_t y);

/* Fill a world's tile map for the given map id (border, terrain, exits). */
void scene_load_tiles(World *w, MapId map_id);

/* SceneId <-> MapId conversion. */
MapId scene_id_to_map(SceneId scene);
SceneId map_to_scene_id(MapId map);

#endif /* SCENE_H */

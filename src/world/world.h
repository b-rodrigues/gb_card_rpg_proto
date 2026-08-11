#ifndef WORLD_H
#define WORLD_H

#include "entity.h"
#include "rpg/state.h"
#include <stdbool.h>

#define WORLD_WIDTH  20
#define WORLD_HEIGHT 12

/* Maximum concurrent hostile actors in a scene (compile-time constant). */
#define MAX_WORLD_ACTORS 4
#define NO_ACTOR_INDEX   0xFF

typedef enum {
    MOVE_RESULT_NONE        = 0,
    MOVE_RESULT_BLOCKED     = 1,
    MOVE_RESULT_MOVED       = 2,
    MOVE_RESULT_MAP_CHANGED = 3,
    MOVE_RESULT_ENCOUNTER   = 4
} WorldMoveResult;

typedef enum {
    MAP_FIELD         = 0,
    MAP_TOWN          = 1,
    MAP_FOREST        = 2,
    MAP_MOUNTAIN_PASS = 3,
    MAP_CASTLE        = 4
} MapId;

/* A single generic exit tile type; the scene definition owns the
 * destination/spawn/visual of each exit. */
typedef enum {
    TILE_FLOOR    = 0,
    TILE_WALL     = 1,
    TILE_EXIT     = 2,
    TILE_BUILDING = 3
} TileType;

/* Mutable runtime state for a spawned World Actor.  Static actor
 * configuration lives in WorldActorDefinition; hostile actors are spawned
 * into World.actors by actor_load_scene().  actor_id is the persistent
 * ActorId copied from the definition (0 for non-persistent). */
typedef struct {
    uint16_t actor_id;
    EntityId id;
    uint8_t active;
    uint8_t x;
    uint8_t y;
    uint8_t facing;
    uint8_t hp;
    uint8_t max_hp;
    uint8_t flags;               /* runtime state flags (future) */
} WorldActorRuntime;

typedef struct {
    uint8_t width;
    uint8_t height;
    MapId map_id;
    uint8_t encounter_actor_index;   /* slot in actors[], or NO_ACTOR_INDEX */
    bool map_changed;
    Entity player;
    WorldActorRuntime actors[MAX_WORLD_ACTORS];
    uint8_t map[WORLD_HEIGHT][WORLD_WIDTH];
} World;

void world_init(World *w, const GameState *state);
void world_load_map(World *w, MapId map_id, const GameState *state);
void world_change_map(World *w, MapId map_id, uint8_t spawn_x, uint8_t spawn_y,
                      const GameState *state);
bool world_is_walkable(const World *w, uint8_t x, uint8_t y);
WorldMoveResult world_move_player(World *w, int8_t dx, int8_t dy,
                                  const GameState *state);
void world_on_battle_end(World *w, GameState *state, bool victory);
void world_set_player_pos(World *w, uint8_t x, uint8_t y);
void world_set_actor_pos(World *w, EntityId id, uint8_t x, uint8_t y);
void world_set_player_facing(World *w, Direction facing);

#endif /* WORLD_H */

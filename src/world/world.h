#ifndef WORLD_H
#define WORLD_H

#include "entity.h"
#include "rpg/state.h"
#include <stdbool.h>

/* Hard caps for the tile buffer: a scene may be any size up to these.
 * The overworld camera windows a WORLD_VIEW_W x WORLD_VIEW_H view out of
 * the scene and scrolls it with the scroll offset; the 40-col cap
 * exercises the tilemap ring-buffer wrap (the 32x32 BG tilemap is 32
 * wide). */
#define WORLD_WIDTH  40
#define WORLD_HEIGHT 24

/* Overworld camera view window in tiles.  The camera (World.scroll_x/y)
 * keeps the player inside this window, scrolling as the player crosses its
 * edge and clamping at the scene bounds (world width/height). */
#define WORLD_VIEW_W 20
#define WORLD_VIEW_H 12

/* Maximum concurrent hostile actors in a scene (compile-time constant). */
#define MAX_WORLD_ACTORS 4
#define NO_ACTOR_INDEX   0xFF

/* Frames to walk a single tile (1 px/frame of an 8px tile).  Movement is
 * animated: a tile commit happens MOVE_FRAMES frames after the move starts,
 * at which point PLAYER_MOVED / exits / encounters are resolved. */
#define MOVE_FRAMES 8

typedef enum {
    MOVE_STATE_IDLE   = 0,
    MOVE_STATE_MOVING = 1
} MoveState;

/* What the commit of a started move must resolve.  Set by world_try_begin_move
 * from the target tile's contents, consumed by world_update_move. */
typedef enum {
    MOVE_OUTCOME_NONE      = 0,
    MOVE_OUTCOME_NORMAL    = 1,
    MOVE_OUTCOME_EXIT      = 2,
    MOVE_OUTCOME_ENCOUNTER = 3
} MoveOutcome;

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
    uint8_t gold_reward;         /* copied from the definition */
    uint8_t reward_currency;     /* copied from the definition */
    const char *display_name;    /* copied from the definition */
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

    /* Overworld camera in PIXELS (top-left of the view window).  The camera
     * follows the player's pixel position smoothly (world_update_scroll);
     * SCX/SCY are set from these each frame and the tilemap window is drawn
     * around scroll_x/y (= camera_px/8).  Runtime only, never persistent. */
    uint8_t camera_px_x;
    uint8_t camera_px_y;

    /* Overworld camera tile origin (camera_px/8), the top-left tile of the
     * view window into the scene.  Derived by world_update_scroll and
     * exposed to the snapshot as scroll_x/scroll_y. */
    uint8_t scroll_x;
    uint8_t scroll_y;

    /* Movement animation state (runtime only, never persistent).  When
     * move_state == MOVE_STATE_MOVING, the player is animating from
     * player.position toward move_target over MOVE_FRAMES; the tile
     * position only commits at the end of the walk.  The renderer derives
     * the sub-tile pixel position from (position, target, progress). */
    uint8_t move_state;      /* MoveState */
    uint8_t move_target_x;   /* target tile committed at the end of the walk */
    uint8_t move_target_y;
    uint8_t move_progress;   /* 0..MOVE_FRAMES, sub-tile pixel offset */
    uint8_t move_outcome;    /* MoveOutcome resolved at commit */
} World;

void world_init(World *w, const GameState *state);
void world_load_map(World *w, MapId map_id, const GameState *state);
void world_change_map(World *w, MapId map_id, uint8_t spawn_x, uint8_t spawn_y,
                      const GameState *state);
bool world_is_walkable(const World *w, uint8_t x, uint8_t y);

/* Overworld camera: keep the player's sprite inside the
 * WORLD_VIEW_W x WORLD_VIEW_H view window, scrolling smoothly in pixels
 * (SCX/SCY) only when the player approaches the view edge, clamped to the
 * scene bounds (scenes smaller than the view never scroll).  Also derives
 * scroll_x/y (= camera_px/8) for the tilemap window and the snapshot.
 * Called once per overworld frame. */
void world_update_scroll(World *w);

/* Animated movement: world_try_begin_move validates the target and starts
 * the MOVE_FRAMES animation (returns BLOCKED if the tile cannot be walked);
 * world_update_move advances it one frame and resolves the commit (tile
 * move, exit, or encounter).  Returns a WorldMoveResult for the caller.
 * The renderer derives the player's pixel position from the move state via
 * world_player_px/player_py. */
WorldMoveResult world_try_begin_move(World *w, int8_t dx, int8_t dy,
                                     const GameState *state);
WorldMoveResult world_update_move(World *w, const GameState *state);
bool world_is_moving(const World *w);

/* Renderer pixel position (tile*8 plus the sub-tile walk progress) of the
 * player sprite.  Valid whenever the player is not animating a move. */
uint8_t world_player_px(const World *w);
uint8_t world_player_py(const World *w);

void world_on_battle_end(Game *g, bool victory);

/* End a battle by fleeing: the enemy stays on the map at the HP it had when
 * the hero ran (written back into the runtime actor); no reward, defeat or
 * quest progress is applied. */
void world_on_battle_fled(Game *g);
void world_set_player_pos(World *w, uint8_t x, uint8_t y);
void world_set_actor_pos(World *w, EntityId id, uint8_t x, uint8_t y);
void world_set_player_facing(World *w, Direction facing);

#endif /* WORLD_H */

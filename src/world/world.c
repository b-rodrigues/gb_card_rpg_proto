#include "world.h"
#include "game.h"
#include "telemetry.h"
#include "actor.h"
#include "scene.h"
#include "event.h"
#include "rpg/currency.h"

void world_load_map(World *w, MapId map_id, const GameState *state)
{
    const SceneDefinition *def;

    if (!w) return;

    /* Scene size comes from the scene definition; it may be smaller than
     * the WORLD_WIDTH/HEIGHT buffer caps.  The overworld camera clamps its
     * view window to width/height. */
    def = scene_definition_for_map(map_id);
    w->width = def ? def->width : WORLD_WIDTH;
    w->height = def ? def->height : WORLD_HEIGHT;
    w->map_id = map_id;
    w->encounter_actor_index = NO_ACTOR_INDEX;
    w->map_changed = false;
    w->move_state = MOVE_STATE_IDLE;
    w->move_progress = 0;
    w->move_outcome = MOVE_OUTCOME_NONE;
    /* New scene: start the camera at the scene origin; world_update_scroll
     * brings the player into view (and clamps) on the next overworld frame. */
    w->scroll_x = 0;
    w->scroll_y = 0;
    w->camera_px_x = 0;
    w->camera_px_y = 0;

    /* Scene data determines the terrain and the exits. */
    scene_load_tiles(w, map_id);

    /* Scene data determines which hostile actors are spawned.  Actors
     * whose ActorId is DEFEATED in state are not re-spawned. */
    actor_load_scene(w, map_id, state);
}

void world_update_scroll(World *w)
{
    uint8_t player_px;
    uint8_t player_py;
    uint8_t max_x;
    uint8_t max_y;

    if (!w) return;

    player_px = world_player_px(w);
    player_py = world_player_py(w);

    /* Player-centered camera: keep the player at the view centre, clamped
     * at the scene edges (so near an edge the player moves off-centre while
     * the camera stays at the boundary).  The half-view in pixels is
     * VIEW*4; the subtraction is guarded so it never underflows a byte. */
    if (player_px < (uint8_t)(WORLD_VIEW_W * 4)) {
        w->camera_px_x = 0;
    } else {
        w->camera_px_x = (uint8_t)(player_px - WORLD_VIEW_W * 4);
    }
    if (player_py < (uint8_t)(WORLD_VIEW_H * 4)) {
        w->camera_px_y = 0;
    } else {
        w->camera_px_y = (uint8_t)(player_py - WORLD_VIEW_H * 4);
    }

    /* Clamp the view window to the scene bounds (smaller scenes never
     * scroll). */
    max_x = w->width > WORLD_VIEW_W ? (uint8_t)((w->width - WORLD_VIEW_W) * 8) : 0;
    max_y = w->height > WORLD_VIEW_H ? (uint8_t)((w->height - WORLD_VIEW_H) * 8) : 0;
    if (w->camera_px_x > max_x) {
        w->camera_px_x = max_x;
    }
    if (w->camera_px_y > max_y) {
        w->camera_px_y = max_y;
    }

    /* Derived tile camera for the renderer window and the snapshot. */
    w->scroll_x = (uint8_t)(w->camera_px_x >> 3);
    w->scroll_y = (uint8_t)(w->camera_px_y >> 3);
}

void world_init(World *w, const GameState *state)
{
    if (!w) return;
    entity_init(&w->player, ENTITY_ID_PLAYER, 4, 4, 10, 10);
    world_load_map(w, MAP_FIELD, state);
}

void world_change_map(World *w, MapId map_id, uint8_t spawn_x, uint8_t spawn_y,
                      const GameState *state)
{
    MapId old_map;
    if (!w) return;
    old_map = w->map_id;
    world_load_map(w, map_id, state);
    w->player.position.x = spawn_x;
    w->player.position.y = spawn_y;
    w->map_changed = true;
    telemetry_emit(EVENT_MAP_CHANGED, (uint8_t)old_map, (uint8_t)map_id, spawn_x, spawn_y);
}

bool world_is_walkable(const World *w, uint8_t x, uint8_t y)
{
    uint8_t tile;
    if (!w) return false;
    if (x >= w->width || y >= w->height) return false;
    tile = w->map[y][x];
    if (tile == TILE_WALL || tile == TILE_BUILDING) {
        return false;
    }
    return true;
}

WorldMoveResult world_try_begin_move(World *w, int8_t dx, int8_t dy,
                                     const GameState *state)
{
    uint8_t target_x, target_y;
    uint8_t target_tile;
    uint8_t hostile_slot;
    uint8_t outcome;
    const WorldActorDefinition *actor;

    if (!w) return MOVE_RESULT_NONE;
    if (w->move_state == MOVE_STATE_MOVING) return MOVE_RESULT_NONE;
    (void)state;   /* only world_update_move resolves the exit at commit */

    if (dy < 0) w->player.facing = DIRECTION_UP;
    else if (dy > 0) w->player.facing = DIRECTION_DOWN;
    else if (dx < 0) w->player.facing = DIRECTION_LEFT;
    else if (dx > 0) w->player.facing = DIRECTION_RIGHT;

    target_x = (uint8_t)(w->player.position.x + dx);
    target_y = (uint8_t)(w->player.position.y + dy);

    if (!world_is_walkable(w, target_x, target_y)) {
        return MOVE_RESULT_BLOCKED;
    }

    target_tile = w->map[target_y][target_x];

    /* Generic hostile World Actor collision: record which slot was hit so
     * the battle system can read the right HP.  Resolved at move commit. */
    hostile_slot = actor_find_hostile_slot(w, target_x, target_y);
    if (hostile_slot != NO_ACTOR_INDEX) {
        outcome = MOVE_OUTCOME_ENCOUNTER;
        w->encounter_actor_index = hostile_slot;
    } else {
        /* Generic friendly World Actor collision (static definitions). */
        actor = actor_find_at(w, target_x, target_y);
        if (actor) {
            telemetry_emit(EVENT_ACTOR_COLLISION, target_x, target_y,
                           (uint8_t)actor->id, 0);
            return MOVE_RESULT_BLOCKED;
        }
        outcome = (target_tile == TILE_EXIT) ? MOVE_OUTCOME_EXIT
                                             : MOVE_OUTCOME_NORMAL;
    }

    w->move_target_x = target_x;
    w->move_target_y = target_y;
    w->move_progress = 0;
    w->move_outcome = outcome;
    w->move_state = MOVE_STATE_MOVING;
    return MOVE_RESULT_MOVED;
}

WorldMoveResult world_update_move(World *w, const GameState *state)
{
    uint8_t target_x, target_y;

    if (!w) return MOVE_RESULT_NONE;
    if (w->move_state != MOVE_STATE_MOVING) return MOVE_RESULT_NONE;

    w->move_progress++;
    if (w->move_progress >= MOVE_FRAMES) {
        /* Commit: resolve the move's outcome against the target tile. */
        target_x = w->move_target_x;
        target_y = w->move_target_y;
        w->move_progress = 0;
        w->move_state = MOVE_STATE_IDLE;

        if (w->move_outcome == MOVE_OUTCOME_EXIT) {
            /* Generic scene exit: the scene definition owns destination +
             * spawn.  Like the legacy instant move, the player never commits
             * PLAYER_MOVED onto the gate; the map changes instead. */
            const SceneDefinition *def = scene_definition_for_map(w->map_id);
            const SceneExit *ex = scene_exit_at(def, target_x, target_y);
            if (ex) {
                world_change_map(w, scene_id_to_map(ex->target_scene),
                                 ex->spawn_x, ex->spawn_y, state);
                return MOVE_RESULT_MAP_CHANGED;
            }
            return MOVE_RESULT_BLOCKED;
        } else if (w->move_outcome == MOVE_OUTCOME_ENCOUNTER) {
            /* The player does not occupy the enemy tile; battle starts from
             * the pre-move position (matches the legacy instant behavior).
             * The hostile slot was persisted by world_try_begin_move. */
            if (w->encounter_actor_index != NO_ACTOR_INDEX) {
                telemetry_emit(EVENT_ACTOR_COLLISION, target_x, target_y,
                               (uint8_t)w->actors[w->encounter_actor_index].id, 0);
                telemetry_emit(EVENT_ENCOUNTER_STARTED,
                               (uint8_t)w->actors[w->encounter_actor_index].id, 0, 0, 0);
                return MOVE_RESULT_ENCOUNTER;
            }
            return MOVE_RESULT_BLOCKED;
        } else {
            telemetry_emit(EVENT_PLAYER_MOVED, w->player.position.x,
                           w->player.position.y, target_x, target_y);
            w->player.position.x = target_x;
            w->player.position.y = target_y;
            return MOVE_RESULT_MOVED;
        }
    }
    return MOVE_RESULT_MOVED;
}

uint8_t world_player_px(const World *w)
{
    uint8_t px;
    uint8_t base;
    uint8_t progress;
    if (!w) return 0;
    px = (uint8_t)(w->player.position.x * 8);
    if (w->move_state == MOVE_STATE_MOVING) {
        base = w->player.position.x;
        progress = w->move_progress;
        if (w->move_target_x > base) px = (uint8_t)(px + progress);
        else if (w->move_target_x < base) px = (uint8_t)(px - progress);
    }
    return px;
}

uint8_t world_player_py(const World *w)
{
    uint8_t py;
    uint8_t base;
    uint8_t progress;
    if (!w) return 0;
    py = (uint8_t)(w->player.position.y * 8);
    if (w->move_state == MOVE_STATE_MOVING) {
        base = w->player.position.y;
        progress = w->move_progress;
        if (w->move_target_y > base) py = (uint8_t)(py + progress);
        else if (w->move_target_y < base) py = (uint8_t)(py - progress);
    }
    return py;
}

void world_on_battle_end(Game *g, bool victory)
{
    World *w;
    uint8_t idx;
    uint16_t actor_id;
    if (!g) return;
    w = &g->world;

    idx = w->encounter_actor_index;
    w->encounter_actor_index = NO_ACTOR_INDEX;
    if (idx == NO_ACTOR_INDEX) return;

    if (victory) {
        actor_id = w->actors[idx].actor_id;
        w->actors[idx].active = 0;
        w->actors[idx].hp = 0;
        w->actors[idx].flags = ACTOR_STATE_NONE;
        telemetry_emit(EVENT_ENTITY_DEFEATED, (uint8_t)w->actors[idx].id, 0, 0, 0);
        if (w->actors[idx].reward_currency != 0 && w->actors[idx].gold_reward != 0) {
            currency_add(&g->state, (CurrencyId)w->actors[idx].reward_currency,
                         w->actors[idx].gold_reward);
        }
        if (actor_id != 0) {
            game_world_set_actor_state(&g->state, actor_id, ACTOR_STATE_DEFEATED);
        }
        /* Quest progress / final-boss ending is expressed by the event table. */
        event_resolve_actor_defeated(g, actor_id, w->actors[idx].id);
    }
}

void world_on_battle_fled(Game *g)
{
    World *w;
    uint8_t idx;
    if (!g) return;
    w = &g->world;

    idx = w->encounter_actor_index;
    w->encounter_actor_index = NO_ACTOR_INDEX;
    if (idx == NO_ACTOR_INDEX) return;

    /* The enemy stays on the map with the HP it had when the hero fled. */
    if (w->actors[idx].active) {
        w->actors[idx].hp = g->battle.enemy.hp;
    }
}

void world_set_player_pos(World *w, uint8_t x, uint8_t y)
{
    if (!w) return;
    w->player.position.x = x;
    w->player.position.y = y;
    /* Teleports cancel any in-flight move animation (and its pending
     * encounter commit). */
    w->move_state = MOVE_STATE_IDLE;
    w->move_progress = 0;
    w->move_outcome = MOVE_OUTCOME_NONE;
    w->encounter_actor_index = NO_ACTOR_INDEX;
}

void world_set_actor_pos(World *w, EntityId id, uint8_t x, uint8_t y)
{
    uint8_t i;
    if (!w) return;
    for (i = 0; i < MAX_WORLD_ACTORS; i++) {
        if (w->actors[i].active && w->actors[i].id == id) {
            w->actors[i].x = x;
            w->actors[i].y = y;
            return;
        }
    }
}

void world_set_player_facing(World *w, Direction facing)
{
    if (!w) return;
    w->player.facing = facing;
}

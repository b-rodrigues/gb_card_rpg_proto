#include "world.h"
#include "game.h"
#include "telemetry.h"
#include "actor.h"
#include "scene.h"
#include "event.h"
#include "rpg/currency.h"

void world_load_map(World *w, MapId map_id, const GameState *state)
{
    if (!w) return;

    w->width = WORLD_WIDTH;
    w->height = WORLD_HEIGHT;
    w->map_id = map_id;
    w->encounter_actor_index = NO_ACTOR_INDEX;
    w->map_changed = false;

    /* Scene data determines the terrain and the exits. */
    scene_load_tiles(w, map_id);

    /* Scene data determines which hostile actors are spawned.  Actors
     * whose ActorId is DEFEATED in state are not re-spawned. */
    actor_load_scene(w, map_id, state);
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
    if (x >= WORLD_WIDTH || y >= WORLD_HEIGHT) return false;
    tile = w->map[y][x];
    if (tile == TILE_WALL || tile == TILE_BUILDING) {
        return false;
    }
    return true;
}

WorldMoveResult world_move_player(World *w, int8_t dx, int8_t dy,
                                  const GameState *state)
{
    uint8_t old_x, old_y;
    uint8_t target_x, target_y;
    uint8_t target_tile;
    uint8_t hostile_slot;
    const WorldActorDefinition *actor;

    if (!w) return MOVE_RESULT_NONE;

    if (dy < 0) w->player.facing = DIRECTION_UP;
    else if (dy > 0) w->player.facing = DIRECTION_DOWN;
    else if (dx < 0) w->player.facing = DIRECTION_LEFT;
    else if (dx > 0) w->player.facing = DIRECTION_RIGHT;

    target_x = (uint8_t)((int16_t)w->player.position.x + dx);
    target_y = (uint8_t)((int16_t)w->player.position.y + dy);

    if (!world_is_walkable(w, target_x, target_y)) {
        return MOVE_RESULT_BLOCKED;
    }

    target_tile = w->map[target_y][target_x];

    /* Generic scene exit: the scene definition owns destination + spawn. */
    if (target_tile == TILE_EXIT) {
        const SceneDefinition *def = scene_definition_for_map(w->map_id);
        const SceneExit *ex = scene_exit_at(def, target_x, target_y);
        if (ex) {
            world_change_map(w, scene_id_to_map(ex->target_scene),
                             ex->spawn_x, ex->spawn_y, state);
            return MOVE_RESULT_MAP_CHANGED;
        }
    }

    /* Generic hostile World Actor collision: record which slot was hit so
     * the battle system can read the right HP. */
    hostile_slot = actor_find_hostile_slot(w, target_x, target_y);
    if (hostile_slot != NO_ACTOR_INDEX) {
        telemetry_emit(EVENT_ACTOR_COLLISION, target_x, target_y,
                       (uint8_t)w->actors[hostile_slot].id, 0);
        telemetry_emit(EVENT_ENCOUNTER_STARTED,
                       (uint8_t)w->actors[hostile_slot].id, 0, 0, 0);
        w->encounter_actor_index = hostile_slot;
        return MOVE_RESULT_ENCOUNTER;
    }

    /* Generic friendly World Actor collision (static definitions). */
    actor = actor_find_at(w, target_x, target_y);
    if (actor) {
        telemetry_emit(EVENT_ACTOR_COLLISION, target_x, target_y,
                       (uint8_t)actor->id, 0);
        return MOVE_RESULT_BLOCKED;
    }

    old_x = w->player.position.x;
    old_y = w->player.position.y;
    w->player.position.x = target_x;
    w->player.position.y = target_y;
    telemetry_emit(EVENT_PLAYER_MOVED, old_x, old_y, target_x, target_y);
    return MOVE_RESULT_MOVED;
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

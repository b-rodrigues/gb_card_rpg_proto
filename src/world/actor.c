#include "actor.h"
#include <stddef.h>

/* ── Actor engine ──────────────────────────────────────────────────
 * The per-scene actor definitions are game content, registered at boot via
 * actor_register_tables() (see src/game/actors.c).  Friendly actors are
 * pure static definitions; hostile actors are spawned into World.actors
 * runtime slots so a scene can hold several at once.  Each hostile
 * definition carries a stable ActorId (unique across scenes) so its defeat
 * can be recorded persistently in GameState.world. */

static const WorldActorTable *g_actor_tables = NULL;
static uint8_t g_actor_table_count = 0;

void actor_register_tables(const WorldActorTable *tables, uint8_t count)
{
    g_actor_tables = tables;
    g_actor_table_count = count;
}

static const WorldActorDefinition *actor_defs_for_map(MapId map_id, uint8_t *count)
{
    uint8_t i;
    if (!g_actor_tables) {
        if (count) *count = 0;
        return NULL;
    }
    for (i = 0; i < g_actor_table_count; i++) {
        if (g_actor_tables[i].map_id == map_id) {
            if (count) *count = g_actor_tables[i].count;
            return g_actor_tables[i].defs;
        }
    }
    if (count) *count = 0;
    return NULL;
}

static const WorldActorDefinition *actor_find_def_by_actor_id(MapId map_id, ActorId actor_id)
{
    const WorldActorDefinition *defs;
    uint8_t count, i;

    defs = actor_defs_for_map(map_id, &count);
    for (i = 0; i < count; i++) {
        if (defs[i].actor_id == actor_id) {
            return &defs[i];
        }
    }
    return NULL;
}

static void actor_spawn(WorldActorRuntime *r, const WorldActorDefinition *def)
{
    r->actor_id = def->actor_id;
    r->id = def->id;
    r->active = 1;
    r->x = def->x;
    r->y = def->y;
    r->facing = def->facing;
    r->hp = def->hp;
    r->max_hp = def->max_hp;
    r->flags = ACTOR_STATE_NONE;
    r->gold_reward = def->gold_reward;
    r->reward_currency = def->reward_currency;
    r->display_name = def->display_name;
}

uint8_t actor_find_hostile_slot(const World *world, uint8_t x, uint8_t y)
{
    uint8_t i;
    if (!world) return NO_ACTOR_INDEX;
    for (i = 0; i < MAX_WORLD_ACTORS; i++) {
        if (world->actors[i].active &&
            world->actors[i].x == x &&
            world->actors[i].y == y) {
            return i;
        }
    }
    return NO_ACTOR_INDEX;
}

const WorldActorDefinition *actor_find_at(const World *world, uint8_t x, uint8_t y)
{
    const WorldActorDefinition *defs;
    uint8_t count, i, slot;

    if (!world) return NULL;

    slot = actor_find_hostile_slot(world, x, y);
    if (slot != NO_ACTOR_INDEX) {
        /* Hostile defs are keyed by their stable ActorId (copied into the
         * runtime slot), never by EntityId -- a scene may hold several
         * actors of the same EntityId with different defs. */
        return actor_find_def_by_actor_id(world->map_id, world->actors[slot].actor_id);
    }

    defs = actor_defs_for_map(world->map_id, &count);
    for (i = 0; i < count; i++) {
        if (!(defs[i].flags & ACTOR_FLAG_HOSTILE) &&
            defs[i].x == x && defs[i].y == y) {
            return &defs[i];
        }
    }
    return NULL;
}

ActorEngageResult actor_engage(const WorldActorDefinition *actor, DialogueState *dialogue)
{
    if (!actor) return ENGAGE_NONE;
    if (actor->flags & ACTOR_FLAG_HOSTILE) {
        return ENGAGE_BATTLE;
    }

    if (actor->interaction == INTERACTION_DIALOGUE) {
        dialogue_start_def(dialogue, actor->dialogue_id);
        return ENGAGE_DIALOGUE;
    }

    if (actor->interaction == INTERACTION_SHOP) {
        return ENGAGE_SHOP;
    }

    return ENGAGE_NONE;
}

void actor_load_scene(World *world, MapId map_id, const GameState *state)
{
    const WorldActorDefinition *defs;
    uint8_t count, i, slot;

    if (!world) return;
    defs = actor_defs_for_map(map_id, &count);

    for (slot = 0; slot < MAX_WORLD_ACTORS; slot++) {
        world->actors[slot].active = 0;
    }

    slot = 0;
    for (i = 0; i < count && slot < MAX_WORLD_ACTORS; i++) {
        if (defs[i].flags & ACTOR_FLAG_HOSTILE) {
            if (defs[i].actor_id != 0 &&
                game_world_actor_is_defeated(state, defs[i].actor_id)) {
                continue;
            }
            /* Conditional spawn: the definition may require a variable to
             * equal a specific value (e.g. the final boss only after the
             * quest is complete). */
            if (defs[i].spawn_variable != 0 &&
                game_variable_get(state, defs[i].spawn_variable) != defs[i].spawn_value) {
                continue;
            }
            actor_spawn(&world->actors[slot], &defs[i]);
            slot++;
        }
    }
}

uint8_t actor_write_snapshot(const World *world, uint8_t *out, uint8_t max_actors)
{
    const WorldActorDefinition *defs;
    uint8_t count, i, n = 0;
    uint8_t slot;

    if (!world || !out || max_actors == 0) return 0;
    defs = actor_defs_for_map(world->map_id, &count);

    /* hostile runtime actors */
    for (slot = 0; slot < MAX_WORLD_ACTORS && n < max_actors; slot++) {
        if (world->actors[slot].active) {
            out[n * ACTOR_SNAPSHOT_ENTRY_SIZE + 0] = (uint8_t)world->actors[slot].id;
            out[n * ACTOR_SNAPSHOT_ENTRY_SIZE + 1] = world->actors[slot].x;
            out[n * ACTOR_SNAPSHOT_ENTRY_SIZE + 2] = world->actors[slot].y;
            out[n * ACTOR_SNAPSHOT_ENTRY_SIZE + 3] = world->actors[slot].facing;
            n++;
        }
    }

    /* friendly static definitions */
    for (i = 0; i < count && n < max_actors; i++) {
        if (defs[i].flags & ACTOR_FLAG_HOSTILE) continue;
        out[n * ACTOR_SNAPSHOT_ENTRY_SIZE + 0] = (uint8_t)defs[i].id;
        out[n * ACTOR_SNAPSHOT_ENTRY_SIZE + 1] = defs[i].x;
        out[n * ACTOR_SNAPSHOT_ENTRY_SIZE + 2] = defs[i].y;
        out[n * ACTOR_SNAPSHOT_ENTRY_SIZE + 3] = defs[i].facing;
        n++;
    }
    return n;
}

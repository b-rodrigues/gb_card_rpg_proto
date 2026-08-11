#include "actor.h"
#include <stddef.h>

/* ── Scene-owned actor definitions ─────────────────────────────────
 *
 * Friendly actors are pure static definitions.  Hostile actors are
 * spawned into World.actors runtime slots by actor_load_scene(), so a
 * scene can hold several hostile actors at once.  Each hostile definition
 * carries a stable ActorId (unique across scenes) so its defeat can be
 * recorded persistently in GameState.world and survive scene reloads.
 */

static const WorldActorDefinition g_town_actors[] = {
    {
        0, ENTITY_ID_MAYOR, 10, 5, DIRECTION_DOWN,
        ACTOR_FLAG_BLOCKING | ACTOR_FLAG_INTERACTABLE,
        'M', INTERACTION_DIALOGUE, DIALOGUE_ID_MAYOR_GREETING, BATTLE_NONE, 0, 0
    },
    {
        0, ENTITY_ID_GUARD, 10, 8, DIRECTION_DOWN,
        ACTOR_FLAG_BLOCKING | ACTOR_FLAG_INTERACTABLE,
        'G', INTERACTION_DIALOGUE, DIALOGUE_ID_GUARD_GREETING, BATTLE_NONE, 0, 0
    },
    {
        0, ENTITY_ID_SHOPKEEPER, 9, 3, DIRECTION_DOWN,
        ACTOR_FLAG_BLOCKING | ACTOR_FLAG_INTERACTABLE,
        'S', INTERACTION_DIALOGUE, DIALOGUE_ID_SHOPKEEPER_GREETING, BATTLE_NONE, 0, 0
    }
};

static const WorldActorDefinition g_field_actors[] = {
    {
        1, ENTITY_ID_SLIME, 14, 8, DIRECTION_DOWN,
        ACTOR_FLAG_HOSTILE | ACTOR_FLAG_BLOCKING | ACTOR_FLAG_INTERACTABLE,
        'E', INTERACTION_COMBAT, DIALOGUE_ID_NONE, BATTLE_SLIME, 5, 5
    }
};

static const WorldActorDefinition g_forest_actors[] = {
    {
        2, ENTITY_ID_SLIME, 10, 8, DIRECTION_DOWN,
        ACTOR_FLAG_HOSTILE | ACTOR_FLAG_BLOCKING | ACTOR_FLAG_INTERACTABLE,
        'E', INTERACTION_COMBAT, DIALOGUE_ID_NONE, BATTLE_SLIME, 6, 6
    },
    {
        3, ENTITY_ID_BAT, 7, 4, DIRECTION_DOWN,
        ACTOR_FLAG_HOSTILE | ACTOR_FLAG_BLOCKING | ACTOR_FLAG_INTERACTABLE,
        'V', INTERACTION_COMBAT, DIALOGUE_ID_NONE, BATTLE_BAT, 4, 4
    }
};

static const WorldActorDefinition g_mountain_pass_actors[] = {
    {
        4, ENTITY_ID_SLIME, 14, 7, DIRECTION_DOWN,
        ACTOR_FLAG_HOSTILE | ACTOR_FLAG_BLOCKING | ACTOR_FLAG_INTERACTABLE,
        'E', INTERACTION_COMBAT, DIALOGUE_ID_NONE, BATTLE_SLIME, 8, 8
    }
};

static const WorldActorDefinition g_castle_actors[] = {
    {
        5, ENTITY_ID_BAT, 12, 7, DIRECTION_DOWN,
        ACTOR_FLAG_HOSTILE | ACTOR_FLAG_BLOCKING | ACTOR_FLAG_INTERACTABLE,
        'V', INTERACTION_COMBAT, DIALOGUE_ID_NONE, BATTLE_BAT, 4, 4
    }
};

static const WorldActorDefinition *actor_defs_for_map(MapId map_id, uint8_t *count)
{
    switch (map_id) {
        case MAP_TOWN:
            if (count) *count = (uint8_t)(sizeof(g_town_actors) / sizeof(g_town_actors[0]));
            return g_town_actors;
        case MAP_FIELD:
            if (count) *count = (uint8_t)(sizeof(g_field_actors) / sizeof(g_field_actors[0]));
            return g_field_actors;
        case MAP_FOREST:
            if (count) *count = (uint8_t)(sizeof(g_forest_actors) / sizeof(g_forest_actors[0]));
            return g_forest_actors;
        case MAP_MOUNTAIN_PASS:
            if (count) *count = (uint8_t)(sizeof(g_mountain_pass_actors) / sizeof(g_mountain_pass_actors[0]));
            return g_mountain_pass_actors;
        case MAP_CASTLE:
            if (count) *count = (uint8_t)(sizeof(g_castle_actors) / sizeof(g_castle_actors[0]));
            return g_castle_actors;
        default:
            if (count) *count = 0;
            return NULL;
    }
}

static const WorldActorDefinition *actor_find_def_by_id(MapId map_id, EntityId id)
{
    const WorldActorDefinition *defs;
    uint8_t count, i;

    defs = actor_defs_for_map(map_id, &count);
    for (i = 0; i < count; i++) {
        if (defs[i].id == id) {
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
        return actor_find_def_by_id(world->map_id, world->actors[slot].id);
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

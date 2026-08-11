#include "actor.h"
#include <stddef.h>

/* ── Scene-owned actor definitions ─────────────────────────────────
 *
 * Actors belong to scenes.  A scene is identified by its MapId (which
 * mirrors SceneId 1:1).  Friendly actors are pure static definitions;
 * hostile actors are spawned into World.enemy by actor_load_scene().
 */

static const WorldActorDefinition g_town_actors[] = {
    {
        ENTITY_ID_MAYOR, 10, 5, DIRECTION_DOWN,
        ACTOR_FLAG_BLOCKING | ACTOR_FLAG_INTERACTABLE,
        'M', INTERACTION_DIALOGUE, DIALOGUE_ID_MAYOR_GREETING, BATTLE_NONE, 0, 0
    },
    {
        ENTITY_ID_GUARD, 10, 8, DIRECTION_DOWN,
        ACTOR_FLAG_BLOCKING | ACTOR_FLAG_INTERACTABLE,
        'G', INTERACTION_DIALOGUE, DIALOGUE_ID_GUARD_GREETING, BATTLE_NONE, 0, 0
    },
    {
        ENTITY_ID_SHOPKEEPER, 9, 3, DIRECTION_DOWN,
        ACTOR_FLAG_BLOCKING | ACTOR_FLAG_INTERACTABLE,
        'S', INTERACTION_DIALOGUE, DIALOGUE_ID_SHOPKEEPER_GREETING, BATTLE_NONE, 0, 0
    }
};

static const WorldActorDefinition g_field_actors[] = {
    {
        ENTITY_ID_SLIME, 14, 8, DIRECTION_DOWN,
        ACTOR_FLAG_HOSTILE | ACTOR_FLAG_BLOCKING | ACTOR_FLAG_INTERACTABLE,
        'E', INTERACTION_COMBAT, DIALOGUE_ID_NONE, BATTLE_SLIME, 5, 5
    }
};

static const WorldActorDefinition g_forest_actors[] = {
    {
        ENTITY_ID_SLIME, 10, 8, DIRECTION_DOWN,
        ACTOR_FLAG_HOSTILE | ACTOR_FLAG_BLOCKING | ACTOR_FLAG_INTERACTABLE,
        'E', INTERACTION_COMBAT, DIALOGUE_ID_NONE, BATTLE_SLIME, 6, 6
    }
};

static const WorldActorDefinition g_mountain_pass_actors[] = {
    {
        ENTITY_ID_SLIME, 14, 7, DIRECTION_DOWN,
        ACTOR_FLAG_HOSTILE | ACTOR_FLAG_BLOCKING | ACTOR_FLAG_INTERACTABLE,
        'E', INTERACTION_COMBAT, DIALOGUE_ID_NONE, BATTLE_SLIME, 8, 8
    }
};

static const WorldActorDefinition g_castle_actors[] = {
    {
        ENTITY_ID_BAT, 12, 7, DIRECTION_DOWN,
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

const WorldActorDefinition *actor_find_at(const World *world, uint8_t x, uint8_t y)
{
    const WorldActorDefinition *defs;
    uint8_t count, i;

    if (!world) return NULL;
    defs = actor_defs_for_map(world->map_id, &count);

    for (i = 0; i < count; i++) {
        if (defs[i].flags & ACTOR_FLAG_HOSTILE) {
            if (world->enemy.active &&
                world->enemy.id == defs[i].id &&
                world->enemy.position.x == x &&
                world->enemy.position.y == y) {
                return &defs[i];
            }
        } else if (defs[i].x == x && defs[i].y == y) {
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

void actor_load_scene(World *world, MapId map_id)
{
    const WorldActorDefinition *defs;
    uint8_t count, i;

    if (!world) return;
    defs = actor_defs_for_map(map_id, &count);

    world->enemy.active = false;
    for (i = 0; i < count; i++) {
        if (defs[i].flags & ACTOR_FLAG_HOSTILE) {
            entity_init(&world->enemy, defs[i].id,
                        defs[i].x, defs[i].y, defs[i].hp, defs[i].max_hp);
            break;
        }
    }
}

uint8_t actor_write_snapshot(const World *world, uint8_t *out, uint8_t max_actors)
{
    const WorldActorDefinition *defs;
    uint8_t count, i, n = 0;

    if (!world || !out || max_actors == 0) return 0;
    defs = actor_defs_for_map(world->map_id, &count);

    for (i = 0; i < count && n < max_actors; i++) {
        if (defs[i].flags & ACTOR_FLAG_HOSTILE) {
            if (!world->enemy.active || world->enemy.id != defs[i].id) {
                continue;   /* defeated / not spawned */
            }
            out[n * ACTOR_SNAPSHOT_ENTRY_SIZE + 0] = (uint8_t)defs[i].id;
            out[n * ACTOR_SNAPSHOT_ENTRY_SIZE + 1] = world->enemy.position.x;
            out[n * ACTOR_SNAPSHOT_ENTRY_SIZE + 2] = world->enemy.position.y;
            out[n * ACTOR_SNAPSHOT_ENTRY_SIZE + 3] = (uint8_t)world->enemy.facing;
        } else {
            out[n * ACTOR_SNAPSHOT_ENTRY_SIZE + 0] = (uint8_t)defs[i].id;
            out[n * ACTOR_SNAPSHOT_ENTRY_SIZE + 1] = defs[i].x;
            out[n * ACTOR_SNAPSHOT_ENTRY_SIZE + 2] = defs[i].y;
            out[n * ACTOR_SNAPSHOT_ENTRY_SIZE + 3] = defs[i].facing;
        }
        n++;
    }
    return n;
}

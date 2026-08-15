#include "actor.h"
#include "game_ids.h"
#include "banked.h"
#include <stddef.h>

/* ── Actor engine ──────────────────────────────────────────────────
 * Per-scene actor definitions live in banked ROM (GAME_CONTENT_BANK).
 * actor_load_scene() copies the current scene's definitions at map load:
 * hostiles into World.actors runtime slots and friendlies into
 * g_static_actors[].  Gameplay lookups are pure WRAM. */

static const WorldActorTable *g_actor_tables = NULL;
static uint8_t g_actor_table_count = 0;
static uint8_t g_actor_bank = 0;

static WorldActorDefinition g_static_actors[4];
static uint8_t g_static_actor_count = 0;

void actor_register_tables(const WorldActorTable *tables, uint8_t count, uint8_t bank)
{
    g_actor_tables = tables;
    g_actor_table_count = count;
    g_actor_bank = bank;
}

static const char *actor_name_for_visual(uint8_t visual)
{
    if (visual == 'V') return "BAT";
    if (visual == 'L') return "LORD OF SLIMES";
    return "SLIME";
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
    r->display_name = actor_name_for_visual(def->visual);
    r->visual = def->visual;
    r->spawn_x = def->x;
    r->spawn_y = def->y;
    r->ai_type = def->ai_type;
    r->ai_step = 0;
    r->ai_timer = PATROL_STEP_INTERVAL;
    r->move_state = 0;
    r->move_target_x = def->x;
    r->move_target_y = def->y;
    r->move_progress = 0;
}

uint8_t actor_find_hostile_slot(const World *world, uint8_t x, uint8_t y)
{
    uint8_t i;
    if (!world) return NO_ACTOR_INDEX;
    for (i = 0; i < MAX_WORLD_ACTORS; i++) {
        if (world->actors[i].active &&
            ((world->actors[i].x == x && world->actors[i].y == y) ||
             (world->actors[i].move_state &&
              world->actors[i].move_target_x == x &&
              world->actors[i].move_target_y == y))) {
            return i;
        }
    }
    return NO_ACTOR_INDEX;
}

const WorldActorDefinition *actor_find_at(const World *world, uint8_t x, uint8_t y)
{
    uint8_t i;
    if (!world) return NULL;

    for (i = 0; i < g_static_actor_count; i++) {
        if (g_static_actors[i].x == x && g_static_actors[i].y == y) {
            return &g_static_actors[i];
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
    static WorldActorTable s_load_tbl;
    static WorldActorDefinition s_load_def;
    uint8_t i, d, slot;

    if (!world) return;

    for (slot = 0; slot < MAX_WORLD_ACTORS; slot++) {
        world->actors[slot].active = 0;
    }
    g_static_actor_count = 0;

    if (!g_actor_tables) return;

    for (i = 0; i < g_actor_table_count; i++) {
        if (g_actor_bank != 0) {
            banked_copy(g_actor_bank, &s_load_tbl, &g_actor_tables[i], sizeof(WorldActorTable));
        } else {
            s_load_tbl = g_actor_tables[i];
        }

        if (s_load_tbl.map_id == map_id) {
            slot = 0;
            for (d = 0; d < s_load_tbl.count; d++) {
                if (g_actor_bank != 0) {
                    banked_copy(g_actor_bank, &s_load_def, &s_load_tbl.defs[d], sizeof(WorldActorDefinition));
                } else {
                    s_load_def = s_load_tbl.defs[d];
                }

                if (s_load_def.actor_id != 0 && game_world_actor_is_defeated(state, s_load_def.actor_id)) {
                    continue;
                }
                if (s_load_def.spawn_variable != 0 &&
                    game_variable_get(state, (VariableId)s_load_def.spawn_variable) != s_load_def.spawn_value) {
                    continue;
                }

                if (s_load_def.flags & ACTOR_FLAG_HOSTILE) {
                    if (slot < MAX_WORLD_ACTORS) {
                        actor_spawn(&world->actors[slot++], &s_load_def);
                    }
                } else if (g_static_actor_count < 4) {
                    g_static_actors[g_static_actor_count++] = s_load_def;
                }
            }
            break;
        }
    }
}

uint8_t actor_write_snapshot(const World *world, uint8_t *out, uint8_t max_actors)
{
    uint8_t i, n = 0;
    uint8_t slot;

    if (!world || !out || max_actors == 0) return 0;

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
    for (i = 0; i < g_static_actor_count && n < max_actors; i++) {
        out[n * ACTOR_SNAPSHOT_ENTRY_SIZE + 0] = (uint8_t)g_static_actors[i].id;
        out[n * ACTOR_SNAPSHOT_ENTRY_SIZE + 1] = g_static_actors[i].x;
        out[n * ACTOR_SNAPSHOT_ENTRY_SIZE + 2] = g_static_actors[i].y;
        out[n * ACTOR_SNAPSHOT_ENTRY_SIZE + 3] = g_static_actors[i].facing;
        n++;
    }
    return n;
}

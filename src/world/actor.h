#ifndef ACTOR_H
#define ACTOR_H

#include <stdint.h>
#include <stdbool.h>
#include "entity.h"
#include "world.h"
#include "dialogue.h"
#include "rpg/state.h"

/* World Actor flags.  Hostility is a property of the actor: a hostile
 * actor starts combat when engaged, a non-hostile actor runs its
 * interaction (e.g. dialogue). */
#define ACTOR_FLAG_HOSTILE      0x01
#define ACTOR_FLAG_BLOCKING     0x02
#define ACTOR_FLAG_INTERACTABLE 0x04

/* Runtime actor state flags (placeholder for future persistence). */
#define ACTOR_STATE_NONE        0x00
/* future: ACTOR_STATE_DEFEATED, ACTOR_STATE_TALKED_TO, ACTOR_STATE_MOVED ... */

/* What happens when the player engages this actor. */
typedef enum {
    INTERACTION_NONE = 0,
    INTERACTION_DIALOGUE = 1,
    INTERACTION_COMBAT = 2,
    INTERACTION_SHOP = 3,
    INTERACTION_SAVE = 4
} InteractionId;

/* Which battle configuration a hostile actor starts.  The battle system
 * owns everything past this point. */
typedef enum {
    BATTLE_NONE = 0,
    BATTLE_SLIME = 1,
    BATTLE_BAT = 2
} BattleId;

/* Overworld autonomous patrol/AI behavior type. */
typedef enum {
    AI_NONE          = 0,
    AI_PATROL_CIRCLE = 1,   /* Clockwise 2x2 circle around spawn (Bats) */
    AI_PATROL_CROSS  = 2    /* + cross pattern around spawn (Slimes) */
} ActorAiType;

/* Static, scene-owned actor configuration.  No mutable gameplay state.
 * actor_id is the stable persistent instance id (ActorId) used to track
 * defeat/lifecycle in GameState.world; it must be unique across scenes. */
typedef struct {
    uint16_t actor_id;
    EntityId id;
    uint8_t x;
    uint8_t y;
    uint8_t facing;
    uint8_t flags;
    uint8_t visual;              /* ASCII prototype character */
    const char *display_name;    /* semantic name (battle enemy label, ...) */
    InteractionId interaction;
    uint8_t shop_id;             /* which shop this actor runs (0 = none) */
    DialogueId dialogue_id;
    BattleId battle_id;
    ActorAiType ai_type;
    uint8_t hp;                  /* hostile actors only */
    uint8_t max_hp;              /* hostile actors only */
    uint8_t gold_reward;         /* gold granted on defeat (0 = none) */
    CurrencyId reward_currency;  /* currency granted on defeat (0 = none) */
    VariableId spawn_variable;   /* spawn only when this variable == spawn_value (0 = always) */
    int16_t spawn_value;
} WorldActorDefinition;

/* A per-map block of actor definitions.  The engine owns no scene content:
 * the game layer registers its tables via actor_register_tables(). */
typedef struct {
    MapId map_id;
    const WorldActorDefinition *defs;
    uint8_t count;
} WorldActorTable;

void actor_register_tables(const WorldActorTable *tables, uint8_t count, uint8_t bank);

/* Result of engaging an actor. */
typedef enum {
    ENGAGE_NONE = 0,
    ENGAGE_DIALOGUE = 1,
    ENGAGE_BATTLE = 2,
    ENGAGE_SHOP = 3,
    ENGAGE_SAVE = 4
} ActorEngageResult;

/* Find the actor definition at a world position on the current map.
 * Friendly actors use their static position; hostile actors resolve
 * against the spawned runtime actor slots. */
const WorldActorDefinition *actor_find_at(const World *world, uint8_t x, uint8_t y);

/* Return the runtime slot index of the active hostile actor at (x, y),
 * or NO_ACTOR_INDEX if none. */
uint8_t actor_find_hostile_slot(const World *world, uint8_t x, uint8_t y);

/* Single generic engagement entry point: hostile actors request combat,
 * everything else runs its interaction (dialogue for v1). */
ActorEngageResult actor_engage(const WorldActorDefinition *actor, DialogueState *dialogue);

/* Spawn all hostile actor definitions for the given map into
 * World.actors runtime slots.  Actors whose ActorId is marked DEFEATED in
 * state are not spawned (persistent defeat). */
void actor_load_scene(World *world, MapId map_id, const GameState *state);

/* Write the active scene's actors as compact (id, x, y, facing) 4-byte
 * entries into out.  Returns the number of entries written.  Used by the
 * debug snapshot. */
#define ACTOR_SNAPSHOT_ENTRY_SIZE 4

uint8_t actor_write_snapshot(const World *world, uint8_t *out, uint8_t max_actors);

#endif /* ACTOR_H */

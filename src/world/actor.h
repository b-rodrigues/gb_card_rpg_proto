#ifndef ACTOR_H
#define ACTOR_H

#include <stdint.h>
#include <stdbool.h>
#include "entity.h"
#include "world.h"
#include "dialogue.h"

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
    INTERACTION_COMBAT = 2
} InteractionId;

/* Which battle configuration a hostile actor starts.  The battle system
 * owns everything past this point. */
typedef enum {
    BATTLE_NONE = 0,
    BATTLE_SLIME = 1,
    BATTLE_BAT = 2
} BattleId;

/* Static, scene-owned actor configuration.  No mutable gameplay state. */
typedef struct {
    EntityId id;
    uint8_t x;
    uint8_t y;
    uint8_t facing;
    uint8_t flags;
    uint8_t visual;              /* ASCII prototype character */
    InteractionId interaction;
    DialogueId dialogue_id;
    BattleId battle_id;
    uint8_t hp;                  /* hostile actors only */
    uint8_t max_hp;              /* hostile actors only */
} WorldActorDefinition;

/* Result of engaging an actor. */
typedef enum {
    ENGAGE_NONE = 0,
    ENGAGE_DIALOGUE = 1,
    ENGAGE_BATTLE = 2
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
 * World.actors runtime slots. */
void actor_load_scene(World *world, MapId map_id);

/* Write the active scene's actors as compact (id, x, y, facing) 4-byte
 * entries into out.  Returns the number of entries written.  Used by the
 * debug snapshot. */
#define ACTOR_SNAPSHOT_ENTRY_SIZE 4

uint8_t actor_write_snapshot(const World *world, uint8_t *out, uint8_t max_actors);

#endif /* ACTOR_H */

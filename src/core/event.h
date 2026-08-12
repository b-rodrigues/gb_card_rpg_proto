#ifndef RPG_EVENT_H
#define RPG_EVENT_H

#include <stdint.h>
#include <stdbool.h>
#include "entity.h"
#include "world.h"
#include "dialogue.h"
#include "story.h"
#include "actor.h"

/* Declarative scripted events connect existing systems (world actors,
 * interaction, dialogue, flags/variables, scenes) without embedding the
 * sequence in gameplay code.  An event fires when its trigger condition is
 * met and executes a fixed list of actions.  First match in table order
 * wins, so more specific conditions must be listed first. */

/* Stable event identifiers (telemetry + host EVENT_ID_MAP). */
typedef enum {
    EVENT_ID_NONE = 0,
    EVENT_ID_TOWN_ARRIVAL = 1,
    EVENT_ID_QUEST_START = 2,
    EVENT_ID_QUEST_ACTIVE = 3,
    EVENT_ID_QUEST_COMPLETE = 4,
    EVENT_ID_QUEST_DONE = 5,
    EVENT_ID_GUARD_AFTER_MAYOR = 6,
    EVENT_ID_GUARD_GREETING = 7,
    EVENT_ID_MONSTER_DEFEATED = 8,
    EVENT_ID_BOSS_DEFEATED = 9
} EventId;

typedef enum {
    EVENT_TRIGGER_INTERACT = 1,      /* player engages a specific actor */
    EVENT_TRIGGER_MAP_ENTER = 2,     /* player enters a scene */
    EVENT_TRIGGER_ACTOR_DEFEATED = 3 /* a hostile actor was defeated in battle */
} EventTriggerType;

/* Generic event condition.  A FLAG cond requires flag is/not set; a
 * VARIABLE cond requires value >= threshold (at_least) or == threshold. */
typedef enum {
    EVENT_COND_NONE = 0,
    EVENT_COND_FLAG = 1,
    EVENT_COND_VARIABLE = 2
} EventCondType;

typedef struct {
    EventCondType type;
    uint16_t id;        /* StoryFlagId / VariableId */
    int16_t value;      /* variable threshold */
    bool flag_set;      /* FLAG: required state */
    bool at_least;      /* VARIABLE: >= when true, == when false */
} EventCond;

#define MAX_EVENT_CONDS 3

typedef enum {
    EVENT_ACTION_NONE = 0,
    EVENT_ACTION_DIALOGUE = 1,
    EVENT_ACTION_SET_FLAG = 2,
    EVENT_ACTION_CLEAR_FLAG = 3,
    EVENT_ACTION_SET_VARIABLE = 4,
    EVENT_ACTION_ADD_VARIABLE = 5,
    EVENT_ACTION_SCENE_CHANGE = 6,
    EVENT_ACTION_ADD_ITEM = 7
} EventActionType;

typedef struct {
    EventActionType type;
    uint8_t arg0;   /* DialogueId / StoryFlagId / VariableId / SceneId / ItemId */
    int16_t arg1;   /* value / spawn x / delta / quantity */
    int16_t arg2;   /* spawn y */
} EventAction;

#define MAX_EVENT_ACTIONS 4

typedef struct {
    EventId id;
    EventTriggerType trigger;
    EntityId actor;             /* INTERACT: which actor (0 = any) */
    MapId map;                  /* MAP_ENTER: which map (0 = any) */
    uint8_t cond_count;
    EventCond conds[MAX_EVENT_CONDS];
    uint8_t action_count;
    EventAction actions[MAX_EVENT_ACTIONS];
} EventDefinition;

/* Sentinel meaning "any map" for a MAP_ENTER event target. */
#define EVENT_MAP_ANY 0xFF

/* Resolve the first matching INTERACT event for the given actor and run its
 * actions.  Returns the engage result the screen should act on:
 * ENGAGE_DIALOGUE if a dialogue was started, else ENGAGE_NONE. */
ActorEngageResult event_engage_actor(Game *g, const WorldActorDefinition *actor);

/* Resolve the first matching MAP_ENTER event for the map and run it. */
void event_resolve_map_enter(Game *g, MapId to_map);

/* Resolve the first matching ACTOR_DEFEATED event for the defeated actor and
 * run its actions (e.g. quest progress counters, final-boss ending). */
void event_resolve_actor_defeated(Game *g, ActorId actor_id, EntityId entity_id);

#endif /* RPG_EVENT_H */

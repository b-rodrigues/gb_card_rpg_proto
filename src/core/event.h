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
    EVENT_ID_MAYOR_INTRO = 2,
    EVENT_ID_MAYOR_GREETING = 3,
    EVENT_ID_GUARD_AFTER_MAYOR = 4,
    EVENT_ID_GUARD_GREETING = 5
} EventId;

typedef enum {
    EVENT_TRIGGER_INTERACT = 1,   /* player engages a specific actor */
    EVENT_TRIGGER_MAP_ENTER = 2   /* player enters a scene */
} EventTriggerType;

typedef enum {
    EVENT_ACTION_NONE = 0,
    EVENT_ACTION_DIALOGUE = 1,
    EVENT_ACTION_SET_FLAG = 2,
    EVENT_ACTION_CLEAR_FLAG = 3,
    EVENT_ACTION_SET_VARIABLE = 4,
    EVENT_ACTION_ADD_VARIABLE = 5,
    EVENT_ACTION_SCENE_CHANGE = 6
} EventActionType;

typedef struct {
    EventActionType type;
    uint8_t arg0;   /* DialogueId / StoryFlagId / VariableId / SceneId */
    int16_t arg1;   /* value / spawn x / delta */
    int16_t arg2;   /* spawn y */
} EventAction;

#define MAX_EVENT_ACTIONS 4

typedef struct {
    EventId id;
    EventTriggerType trigger;
    EntityId actor;             /* INTERACT: which actor (0 = any) */
    MapId map;                  /* MAP_ENTER: which map (0 = any) */
    StoryFlagId req_flag;       /* 0 = no flag condition */
    bool req_flag_set;          /* required value when req_flag != 0 */
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

#endif /* RPG_EVENT_H */

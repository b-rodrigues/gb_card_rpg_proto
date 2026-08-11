#include "event.h"
#include "game.h"
#include "telemetry.h"
#include "screen.h"
#include <stddef.h>

/* ── Event table ────────────────────────────────────────────────────
 * First match in table order wins.  For INTERACT events, more specific
 * conditions (a flag requirement) must precede the default fallback.
 *
 * TOWN_ARRIVAL   : entering town for the first time (ARRIVED_TOWN unset)
 *                  marks ARRIVED_TOWN.  Replaces story_on_map_enter().
 * MAYOR_INTRO    : first meeting with the Mayor starts the intro dialogue
 *                  and sets MET_MAYOR immediately.
 * MAYOR_GREETING : the Mayor's already-met greeting.
 * GUARD_AFTER_MAYOR : the Guard acknowledges the hero once MET_MAYOR.
 * GUARD_GREETING : the Guard's default greeting before MET_MAYOR.
 */
static const EventDefinition g_event_defs[] = {
    {
        EVENT_ID_TOWN_ARRIVAL, EVENT_TRIGGER_MAP_ENTER, ENTITY_ID_NONE, MAP_TOWN,
        STORY_FLAG_ID_ARRIVED_TOWN, false,
        1,
        {{ EVENT_ACTION_SET_FLAG, STORY_FLAG_ID_ARRIVED_TOWN, 0, 0 }}
    },
    {
        EVENT_ID_MAYOR_INTRO, EVENT_TRIGGER_INTERACT, ENTITY_ID_MAYOR, EVENT_MAP_ANY,
        STORY_FLAG_ID_MET_MAYOR, false,
        2,
        {
            { EVENT_ACTION_DIALOGUE, DIALOGUE_ID_MAYOR_INTRO, 0, 0 },
            { EVENT_ACTION_SET_FLAG, STORY_FLAG_ID_MET_MAYOR, 0, 0 }
        }
    },
    {
        EVENT_ID_MAYOR_GREETING, EVENT_TRIGGER_INTERACT, ENTITY_ID_MAYOR, EVENT_MAP_ANY,
        STORY_FLAG_ID_MET_MAYOR, true,
        1,
        {{ EVENT_ACTION_DIALOGUE, DIALOGUE_ID_MAYOR_GREETING, 0, 0 }}
    },
    {
        EVENT_ID_GUARD_AFTER_MAYOR, EVENT_TRIGGER_INTERACT, ENTITY_ID_GUARD, EVENT_MAP_ANY,
        STORY_FLAG_ID_MET_MAYOR, true,
        1,
        {{ EVENT_ACTION_DIALOGUE, DIALOGUE_ID_GUARD_AFTER_MAYOR, 0, 0 }}
    },
    {
        EVENT_ID_GUARD_GREETING, EVENT_TRIGGER_INTERACT, ENTITY_ID_GUARD, EVENT_MAP_ANY,
        STORY_FLAG_ID_MET_MAYOR, false,
        1,
        {{ EVENT_ACTION_DIALOGUE, DIALOGUE_ID_GUARD_GREETING, 0, 0 }}
    }
};

#define NUM_EVENT_DEFS (sizeof(g_event_defs) / sizeof(g_event_defs[0]))

static bool event_condition_met(const GameState *state, const EventDefinition *def)
{
    if (def->req_flag != 0) {
        if (story_has_flag(state, def->req_flag) != def->req_flag_set) {
            return false;
        }
    }
    return true;
}

static void event_execute_actions(Game *g, const EventDefinition *def,
                                  bool *dialogue_started)
{
    uint8_t i;
    for (i = 0; i < def->action_count; i++) {
        const EventAction *a = &def->actions[i];
        switch (a->type) {
            case EVENT_ACTION_DIALOGUE:
                dialogue_start_def(&g->dialogue, (DialogueId)a->arg0);
                if (dialogue_started) *dialogue_started = true;
                break;
            case EVENT_ACTION_SET_FLAG:
                story_set_flag(&g->state, (StoryFlagId)a->arg0);
                break;
            case EVENT_ACTION_CLEAR_FLAG:
                story_clear_flag(&g->state, (StoryFlagId)a->arg0);
                break;
            case EVENT_ACTION_SET_VARIABLE:
                game_variable_set(&g->state, (VariableId)a->arg0, a->arg1);
                break;
            case EVENT_ACTION_ADD_VARIABLE:
                game_variable_add(&g->state, (VariableId)a->arg0, a->arg1);
                break;
            case EVENT_ACTION_SCENE_CHANGE:
                scene_load(g, (SceneId)a->arg0, (uint8_t)a->arg1, (uint8_t)a->arg2);
                break;
            default:
                break;
        }
    }
}

ActorEngageResult event_engage_actor(Game *g, const WorldActorDefinition *actor)
{
    uint8_t i;
    bool dialogue_started = false;
    const EventDefinition *def;

    if (!g || !actor) return ENGAGE_NONE;

    for (i = 0; i < NUM_EVENT_DEFS; i++) {
        def = &g_event_defs[i];
        if (def->trigger != EVENT_TRIGGER_INTERACT) continue;
        if (def->actor != ENTITY_ID_NONE && def->actor != actor->id) continue;
        if (!event_condition_met(&g->state, def)) continue;

        telemetry_emit(EVENT_SCRIPT_TRIGGERED, (uint8_t)def->id, 0, 0, 0);
        event_execute_actions(g, def, &dialogue_started);
        return dialogue_started ? ENGAGE_DIALOGUE : ENGAGE_NONE;
    }
    return ENGAGE_NONE;
}

void event_resolve_map_enter(Game *g, MapId to_map)
{
    uint8_t i;
    bool dialogue_started = false;
    const EventDefinition *def;

    if (!g) return;

    for (i = 0; i < NUM_EVENT_DEFS; i++) {
        def = &g_event_defs[i];
        if (def->trigger != EVENT_TRIGGER_MAP_ENTER) continue;
        if (def->map != EVENT_MAP_ANY && def->map != to_map) continue;
        if (!event_condition_met(&g->state, def)) continue;

        telemetry_emit(EVENT_SCRIPT_TRIGGERED, (uint8_t)def->id, 0, 0, 0);
        event_execute_actions(g, def, &dialogue_started);
        return;
    }
}

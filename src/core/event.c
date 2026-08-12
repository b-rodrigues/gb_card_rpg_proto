#include "event.h"
#include "game.h"
#include "telemetry.h"
#include "screen.h"
#include "rpg/items.h"
#include "rpg/inventory.h"
#include "rpg/currency.h"
#include "rpg/progression.h"
#include <stddef.h>

/* ── Event table ────────────────────────────────────────────────────
 * First match in table order wins.  For INTERACT events, more specific
 * conditions must precede the default fallback.
 *
 * Mayor quest (MONSTER HUNT), expressed as data, not game code:
 *   QUEST_START    : first meeting (quest NOT_STARTED) starts the quest:
 *                    MAYOR_INTRO dialogue, MET_MAYOR, quest=ACTIVE.
 *   QUEST_COMPLETE : quest ACTIVE and >= 3 monsters defeated -> reward
 *                    dialogue, give SWORD, quest=COMPLETE (given once).
 *   QUEST_ACTIVE   : quest ACTIVE (fewer than 3 defeated) -> "still working".
 *   QUEST_DONE     : quest COMPLETE -> already-rewarded dialogue.
 *   MONSTER_DEFEATED : every hostile defeat increments the global
 *                    MONSTERS_DEFEATED counter (no quest gating), so kills
 *                    before the quest starts still count.
 */
static const EventDefinition g_event_defs[] = {
    {
        EVENT_ID_TOWN_ARRIVAL, EVENT_TRIGGER_MAP_ENTER, ENTITY_ID_NONE, MAP_TOWN,
        1,
        {{ EVENT_COND_FLAG, STORY_FLAG_ID_ARRIVED_TOWN, 0, false, false }},
        1,
        {{ EVENT_ACTION_SET_FLAG, STORY_FLAG_ID_ARRIVED_TOWN, 0, 0 }}
    },
    {
        EVENT_ID_QUEST_START, EVENT_TRIGGER_INTERACT, ENTITY_ID_MAYOR, EVENT_MAP_ANY,
        1,
        {{ EVENT_COND_VARIABLE, VARIABLE_ID_QUEST_MONSTER_HUNT, 0, false, false }},
        3,
        {
            { EVENT_ACTION_DIALOGUE, DIALOGUE_ID_MAYOR_INTRO, 0, 0 },
            { EVENT_ACTION_SET_FLAG, STORY_FLAG_ID_MET_MAYOR, 0, 0 },
            { EVENT_ACTION_SET_VARIABLE, VARIABLE_ID_QUEST_MONSTER_HUNT, 1, 0 }
        }
    },
    {
        EVENT_ID_QUEST_COMPLETE, EVENT_TRIGGER_INTERACT, ENTITY_ID_MAYOR, EVENT_MAP_ANY,
        2,
        {
            { EVENT_COND_VARIABLE, VARIABLE_ID_QUEST_MONSTER_HUNT, 1, false, false },
            { EVENT_COND_VARIABLE, VARIABLE_ID_MONSTERS_DEFEATED, 3, false, true }
        },
        3,
        {
            { EVENT_ACTION_DIALOGUE, DIALOGUE_ID_QUEST_COMPLETE, 0, 0 },
            { EVENT_ACTION_ADD_ITEM, ITEM_SWORD, 1, 0 },
            { EVENT_ACTION_SET_VARIABLE, VARIABLE_ID_QUEST_MONSTER_HUNT, 2, 0 }
        }
    },
    {
        EVENT_ID_QUEST_ACTIVE, EVENT_TRIGGER_INTERACT, ENTITY_ID_MAYOR, EVENT_MAP_ANY,
        1,
        {{ EVENT_COND_VARIABLE, VARIABLE_ID_QUEST_MONSTER_HUNT, 1, false, false }},
        1,
        {{ EVENT_ACTION_DIALOGUE, DIALOGUE_ID_QUEST_ACTIVE, 0, 0 }}
    },
    {
        EVENT_ID_QUEST_DONE, EVENT_TRIGGER_INTERACT, ENTITY_ID_MAYOR, EVENT_MAP_ANY,
        1,
        {{ EVENT_COND_VARIABLE, VARIABLE_ID_QUEST_MONSTER_HUNT, 2, false, false }},
        1,
        {{ EVENT_ACTION_DIALOGUE, DIALOGUE_ID_QUEST_DONE, 0, 0 }}
    },
    {
        EVENT_ID_BOSS_DEFEATED, EVENT_TRIGGER_ACTOR_DEFEATED, ENTITY_ID_SLIME_LORD, EVENT_MAP_ANY,
        0,
        {{ EVENT_COND_NONE, 0, 0, false, false }},
        1,
        {{ EVENT_ACTION_SET_VARIABLE, VARIABLE_ID_ENDING_SHOWN, 1, 0 }}
    },
    {
        EVENT_ID_MONSTER_DEFEATED, EVENT_TRIGGER_ACTOR_DEFEATED, ENTITY_ID_NONE, EVENT_MAP_ANY,
        0,
        {{ EVENT_COND_NONE, 0, 0, false, false }},
        1,
        {{ EVENT_ACTION_ADD_VARIABLE, VARIABLE_ID_MONSTERS_DEFEATED, 1, 0 }}
    },
    {
        EVENT_ID_GUARD_AFTER_MAYOR, EVENT_TRIGGER_INTERACT, ENTITY_ID_GUARD, EVENT_MAP_ANY,
        1,
        {{ EVENT_COND_FLAG, STORY_FLAG_ID_MET_MAYOR, 0, true, false }},
        1,
        {{ EVENT_ACTION_DIALOGUE, DIALOGUE_ID_GUARD_AFTER_MAYOR, 0, 0 }}
    },
    {
        EVENT_ID_GUARD_GREETING, EVENT_TRIGGER_INTERACT, ENTITY_ID_GUARD, EVENT_MAP_ANY,
        1,
        {{ EVENT_COND_FLAG, STORY_FLAG_ID_MET_MAYOR, 0, false, false }},
        1,
        {{ EVENT_ACTION_DIALOGUE, DIALOGUE_ID_GUARD_GREETING, 0, 0 }}
    }
};

#define NUM_EVENT_DEFS (sizeof(g_event_defs) / sizeof(g_event_defs[0]))

static bool event_condition_met(const GameState *state, const EventCond *cond)
{
    int16_t v;
    switch (cond->type) {
        case EVENT_COND_FLAG:
            return story_has_flag(state, (StoryFlagId)cond->id) == cond->flag_set;
        case EVENT_COND_VARIABLE:
            v = game_variable_get(state, (VariableId)cond->id);
            if (cond->at_least) return v >= cond->value;
            return v == cond->value;
        default:
            return true;
    }
}

static bool event_conds_met(const GameState *state, const EventDefinition *def)
{
    uint8_t i;
    for (i = 0; i < def->cond_count; i++) {
        if (!event_condition_met(state, &def->conds[i])) return false;
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
            case EVENT_ACTION_ADD_ITEM:
                inventory_add(&g->state.inventory, (ItemId)a->arg0, (uint8_t)a->arg1);
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
        if (!event_conds_met(&g->state, def)) continue;

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
        if (!event_conds_met(&g->state, def)) continue;

        telemetry_emit(EVENT_SCRIPT_TRIGGERED, (uint8_t)def->id, 0, 0, 0);
        event_execute_actions(g, def, &dialogue_started);
        return;
    }
}

void event_resolve_actor_defeated(Game *g, ActorId actor_id, EntityId entity_id)
{
    uint8_t i;
    bool dialogue_started = false;
    const EventDefinition *def;

    /* Runs for every hostile defeat, including non-persistent actors
     * (actor_id 0), so their kills count toward quest progress. */
    if (!g) return;
    (void)actor_id;

    for (i = 0; i < NUM_EVENT_DEFS; i++) {
        def = &g_event_defs[i];
        if (def->trigger != EVENT_TRIGGER_ACTOR_DEFEATED) continue;
        if (def->actor != ENTITY_ID_NONE && def->actor != entity_id) continue;
        if (!event_conds_met(&g->state, def)) continue;

        telemetry_emit(EVENT_SCRIPT_TRIGGERED, (uint8_t)def->id, 0, 0, 0);
        event_execute_actions(g, def, &dialogue_started);
        return;
    }
}

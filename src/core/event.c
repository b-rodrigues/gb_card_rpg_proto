#include "event.h"
#include "game.h"
#include "telemetry.h"
#include "screen.h"
#include "rpg/inventory.h"
#include "rpg/currency.h"

/* ── Event engine ──────────────────────────────────────────────────
 * The engine matches events against a table registered by the game layer
 * (event_init, called from src/game/events.c).  First match wins; for
 * INTERACT events more specific conditions must precede the fallback. */
static const EventDefinition *g_events = NULL;
static uint8_t g_event_count = 0;

void event_init(const EventDefinition *table, uint8_t count)
{
    g_events = table;
    g_event_count = count;
}

static bool event_condition_met(const GameState *state, const EventCond *cond)
{
    int16_t v;
    switch (cond->type) {
        case EVENT_COND_FLAG:
            return story_has_flag(state, (FlagId)cond->id) == cond->flag_set;
        case EVENT_COND_VARIABLE:
            v = game_variable_get(state, (VariableId)cond->id);
            if (cond->at_least) return v >= cond->value;
            return v == cond->value;
        case EVENT_COND_ITEM_COUNT:
            v = (int16_t)inventory_count(&state->inventory, (ItemId)cond->id);
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
                story_set_flag(&g->state, (FlagId)a->arg0);
                break;
            case EVENT_ACTION_CLEAR_FLAG:
                story_clear_flag(&g->state, (FlagId)a->arg0);
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
            case EVENT_ACTION_ADD_CURRENCY:
                currency_add(&g->state, (CurrencyId)a->arg0, a->arg1);
                break;
            case EVENT_ACTION_REMOVE_ITEM:
                inventory_remove(&g->state.inventory, (ItemId)a->arg0, (uint8_t)a->arg1);
                break;
            default:
                break;
        }
    }
}

static const EventDefinition *event_first_match(Game *g, EventTriggerType trigger,
                                                EntityId actor, MapId map)
{
    uint8_t i;
    if (!g_events) return NULL;
    for (i = 0; i < g_event_count; i++) {
        const EventDefinition *def = &g_events[i];
        if (def->trigger != trigger) continue;
        if (actor != ENTITY_ID_NONE && def->actor != ENTITY_ID_NONE && def->actor != actor) continue;
        if (map != EVENT_MAP_ANY && def->map != EVENT_MAP_ANY && def->map != map) continue;
        if (!event_conds_met(&g->state, def)) continue;
        return def;
    }
    return NULL;
}

ActorEngageResult event_engage_actor(Game *g, const WorldActorDefinition *actor)
{
    bool dialogue_started = false;
    const EventDefinition *def;

    if (!g || !actor) return ENGAGE_NONE;

    def = event_first_match(g, EVENT_TRIGGER_INTERACT, actor->id, EVENT_MAP_ANY);
    if (def) {
        telemetry_emit(EVENT_SCRIPT_TRIGGERED, (uint8_t)def->id, 0, 0, 0);
        event_execute_actions(g, def, &dialogue_started);
        return dialogue_started ? ENGAGE_DIALOGUE : ENGAGE_NONE;
    }
    return ENGAGE_NONE;
}

void event_resolve_map_enter(Game *g, MapId to_map)
{
    bool dialogue_started = false;
    const EventDefinition *def;

    if (!g) return;

    def = event_first_match(g, EVENT_TRIGGER_MAP_ENTER, ENTITY_ID_NONE, to_map);
    if (def) {
        telemetry_emit(EVENT_SCRIPT_TRIGGERED, (uint8_t)def->id, 0, 0, 0);
        event_execute_actions(g, def, &dialogue_started);
    }
}

void event_resolve_actor_defeated(Game *g, ActorId actor_id, EntityId entity_id)
{
    bool dialogue_started = false;
    const EventDefinition *def;

    /* Runs for every hostile defeat, including non-persistent actors
     * (actor_id 0), so their kills count toward quest progress. */
    if (!g) return;
    (void)actor_id;

    def = event_first_match(g, EVENT_TRIGGER_ACTOR_DEFEATED, entity_id, EVENT_MAP_ANY);
    if (def) {
        telemetry_emit(EVENT_SCRIPT_TRIGGERED, (uint8_t)def->id, 0, 0, 0);
        event_execute_actions(g, def, &dialogue_started);
    }
}

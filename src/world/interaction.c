#include "interaction.h"
#include "game.h"
#include "event.h"
#include "telemetry.h"
#include "story.h"
#include <stddef.h>

ActorEngageResult interaction_try_at(Game *g, uint8_t target_x, uint8_t target_y)
{
    const WorldActorDefinition *actor;
    ActorEngageResult result;
    uint8_t slot;

    if (!g) return ENGAGE_NONE;

    actor = actor_find_at(&g->world, target_x, target_y);
    if (!actor) return ENGAGE_NONE;

    telemetry_emit(EVENT_ACTOR_INTERACTION, target_x, target_y,
                   (uint8_t)actor->id, (uint8_t)actor->interaction);

    /* Hostile actors always start combat (events don't override this). */
    if (actor->flags & ACTOR_FLAG_HOSTILE) {
        /* Record the hostile runtime slot so battle can read its HP. */
        slot = actor_find_hostile_slot(&g->world, target_x, target_y);
        g->world.encounter_actor_index = slot;
        return ENGAGE_BATTLE;
    }

    /* Non-hostile actors resolve through the scripted event system first;
     * the static interaction (dialogue) is the fallback when no event
     * matches (e.g. the shopkeeper). */
    result = event_engage_actor(g, actor);
    if (result != ENGAGE_NONE) {
        return result;
    }
    return actor_engage(actor, &g->dialogue);
}

ActorEngageResult interaction_try_facing(Game *g)
{
    int8_t dx = 0;
    int8_t dy = 0;
    uint8_t px, py, target_x, target_y;
    const WorldActorDefinition *actor;

    if (!g) return ENGAGE_NONE;

    px = g->world.player.position.x;
    py = g->world.player.position.y;

    switch (g->world.player.facing) {
        case DIRECTION_UP:    dy = -1; break;
        case DIRECTION_DOWN:  dy = 1;  break;
        case DIRECTION_LEFT:  dx = -1; break;
        case DIRECTION_RIGHT: dx = 1;  break;
    }

    target_x = (uint8_t)((int16_t)px + dx);
    target_y = (uint8_t)((int16_t)py + dy);

    actor = actor_find_at(&g->world, target_x, target_y);
    telemetry_emit(EVENT_INTERACTION_ATTEMPT, target_x, target_y,
                   (uint8_t)g->world.player.facing,
                   actor ? (uint8_t)actor->id : 0);

    return interaction_try_at(g, target_x, target_y);
}

ActorEngageResult interaction_try_bump(Game *g, int8_t dx, int8_t dy)
{
    uint8_t px, py, target_x, target_y;

    if (!g || (dx == 0 && dy == 0)) return ENGAGE_NONE;

    px = g->world.player.position.x;
    py = g->world.player.position.y;

    target_x = (uint8_t)((int16_t)px + dx);
    target_y = (uint8_t)((int16_t)py + dy);

    return interaction_try_at(g, target_x, target_y);
}

void interaction_on_dialogue_end(DialogueState *dialogue, GameState *state)
{
    if (!dialogue || !state) return;
    if (dialogue->completion_flag != 0) {
        story_set_flag(state, dialogue->completion_flag);
    }
}

#include "interaction.h"
#include "telemetry.h"
#include "story.h"
#include <stddef.h>

ActorEngageResult interaction_try_at(const World *world, uint8_t target_x, uint8_t target_y, DialogueState *dialogue)
{
    const WorldActorDefinition *actor;
    ActorEngageResult result;

    if (!world || !dialogue) return ENGAGE_NONE;

    actor = actor_find_at(world, target_x, target_y);
    if (!actor) return ENGAGE_NONE;

    telemetry_emit(EVENT_ACTOR_INTERACTION, target_x, target_y,
                   (uint8_t)actor->id, (uint8_t)actor->interaction);

    result = actor_engage(actor, dialogue);
    return result;
}

ActorEngageResult interaction_try_facing(const World *world, DialogueState *dialogue)
{
    int8_t dx = 0;
    int8_t dy = 0;
    uint8_t px, py, target_x, target_y;
    const WorldActorDefinition *actor;

    if (!world || !dialogue) return ENGAGE_NONE;

    px = world->player.position.x;
    py = world->player.position.y;

    switch (world->player.facing) {
        case DIRECTION_UP:    dy = -1; break;
        case DIRECTION_DOWN:  dy = 1;  break;
        case DIRECTION_LEFT:  dx = -1; break;
        case DIRECTION_RIGHT: dx = 1;  break;
    }

    target_x = (uint8_t)((int16_t)px + dx);
    target_y = (uint8_t)((int16_t)py + dy);

    actor = actor_find_at(world, target_x, target_y);
    telemetry_emit(EVENT_INTERACTION_ATTEMPT, target_x, target_y,
                   (uint8_t)world->player.facing,
                   actor ? (uint8_t)actor->id : 0);

    return interaction_try_at(world, target_x, target_y, dialogue);
}

ActorEngageResult interaction_try_bump(const World *world, int8_t dx, int8_t dy, DialogueState *dialogue)
{
    uint8_t px, py, target_x, target_y;

    if (!world || !dialogue || (dx == 0 && dy == 0)) return ENGAGE_NONE;

    px = world->player.position.x;
    py = world->player.position.y;

    target_x = (uint8_t)((int16_t)px + dx);
    target_y = (uint8_t)((int16_t)py + dy);

    return interaction_try_at(world, target_x, target_y, dialogue);
}

void interaction_on_dialogue_end(DialogueState *dialogue, uint32_t *story_flags)
{
    if (!dialogue || !story_flags) return;
    if (dialogue->completion_flag != 0) {
        story_set_flag(story_flags, dialogue->completion_flag);
    }
}

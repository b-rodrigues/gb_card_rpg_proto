#include "interaction.h"
#include "telemetry.h"
#include "story.h"
#include <stddef.h>

bool interaction_try_at(MapId map_id, uint8_t target_x, uint8_t target_y, DialogueState *dialogue)
{
    const NpcDef *npc;
    if (!dialogue) return false;

    npc = npc_find_at(map_id, target_x, target_y);
    if (!npc) return false;

    /* Emit NPC collision telemetry: entity_type=ENTITY_NPC, entity_id=npc->id */
    telemetry_emit(EVENT_COLLISION, target_x, target_y, ENTITY_NPC, (uint8_t)npc->id);

    /* Start data-driven dialogue for this NPC */
    dialogue_start_def(dialogue, npc->dialogue_id);
    return true;
}

bool interaction_try_facing(const World *world, DialogueState *dialogue)
{
    int8_t dx = 0;
    int8_t dy = 0;
    uint8_t px, py, target_x, target_y;

    if (!world || !dialogue) return false;

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

    {
        const NpcDef *npc = npc_find_at(world->map_id, target_x, target_y);
        uint8_t target_entity = npc ? (uint8_t)npc->id : 0;
        telemetry_emit(EVENT_INTERACTION_ATTEMPT, target_x, target_y, (uint8_t)world->player.facing, target_entity);
    }

    return interaction_try_at(world->map_id, target_x, target_y, dialogue);
}

bool interaction_try_bump(const World *world, int8_t dx, int8_t dy, DialogueState *dialogue)
{
    uint8_t px, py, target_x, target_y;

    if (!world || !dialogue || (dx == 0 && dy == 0)) return false;

    px = world->player.position.x;
    py = world->player.position.y;

    target_x = (uint8_t)((int16_t)px + dx);
    target_y = (uint8_t)((int16_t)py + dy);

    return interaction_try_at(world->map_id, target_x, target_y, dialogue);
}

void interaction_on_dialogue_end(DialogueState *dialogue, uint32_t *story_flags)
{
    if (!dialogue || !story_flags) return;
    if (dialogue->completion_flag != 0) {
        story_set_flag(story_flags, dialogue->completion_flag);
    }
}

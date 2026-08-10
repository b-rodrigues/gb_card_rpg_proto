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

void interaction_on_dialogue_end(DialogueState *dialogue, uint32_t *story_flags)
{
    if (!dialogue || !story_flags) return;
    if (dialogue->completion_flag != 0) {
        story_set_flag(story_flags, dialogue->completion_flag);
    }
}

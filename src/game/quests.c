#include "quest.h"
#include "game_ids.h"
#include <stddef.h>

/* ── Quest registry (game content) ─────────────────────────────────
 * Registered with the generic quest engine via game_quest_register().
 * The QUEST menu in the item screen iterates the engine's table, so every
 * quest listed here appears automatically with its name, status and (where
 * relevant) a progress counter. */
static const QuestDefinition g_quests[] = {
    {
        1, "MONSTER HUNT",
        VARIABLE_ID_QUEST_MONSTER_HUNT, 1, 2,
        VARIABLE_ID_MONSTERS_DEFEATED, 3, "monsters", "SWORD"
    },
    {
        2, "LOST AMULET",
        VARIABLE_ID_MERCHANT_QUEST, 1, 2,
        0, 0, "", "MERCHANT"
    }
};

void game_quest_register(void)
{
    quest_init(g_quests, (uint8_t)(sizeof(g_quests) / sizeof(g_quests[0])));
}

#pragma bank 2

#include "quest.h"
#include "game_ids.h"
#include <stddef.h>

/* ── Quest registry (game content, banked ROM) ───────────────────── */
const QuestDefinition g_quests[] = {
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

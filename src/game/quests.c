#include "quest.h"
#include "game_ids.h"

extern const QuestDefinition g_quests[];
#define GAME_QUEST_COUNT 2

void game_quest_register(void)
{
    quest_init(g_quests, GAME_QUEST_COUNT, GAME_CONTENT_BANK);
}

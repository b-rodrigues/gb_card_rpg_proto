#include "quest.h"
#include <stddef.h>

/* ── Quest engine ──────────────────────────────────────────────────
 * The quest table is game content, registered at boot via quest_init().
 * The engine only iterates the table and derives status from the canonical
 * state; all quest behavior (starts, progress, completion) is expressed by
 * the game's event table, not engine code. */
static const QuestDefinition *g_quests = NULL;
static uint8_t g_quest_count = 0;

void quest_init(const QuestDefinition *table, uint8_t count)
{
    g_quests = table;
    g_quest_count = count;
}

uint8_t quest_count(void)
{
    return g_quest_count;
}

const QuestDefinition *quest_at(uint8_t idx)
{
    if (!g_quests || idx >= g_quest_count) return NULL;
    return &g_quests[idx];
}

QuestStatus quest_status(const GameState *state, const QuestDefinition *q)
{
    int16_t v;
    if (!state || !q || q->status_variable == 0) {
        return QUEST_STATUS_NOT_STARTED;
    }
    v = game_variable_get(state, q->status_variable);
    if (v == q->status_complete) return QUEST_STATUS_COMPLETE;
    if (v == q->status_active) return QUEST_STATUS_ACTIVE;
    return QUEST_STATUS_NOT_STARTED;
}

#ifndef QUEST_H
#define QUEST_H

#include <stdint.h>
#include <stdbool.h>
#include "rpg/state.h"

/* Generic quest engine.  The engine owns the quest vocabulary and the
 * status derivation; the game layer supplies its quest table at boot via
 * quest_init() (see src/game/quests.c).  A quest tracks its progress in ONE
 * status variable with thresholds declared per quest; the state itself is
 * mutated by the generic event system (quests are events, not engine code). */

typedef enum {
    QUEST_STATUS_NOT_STARTED = 0,
    QUEST_STATUS_ACTIVE = 1,
    QUEST_STATUS_COMPLETE = 2
} QuestStatus;

typedef struct {
    uint8_t id;                 /* stable quest id */
    const char *name;           /* display label, e.g. "MONSTER HUNT" */
    VariableId status_variable; /* quest status variable (see QuestStatus) */
    int16_t status_active;      /* var == this -> ACTIVE */
    int16_t status_complete;    /* var == this -> COMPLETE */
    VariableId progress_variable;  /* 0 = none; counter shown when ACTIVE */
    int16_t progress_target;
    const char *progress_label; /* e.g. "monsters" -> "monsters: X/3" */
    const char *complete_note;  /* reward shown as "complete - <note>" */
} QuestDefinition;

/* Register the game's quest table.  The engine is generic; the game layer
 * supplies its content table at boot. */
void quest_init(const QuestDefinition *table, uint8_t count);

/* Iterate the registered quest table. */
uint8_t quest_count(void);
const QuestDefinition *quest_at(uint8_t idx);

/* Derive a quest's current status from the canonical state. */
QuestStatus quest_status(const GameState *state, const QuestDefinition *q);

#endif /* QUEST_H */

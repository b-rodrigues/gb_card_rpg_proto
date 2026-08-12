#ifndef DIALOGUE_H
#define DIALOGUE_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_DIALOGUE_LINES 8

typedef enum {
    DIALOGUE_ID_NONE = 0,
    DIALOGUE_ID_MAYOR_GREETING = 1,
    DIALOGUE_ID_GUARD_GREETING = 2,
    DIALOGUE_ID_SHOPKEEPER_GREETING = 3,
    DIALOGUE_ID_MAYOR_INTRO = 4,
    DIALOGUE_ID_GUARD_AFTER_MAYOR = 5,
    DIALOGUE_ID_QUEST_ACTIVE = 6,
    DIALOGUE_ID_QUEST_COMPLETE = 7,
    DIALOGUE_ID_QUEST_DONE = 8,
    DIALOGUE_ID_MERCHANT_INTRO = 9,
    DIALOGUE_ID_MERCHANT_THANKS = 10,
    DIALOGUE_ID_AMULET_FOUND = 11,
    DIALOGUE_ID_AMULET_NOTHING = 12,
    DIALOGUE_ID_COUNT = 13
} DialogueId;

typedef struct {
    DialogueId id;
    const char *speaker;
    uint8_t line_count;
    const char *lines[MAX_DIALOGUE_LINES];
    uint8_t completion_flag;   /* FlagId set when this dialogue ends (0 = none) */
} DialogueDefinition;

typedef struct {
    bool active;
    DialogueId id;
    uint8_t current_line;
    uint8_t line_count;
    const char *speaker;
    const char *lines[MAX_DIALOGUE_LINES];
    uint8_t completion_flag;
} DialogueState;

void dialogue_init(DialogueState *d);
void dialogue_register(const DialogueDefinition *table, uint8_t count);
const DialogueDefinition *dialogue_get_def(DialogueId id);
void dialogue_start(DialogueState *d, DialogueId id, const char *speaker, const char **lines, uint8_t count);
void dialogue_start_def(DialogueState *d, DialogueId id);
bool dialogue_next(DialogueState *d);
DialogueId dialogue_end(DialogueState *d);

#endif /* DIALOGUE_H */

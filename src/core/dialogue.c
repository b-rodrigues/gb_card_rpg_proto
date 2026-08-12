#include "dialogue.h"
#include "telemetry.h"
#include <stddef.h>

static const DialogueDefinition g_dialogue_defs[] = {
    {
        DIALOGUE_ID_MAYOR_GREETING,
        "MAYOR:",
        2,
        {"Hello! I am Mayor.", "Welcome to town!", "", ""},
        0
    },
    {
        DIALOGUE_ID_GUARD_GREETING,
        "GUARD:",
        2,
        {"Halt! Keep peace.", "Watch for slimes.", "", ""},
        0
    },
    {
        DIALOGUE_ID_SHOPKEEPER_GREETING,
        "SHOP:",
        2,
        {"Welcome to my shop.", "Rest a while, friend.", "", ""},
        0
    },
    {
        DIALOGUE_ID_MAYOR_INTRO,
        "MAYOR:",
        3,
        {"I am the Mayor.", "Slimes menace the forest.", "Please help us!", ""},
        0
    },
    {
        DIALOGUE_ID_GUARD_AFTER_MAYOR,
        "GUARD:",
        2,
        {"The Mayor trusts you.", "Welcome, hero!", "", ""},
        0
    },
    {
        DIALOGUE_ID_QUEST_ACTIVE,
        "MAYOR:",
        2,
        {"Still monsters about.", "Defeat them all!", "", ""},
        0
    },
    {
        DIALOGUE_ID_QUEST_COMPLETE,
        "MAYOR:",
        3,
        {"You did it!", "Take this Sword!", "It cuts through slimes.", ""},
        0
    },
    {
        DIALOGUE_ID_QUEST_DONE,
        "MAYOR:",
        2,
        {"The Sword suits you.", "Go forth, hero!", "", ""},
        0
    }
};

#define NUM_DIALOGUE_DEFS (sizeof(g_dialogue_defs) / sizeof(g_dialogue_defs[0]))

const DialogueDefinition *dialogue_get_def(DialogueId id)
{
    uint8_t i;
    for (i = 0; i < NUM_DIALOGUE_DEFS; i++) {
        if (g_dialogue_defs[i].id == id) {
            return &g_dialogue_defs[i];
        }
    }
    return NULL;
}

void dialogue_init(DialogueState *d)
{
    if (!d) return;
    d->active = false;
    d->id = DIALOGUE_ID_NONE;
    d->current_line = 0;
    d->line_count = 0;
    d->speaker = "";
    d->completion_flag = 0;
}

void dialogue_start(DialogueState *d, DialogueId id, const char *speaker, const char **lines, uint8_t count)
{
    uint8_t i;
    if (!d) return;
    if (!lines) return;
    if (count > MAX_DIALOGUE_LINES) count = MAX_DIALOGUE_LINES;

    d->active = true;
    d->id = id;
    d->current_line = 0;
    d->line_count = count;
    d->speaker = speaker ? speaker : "";
    d->completion_flag = 0;

    for (i = 0; i < count; i++) {
        d->lines[i] = lines[i];
    }
    for (; i < MAX_DIALOGUE_LINES; i++) {
        d->lines[i] = "";
    }

    telemetry_emit(EVENT_DIALOGUE_STARTED, (uint8_t)d->id, 0, 0, 0);
}

void dialogue_start_def(DialogueState *d, DialogueId id)
{
    uint8_t i;
    const DialogueDefinition *def = dialogue_get_def(id);
    if (!d || !def) return;

    d->active = true;
    d->id = def->id;
    d->current_line = 0;
    d->line_count = (def->line_count > MAX_DIALOGUE_LINES) ? MAX_DIALOGUE_LINES : def->line_count;
    d->speaker = def->speaker ? def->speaker : "";
    d->completion_flag = def->completion_flag;

    for (i = 0; i < d->line_count; i++) {
        d->lines[i] = def->lines[i];
    }
    for (; i < MAX_DIALOGUE_LINES; i++) {
        d->lines[i] = "";
    }

    telemetry_emit(EVENT_DIALOGUE_STARTED, (uint8_t)d->id, 0, 0, 0);
}

bool dialogue_next(DialogueState *d)
{
    if (!d || !d->active) return false;
    d->current_line++;
    if (d->current_line >= d->line_count) {
        dialogue_end(d);
        return false;
    }
    telemetry_emit(EVENT_DIALOGUE_NEXT, (uint8_t)d->id, d->current_line, 0, 0);
    return true;
}

DialogueId dialogue_end(DialogueState *d)
{
    DialogueId old_id;
    if (!d || !d->active) return DIALOGUE_ID_NONE;
    old_id = d->id;
    d->active = false;
    d->id = DIALOGUE_ID_NONE;
    telemetry_emit(EVENT_DIALOGUE_ENDED, (uint8_t)old_id, 0, 0, 0);
    return old_id;
}

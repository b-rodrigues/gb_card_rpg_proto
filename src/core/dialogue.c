#include "dialogue.h"
#include "telemetry.h"
#include <stddef.h>

/* The dialogue table is game content, registered at boot via
 * dialogue_register().  The engine only matches and plays lines. */
static const DialogueDefinition *g_dialogues = NULL;
static uint8_t g_dialogue_count = 0;

void dialogue_register(const DialogueDefinition *table, uint8_t count)
{
    g_dialogues = table;
    g_dialogue_count = count;
}

const DialogueDefinition *dialogue_get_def(DialogueId id)
{
    uint8_t i;
    if (!g_dialogues) return NULL;
    for (i = 0; i < g_dialogue_count; i++) {
        if (g_dialogues[i].id == id) {
            return &g_dialogues[i];
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

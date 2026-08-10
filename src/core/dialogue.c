#include "dialogue.h"
#include "telemetry.h"
#include <stddef.h>

void dialogue_init(DialogueState *d)
{
    if (!d) return;
    d->active = false;
    d->id = DIALOGUE_ID_NONE;
    d->current_line = 0;
    d->line_count = 0;
    d->speaker = "";
}

void dialogue_start(DialogueState *d, DialogueId id, const char *speaker, const char **lines, uint8_t count)
{
    uint8_t i;
    if (!d) return;
    if (!lines) count = 0;
    if (count > MAX_DIALOGUE_LINES) count = MAX_DIALOGUE_LINES;

    d->active = true;
    d->id = id;
    d->current_line = 0;
    d->line_count = count;
    d->speaker = speaker ? speaker : "";

    for (i = 0; i < count; i++) {
        d->lines[i] = lines[i];
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

void dialogue_end(DialogueState *d)
{
    DialogueId old_id;
    if (!d || !d->active) return;
    old_id = d->id;
    d->active = false;
    d->id = DIALOGUE_ID_NONE;
    telemetry_emit(EVENT_DIALOGUE_ENDED, (uint8_t)old_id, 0, 0, 0);
}

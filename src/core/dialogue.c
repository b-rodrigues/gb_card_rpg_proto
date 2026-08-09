#include "dialogue.h"
#include "telemetry.h"

void dialogue_init(DialogueState *d)
{
    if (!d) return;
    d->active = false;
    d->current_line = 0;
    d->line_count = 0;
    d->speaker = "";
}

void dialogue_start(DialogueState *d, const char *speaker, const char **lines, uint8_t count)
{
    uint8_t i;
    if (!d) return;
    d->active = true;
    d->current_line = 0;
    d->line_count = count;
    d->speaker = speaker ? speaker : "";
    for (i = 0; i < count && i < MAX_DIALOGUE_LINES; i++) {
        d->lines[i] = lines[i];
    }
    telemetry_emit(EVENT_DIALOGUE_STARTED, 0, 0, 0, 0);
}

bool dialogue_next(DialogueState *d)
{
    if (!d || !d->active) return false;
    d->current_line++;
    if (d->current_line >= d->line_count) {
        dialogue_end(d);
        return false;
    }
    telemetry_emit(EVENT_DIALOGUE_NEXT, d->current_line, 0, 0, 0);
    return true;
}

void dialogue_end(DialogueState *d)
{
    if (!d || !d->active) return;
    d->active = false;
    telemetry_emit(EVENT_DIALOGUE_ENDED, 0, 0, 0, 0);
}

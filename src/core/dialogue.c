#include "dialogue.h"
#include "telemetry.h"
#include "banked.h"
#include <stddef.h>

/* The dialogue table is game content, registered at boot via
 * dialogue_register().  The engine only matches and plays lines.
 *
 * The table may live in a banked ROM region (see game_ids.h
 * GAME_CONTENT_BANK).  dialogue_start_def() copies the matching row and its
 * text into WRAM staging (g_dialogue_scratch / g_dlg_speaker / g_dlg_lines)
 * so DialogueState.speaker/.lines point at WRAM, never at banked ROM, and
 * the bank register is restored to 0 before returning.  A second
 * dialogue_start_def() while a dialogue is still active would overwrite the
 * staging; the screen model prevents this (only one dialogue is ever active,
 * started from the overworld screen and not restarted until it ends). */
static const DialogueDefinition *g_dialogues = NULL;
static uint8_t g_dialogue_count = 0;
static uint8_t g_dialogue_bank = 0;

static DialogueDefinition g_dialogue_scratch;
static char g_dlg_speaker[12];
static char g_dlg_lines[MAX_DIALOGUE_LINES][21];

/* banked_copy() takes a uint8_t byte count; a larger row cannot be staged. */
typedef char dialogue_def_fits_banked_copy[sizeof(DialogueDefinition) <= 255 ? 1 : -1];

void dialogue_register(const DialogueDefinition *table, uint8_t count, uint8_t bank)
{
    g_dialogues = table;
    g_dialogue_count = count;
    g_dialogue_bank = bank;
}

static const DialogueDefinition *dialogue_get_row(uint8_t i)
{
    if (g_dialogue_bank == 0) {
        return &g_dialogues[i];
    }
    banked_copy(g_dialogue_bank, &g_dialogue_scratch, &g_dialogues[i],
                sizeof(DialogueDefinition));
    return &g_dialogue_scratch;
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
    const DialogueDefinition *def = NULL;

    if (!d || !g_dialogues) return;

    for (i = 0; i < g_dialogue_count; i++) {
        const DialogueDefinition *row = dialogue_get_row(i);
        if (row->id == id) {
            def = row;
            break;
        }
    }
    if (!def) return;

    d->active = true;
    d->id = def->id;
    d->current_line = 0;
    d->line_count = (def->line_count > MAX_DIALOGUE_LINES) ? MAX_DIALOGUE_LINES : def->line_count;
    d->completion_flag = def->completion_flag;

    /* Stage speaker + line text into WRAM so d->speaker/.lines stay valid
     * after the ROM bank is restored.  Fixed-size copies + forced NUL keep
     * the strings terminated regardless of source length. */
    if (def->speaker) {
        banked_copy(g_dialogue_bank, g_dlg_speaker, def->speaker, 11);
        g_dlg_speaker[11] = 0;
        d->speaker = g_dlg_speaker;
    } else {
        d->speaker = "";
    }

    for (i = 0; i < d->line_count; i++) {
        if (def->lines[i]) {
            banked_copy(g_dialogue_bank, g_dlg_lines[i], def->lines[i], 20);
            g_dlg_lines[i][20] = 0;
            d->lines[i] = g_dlg_lines[i];
        } else {
            d->lines[i] = "";
        }
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

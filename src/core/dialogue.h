#ifndef DIALOGUE_H
#define DIALOGUE_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_DIALOGUE_LINES 4

typedef struct {
    bool active;
    uint8_t current_line;
    uint8_t line_count;
    const char *speaker;
    const char *lines[MAX_DIALOGUE_LINES];
} DialogueState;

void dialogue_init(DialogueState *d);
void dialogue_start(DialogueState *d, const char *speaker, const char **lines, uint8_t count);
bool dialogue_next(DialogueState *d);
void dialogue_end(DialogueState *d);

#endif /* DIALOGUE_H */

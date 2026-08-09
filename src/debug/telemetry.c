#include "telemetry.h"
#include <string.h>

#define MAX_EVENTS 32

static GameEvent event_buffer[MAX_EVENTS];
static uint8_t event_count;
static uint16_t event_seq;
static const uint32_t *telemetry_frame_ptr;

void telemetry_init(void)
{
    event_count = 0;
    event_seq = 0;
    telemetry_frame_ptr = NULL;
}

void telemetry_emit(GameEventType type, uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3)
{
    GameEvent *ev;
    uint8_t i;
    if (event_count >= MAX_EVENTS) {
        for (i = 0; i < MAX_EVENTS - 1; i++) {
            event_buffer[i] = event_buffer[i + 1];
        }
        event_count = MAX_EVENTS - 1;
    }
    
    ev = &event_buffer[event_count++];
    ev->seq = event_seq++;
    ev->frame = telemetry_frame_ptr ? *telemetry_frame_ptr : 0;
    ev->type = type;
    ev->data[0] = d0;
    ev->data[1] = d1;
    ev->data[2] = d2;
    ev->data[3] = d3;
}

const GameEvent* telemetry_get_events(void)
{
    return event_buffer;
}

uint8_t telemetry_get_count(void)
{
    return event_count;
}

void telemetry_set_frame_ptr(const uint32_t *frame_ptr)
{
    telemetry_frame_ptr = frame_ptr;
}

void debug_snapshot(uint8_t *buffer)
{
    /* fixed 256-byte buffer */
    if (!buffer) return;
    /* for now just clear it */
    memset(buffer, 0, 256);
}

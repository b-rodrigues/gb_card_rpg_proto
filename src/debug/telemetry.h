#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <stdint.h>

typedef enum {
    EVENT_PLAYER_MOVED,
    EVENT_COLLISION,
    EVENT_ENCOUNTER_STARTED,
    EVENT_BATTLE_STARTED,
    EVENT_BATTLE_ACTION,
    EVENT_DAMAGE_DEALT,
    EVENT_DAMAGE_RECEIVED,
    EVENT_ENTITY_DEFEATED,
    EVENT_BATTLE_WON,
    EVENT_BATTLE_LOST,
    EVENT_GAME_STATE_CHANGED,
    EVENT_MUSIC_CHANGED
} GameEventType;

typedef struct {
    uint16_t seq;
    uint32_t frame;
    GameEventType type;
    uint8_t data[4];
} GameEvent;

void telemetry_init(void);
void telemetry_emit(GameEventType type, uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3);
const GameEvent* telemetry_get_events(void);
uint8_t telemetry_get_count(void);
void telemetry_set_frame_ptr(const uint32_t *frame_ptr);
void debug_snapshot(uint8_t *buffer);

#endif /* TELEMETRY_H */

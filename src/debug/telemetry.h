#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <stdint.h>


#define MAX_TELEMETRY_EVENTS 32

/* Debug snapshot layout:
 *   bytes  0-19 : core game state (see telemetry.c debug_snapshot)
 *   bytes 20+   : scene actors as (id, x, y, facing) entries */
#define SNAPSHOT_BASE_SIZE      20
#define MAX_SNAPSHOT_ACTORS     4
#define SNAPSHOT_ACTOR_ENTRY_SIZE 4
#define SNAPSHOT_TOTAL_SIZE     (SNAPSHOT_BASE_SIZE + (MAX_SNAPSHOT_ACTORS * SNAPSHOT_ACTOR_ENTRY_SIZE))

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
    EVENT_MUSIC_CHANGED,
    EVENT_MAP_CHANGED,
    EVENT_STORY_FLAG_SET,
    EVENT_STORY_FLAG_CLEARED,
    EVENT_DIALOGUE_STARTED,
    EVENT_DIALOGUE_NEXT,
    EVENT_DIALOGUE_ENDED,
    EVENT_INTERACTION_ATTEMPT,
    EVENT_RENDER_SCREEN,
    EVENT_RENDER_DIALOGUE,
    EVENT_SCREEN_CHANGED,
    EVENT_SCENE_CHANGED,
    EVENT_ACTOR_COLLISION,
    EVENT_ACTOR_INTERACTION,
    EVENT_ACTOR_COMBAT_START
} GameEventType;

typedef struct {
    uint32_t seq;
    uint32_t frame;
    uint8_t type;
    uint8_t data[4];
} GameEvent;

extern uint8_t g_snap_buf[SNAPSHOT_TOTAL_SIZE];
extern GameEvent g_telemetry_buffer[MAX_TELEMETRY_EVENTS];
extern uint8_t g_telemetry_count;
extern uint8_t g_telemetry_head;

void telemetry_init(void);
void telemetry_emit(GameEventType type, uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3);
const GameEvent* telemetry_get_events(void);
uint8_t telemetry_get_count(void);
void telemetry_set_frame_ptr(const uint32_t *frame_ptr);
void debug_snapshot(void);

#endif /* TELEMETRY_H */

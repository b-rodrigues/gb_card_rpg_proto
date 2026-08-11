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

/* Extended RPG state snapshot (g_state_snap_buf) layout:
 *   byte  0            : version/validity (0x01)
 *   bytes 1..8         : FlagState.bytes[0..7]
 *   bytes 9..24        : variables 1..8 as int16 LE
 *   byte  25           : party count
 *   bytes 26..49       : up to 4 party members x {id, level, xp_lo, xp_hi, hp, max_hp}
 *   byte  50           : inventory count
 *   bytes 51..66       : up to 8 inventory entries x {item_id, quantity}
 *   byte  67           : world (persistent actor) count
 *   bytes 68..91       : up to 8 world entries x {actor_id_lo, actor_id_hi, state}
 */
#define STATE_SNAP_VERSION_BYTE    0x01
#define STATE_SNAP_FLAGS_OFFSET    1
#define STATE_SNAP_FLAGS_SIZE      8
#define STATE_SNAP_VARIABLES_OFFSET  9
#define STATE_SNAP_VARIABLES_SIZE    16
#define STATE_SNAP_PARTY_OFFSET      25
#define STATE_SNAP_PARTY_ENTRY_SIZE  6
#define STATE_SNAP_INVENTORY_OFFSET  50
#define STATE_SNAP_INVENTORY_ENTRY_SIZE 2
#define STATE_SNAP_WORLD_OFFSET      67
#define STATE_SNAP_WORLD_ENTRY_SIZE  3
#define STATE_SNAP_TOTAL_SIZE        92

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
    EVENT_ACTOR_COMBAT_START,
    EVENT_VARIABLE_SET,
    EVENT_ITEM_ADDED,
    EVENT_ITEM_REMOVED,
    EVENT_ACTOR_STATE_CHANGE
} GameEventType;

typedef struct {
    uint32_t seq;
    uint32_t frame;
    uint8_t type;
    uint8_t data[4];
} GameEvent;

extern uint8_t g_snap_buf[SNAPSHOT_TOTAL_SIZE];
extern uint8_t g_state_snap_buf[STATE_SNAP_TOTAL_SIZE];
extern GameEvent g_telemetry_buffer[MAX_TELEMETRY_EVENTS];
extern uint8_t g_telemetry_count;
extern uint8_t g_telemetry_head;

void telemetry_init(void);
void telemetry_emit(GameEventType type, uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3);
const GameEvent* telemetry_get_events(void);
uint8_t telemetry_get_count(void);
void telemetry_set_frame_ptr(const uint32_t *frame_ptr);
void debug_snapshot(void);
void debug_state_snapshot(void);

#endif /* TELEMETRY_H */

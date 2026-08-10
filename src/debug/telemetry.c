#include "telemetry.h"
#include "audio.h"
#include <string.h>

uint8_t g_snap_buf[16] = {0};
GameEvent g_telemetry_buffer[MAX_TELEMETRY_EVENTS] = {{0}};
uint8_t g_telemetry_count = 0;
uint8_t g_telemetry_head = 0;
static uint32_t event_seq = 0;
static const uint32_t *telemetry_frame_ptr = NULL;

void telemetry_init(void)
{
    g_telemetry_count = 0;
    g_telemetry_head = 0;
    event_seq = 0;
    telemetry_frame_ptr = NULL;
    memset(g_snap_buf, 0, sizeof(g_snap_buf));
    memset(g_telemetry_buffer, 0, sizeof(g_telemetry_buffer));
}

void telemetry_emit(GameEventType type, uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3)
{
    GameEvent *ev = &g_telemetry_buffer[g_telemetry_head];
    ev->seq = event_seq++;
    ev->frame = telemetry_frame_ptr ? *telemetry_frame_ptr : 0;
    ev->type = (uint8_t)type;
    ev->data[0] = d0;
    ev->data[1] = d1;
    ev->data[2] = d2;
    ev->data[3] = d3;

    g_telemetry_head = (g_telemetry_head + 1) % MAX_TELEMETRY_EVENTS;
    if (g_telemetry_count < MAX_TELEMETRY_EVENTS) {
        g_telemetry_count++;
    }
}

const GameEvent* telemetry_get_events(void)
{
    return g_telemetry_buffer;
}

uint8_t telemetry_get_count(void)
{
    return g_telemetry_count;
}

void telemetry_set_frame_ptr(const uint32_t *frame_ptr)
{
    telemetry_frame_ptr = frame_ptr;
}

#include "game.h"
extern Game g_game;

void debug_snapshot(void)
{
    const Game *g = &g_game;

    g_snap_buf[0] = (uint8_t)g->state_machine.current;
    g_snap_buf[1] = g->world.player.position.x;
    g_snap_buf[2] = g->world.player.position.y;
    g_snap_buf[3] = g->world.player.hp;
    g_snap_buf[4] = g->world.enemy.hp;
    g_snap_buf[5] = g->world.enemy.active ? 1 : 0;
    g_snap_buf[6] = (uint8_t)audio_get_current_track();
    g_snap_buf[7] = (uint8_t)g->battle.turn;
    g_snap_buf[8] = (uint8_t)g->battle.result;
    g_snap_buf[9] = g->battle.player.hp;
    g_snap_buf[10] = g->battle.enemy.hp;
    g_snap_buf[11] = (uint8_t)g->world.map_id;
    g_snap_buf[12] = (uint8_t)g->story_flags; /* Low 8 bits of story_flags bitfield */
    g_snap_buf[13] = g->dialogue.active ? 1 : 0;
    g_snap_buf[14] = g->dialogue.current_line;
    g_snap_buf[15] = (uint8_t)g->dialogue.id;
}

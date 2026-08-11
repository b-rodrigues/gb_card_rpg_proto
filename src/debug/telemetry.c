#include "telemetry.h"
#include "audio.h"
#include "screen.h"
#include "actor.h"

uint8_t g_snap_buf[SNAPSHOT_TOTAL_SIZE] = {0};
GameEvent g_telemetry_buffer[MAX_TELEMETRY_EVENTS] = {{0}};
uint8_t g_telemetry_count = 0;
uint8_t g_telemetry_head = 0;
static uint32_t event_seq = 0;
static const uint32_t *telemetry_frame_ptr = NULL;

void telemetry_init(void)
{
    uint8_t i;
    uint8_t *b = g_snap_buf;
    GameEvent *ev;

    g_telemetry_count = 0;
    g_telemetry_head = 0;
    event_seq = 0;
    telemetry_frame_ptr = NULL;

    /* Manual zero-fill instead of memset(): memset lives in a banked ROM
     * region and banked calls from this debug init path hang under the mGBA
     * debugger. */
    for (i = 0; i < sizeof(g_snap_buf); i++) {
        b[i] = 0;
    }
    for (i = 0; i < MAX_TELEMETRY_EVENTS; i++) {
        ev = &g_telemetry_buffer[i];
        ev->seq = 0;
        ev->frame = 0;
        ev->type = 0;
        ev->data[0] = 0;
        ev->data[1] = 0;
        ev->data[2] = 0;
        ev->data[3] = 0;
    }
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

/* Broad screen value for backward-compatible snapshot byte 0.
 * Dialogue is a sub-screen of overworld in the legacy game_state encoding. */
static uint8_t screen_broad(ScreenId s)
{
    switch (s) {
        case SCREEN_BATTLE:    return 1;
        case SCREEN_GAME_OVER: return 2;
        case SCREEN_THANKS:    return 3;
        default:               return 0; /* OVERWORLD or DIALOGUE */
    }
}

void debug_snapshot(void)
{
    const Game *g = &g_game;
    uint8_t actor_buf[MAX_SNAPSHOT_ACTORS * ACTOR_SNAPSHOT_ENTRY_SIZE];
    uint8_t actor_count;
    uint8_t i;

    g_snap_buf[0] = screen_broad(g->screen);
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
    g_snap_buf[12] = (uint8_t)g->story_flags;
    g_snap_buf[13] = g->dialogue.active ? 1 : 0;
    g_snap_buf[14] = g->dialogue.current_line;
    g_snap_buf[15] = (uint8_t)g->dialogue.id;
    g_snap_buf[16] = (uint8_t)g->world.player.facing;
    g_snap_buf[17] = g->game_over_choice;
    g_snap_buf[18] = (uint8_t)g->screen;
    g_snap_buf[19] = (uint8_t)g->scene;

    actor_count = actor_write_snapshot(&g->world, actor_buf, MAX_SNAPSHOT_ACTORS);
    for (i = 0; i < (MAX_SNAPSHOT_ACTORS * ACTOR_SNAPSHOT_ENTRY_SIZE); i++) {
        g_snap_buf[SNAPSHOT_BASE_SIZE + i] =
            (i < (actor_count * ACTOR_SNAPSHOT_ENTRY_SIZE)) ? actor_buf[i] : 0;
    }
}

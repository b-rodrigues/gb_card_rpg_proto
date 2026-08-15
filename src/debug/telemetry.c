#include "telemetry.h"
#include "audio.h"
#include "screen.h"
#include "actor.h"

uint8_t g_snap_buf[SNAPSHOT_TOTAL_SIZE] = {0};
uint8_t g_state_snap_buf[STATE_SNAP_TOTAL_SIZE] = {0};
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
    b = g_state_snap_buf;
    for (i = 0; i < sizeof(g_state_snap_buf); i++) {
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
    uint8_t first_hp = 0;
    uint8_t first_active = 0;
    uint8_t i;

    for (i = 0; i < MAX_WORLD_ACTORS; i++) {
        if (g->world.actors[i].active) {
            first_hp = g->world.actors[i].hp;
            first_active = 1;
            break;
        }
    }

    g_snap_buf[0] = screen_broad(g->screen);
    g_snap_buf[1] = g->world.player.position.x;
    g_snap_buf[2] = g->world.player.position.y;
    g_snap_buf[3] = g->world.player.hp;
    g_snap_buf[4] = first_hp;
    g_snap_buf[5] = first_active;
    g_snap_buf[6] = (uint8_t)audio_get_current_track();
    g_snap_buf[7] = (uint8_t)g->battle.turn;
    g_snap_buf[8] = (uint8_t)g->battle.result;
    g_snap_buf[9] = g->battle.player.hp;
    g_snap_buf[10] = g->battle.enemy.hp;
    g_snap_buf[11] = (uint8_t)g->world.map_id;
    g_snap_buf[12] = g->state.flags.bytes[0];
    g_snap_buf[13] = g->dialogue.active ? 1 : 0;
    g_snap_buf[14] = g->dialogue.current_line;
    g_snap_buf[15] = (uint8_t)g->dialogue.id;
    g_snap_buf[16] = (uint8_t)g->world.player.facing;
    g_snap_buf[17] = g->game_over_choice;
    g_snap_buf[18] = (uint8_t)g->screen;
    g_snap_buf[19] = (uint8_t)g->state.scene.scene_id;

    actor_count = actor_write_snapshot(&g->world, actor_buf, MAX_SNAPSHOT_ACTORS);
    for (i = 0; i < (MAX_SNAPSHOT_ACTORS * ACTOR_SNAPSHOT_ENTRY_SIZE); i++) {
        g_snap_buf[SNAPSHOT_BASE_SIZE + i] =
            (i < (actor_count * ACTOR_SNAPSHOT_ENTRY_SIZE)) ? actor_buf[i] : 0;
    }

    debug_state_snapshot();
}

/* Serialize the canonical GameState into g_state_snap_buf for the host.
 * Fixed offsets documented in telemetry.h. */
void debug_state_snapshot(void)
{
    const GameState *st;
    uint8_t i;
    uint8_t n;
    uint8_t *b = g_state_snap_buf;

    if (!&g_game) return;
    st = &g_game.state;

    b[0] = STATE_SNAP_VERSION_BYTE;
    for (i = 0; i < MAX_STATE_FLAGS / 8; i++) {
        b[STATE_SNAP_FLAGS_OFFSET + i] = st->flags.bytes[i];
    }
    for (i = 0; i < STATE_SNAP_VARIABLES_SIZE / 2; i++) {
        b[STATE_SNAP_VARIABLES_OFFSET + i * 2]     = (uint8_t)(st->variables.values[i] & 0xFF);
        b[STATE_SNAP_VARIABLES_OFFSET + i * 2 + 1] = (uint8_t)((st->variables.values[i] >> 8) & 0xFF);
    }

    /* Currency: dense slots; report every slot (id = index + 1). */
    b[STATE_SNAP_CURRENCY_COUNT_OFF] = MAX_CURRENCIES;
    for (i = 0; i < MAX_CURRENCIES; i++) {
        n = STATE_SNAP_CURRENCY_ENTRY_OFF + i * STATE_SNAP_CURRENCY_ENTRY_SIZE;
        b[n]     = (uint8_t)(i + 1);
        b[n + 1] = (uint8_t)(st->currency.amount[i] & 0xFF);
        b[n + 2] = (uint8_t)((st->currency.amount[i] >> 8) & 0xFF);
    }

    b[STATE_SNAP_PARTY_OFFSET] = st->party.count;
    for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
        n = STATE_SNAP_PARTY_OFFSET + 1 + i * STATE_SNAP_PARTY_ENTRY_SIZE;
        if (i < st->party.count) {
            b[n]     = (uint8_t)st->party.members[i].id;
            b[n + 1] = st->party.members[i].hp;
            b[n + 2] = st->party.members[i].max_hp;
        } else {
            b[n] = 0; b[n + 1] = 0; b[n + 2] = 0;
        }
    }

    b[STATE_SNAP_INVENTORY_OFFSET] = st->inventory.count;
    for (i = 0; i < 16; i++) {
        n = STATE_SNAP_INVENTORY_OFFSET + 1 + i * STATE_SNAP_INVENTORY_ENTRY_SIZE;
        if (i < st->inventory.count) {
            b[n]     = (uint8_t)st->inventory.entries[i].item_id;
            b[n + 1] = st->inventory.entries[i].quantity;
        } else {
            b[n] = 0; b[n + 1] = 0;
        }
    }

    b[STATE_SNAP_WORLD_OFFSET] = st->world.count;
    for (i = 0; i < 16; i++) {
        n = STATE_SNAP_WORLD_OFFSET + 1 + i * STATE_SNAP_WORLD_ENTRY_SIZE;
        if (i < st->world.count) {
            b[n]     = (uint8_t)(st->world.actors[i].actor_id & 0xFF);
            b[n + 1] = (uint8_t)((st->world.actors[i].actor_id >> 8) & 0xFF);
            b[n + 2] = st->world.actors[i].state;
        } else {
            b[n] = 0; b[n + 1] = 0; b[n + 2] = 0;
        }
    }

    b[STATE_SNAP_PROGRESSION_COUNT_OFF] = st->progression.count;
    for (i = 0; i < MAX_PROGRESSION_TARGETS; i++) {
        n = STATE_SNAP_PROGRESSION_ENTRY_OFF + i * STATE_SNAP_PROGRESSION_ENTRY_SIZE;
        if (i < st->progression.count) {
            b[n]     = st->progression.entries[i].target.type;
            b[n + 1] = (uint8_t)(st->progression.entries[i].target.id & 0xFF);
            b[n + 2] = (uint8_t)((st->progression.entries[i].target.id >> 8) & 0xFF);
            b[n + 3] = st->progression.entries[i].state.level;
            b[n + 4] = (uint8_t)(st->progression.entries[i].state.progress & 0xFF);
            b[n + 5] = (uint8_t)((st->progression.entries[i].state.progress >> 8) & 0xFF);
        } else {
            b[n] = 0; b[n + 1] = 0; b[n + 2] = 0; b[n + 3] = 0; b[n + 4] = 0; b[n + 5] = 0;
        }
    }

    b[STATE_SNAP_EQUIPMENT_OFF] = (uint8_t)st->equipment.weapon;

    /* Runtime overworld camera + scene dims (not part of the saveable
     * GameState; the host asserts scroll/camera for the camera/scroll
     * milestone and the SCX/SCY-render alignment). */
    b[STATE_SNAP_SCROLL_X_OFF]        = g_game.world.scroll_x;
    b[STATE_SNAP_SCROLL_Y_OFF]        = g_game.world.scroll_y;
    b[STATE_SNAP_WORLD_WIDTH_OFF]     = g_game.world.width;
    b[STATE_SNAP_WORLD_HEIGHT_OFF]    = g_game.world.height;
    b[STATE_SNAP_CAMERA_PX_X_OFF]     = g_game.world.camera_px_x;
    b[STATE_SNAP_CAMERA_PX_Y_OFF]     = g_game.world.camera_px_y;
}

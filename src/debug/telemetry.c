#include "telemetry.h"
#include "audio.h"
#include "screen.h"
#include "actor.h"

/* These live in WRAM (BSS): telemetry_init() zero-fills every one of them at
 * boot, so C `= {0}` initializers would only add dead __xinit ROM copies. */
uint8_t g_snap_buf[SNAPSHOT_TOTAL_SIZE];
uint8_t g_state_snap_buf[STATE_SNAP_TOTAL_SIZE];
GameEvent g_telemetry_buffer[MAX_TELEMETRY_EVENTS];
uint8_t g_telemetry_count;
uint8_t g_telemetry_head;
static uint32_t event_seq;
static const uint32_t *telemetry_frame_ptr;

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
    const WorldActorRuntime *ar;
    const WorldActorDefinition *defs;
    uint8_t actor_count, def_count, i, slot, n = 0;
    uint8_t first_hp = 0;
    uint8_t first_active = 0;
    uint8_t *p;

    ar = g->world.actors;
    for (i = 0; i < MAX_WORLD_ACTORS; i++, ar++) {
        if (ar->active) {
            first_hp = ar->hp;
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

    /* Scene actors, written straight into the snapshot (inlined from
     * actor_write_snapshot, its only caller).  Zero-fill the tail so a
     * later snapshot with fewer actors does not leave stale entries. */
    p = &g_snap_buf[SNAPSHOT_BASE_SIZE];
    actor_count = MAX_SNAPSHOT_ACTORS;
    for (slot = 0; slot < MAX_WORLD_ACTORS && n < MAX_SNAPSHOT_ACTORS; slot++) {
        if (g->world.actors[slot].active) {
            p[0] = (uint8_t)g->world.actors[slot].id;
            p[1] = g->world.actors[slot].x;
            p[2] = g->world.actors[slot].y;
            p[3] = g->world.actors[slot].facing;
            p += SNAPSHOT_ACTOR_ENTRY_SIZE;
            n++;
        }
    }
    defs = actor_defs_for_map(g->world.map_id, &def_count);
    for (i = 0; i < def_count && n < MAX_SNAPSHOT_ACTORS; i++) {
        if (defs[i].flags & ACTOR_FLAG_HOSTILE) continue;
        p[0] = (uint8_t)defs[i].id;
        p[1] = defs[i].x;
        p[2] = defs[i].y;
        p[3] = defs[i].facing;
        p += SNAPSHOT_ACTOR_ENTRY_SIZE;
        n++;
    }
    while (n < actor_count) {
        p[0] = 0;
        p[1] = 0;
        p[2] = 0;
        p[3] = 0;
        p += SNAPSHOT_ACTOR_ENTRY_SIZE;
        n++;
    }

    debug_state_snapshot();
}

/* Copy a counted, fixed-entry-size array section into the snapshot.
 * Walks src/dst pointers (no index*size multiply) and zero-fills past the
 * source count.  Source and snapshot entry layouts are byte-identical
 * (int16 values are already little-endian in memory). */
static void snap_section(uint8_t *b, uint8_t count_off, uint8_t entry_off,
                         uint8_t count, const uint8_t *src,
                         uint8_t entry_size, uint8_t max_entries)
{
    uint8_t i, k;
    uint8_t *p = &b[entry_off];

    b[count_off] = count;
    for (i = 0; i < max_entries; i++) {
        for (k = 0; k < entry_size; k++) {
            p[k] = (i < count) ? src[k] : 0;
        }
        src += entry_size;
        p += entry_size;
    }
}

/* Serialize the canonical GameState into g_state_snap_buf for the host.
 * Fixed offsets documented in telemetry.h.  Uses running pointers rather
 * than b[OFFSET + i*SIZE] index arithmetic (SDCC emits tighter code for a
 * walking pointer); the output layout is unchanged. */
void debug_state_snapshot(void)
{
    const GameState *st;
    const uint8_t *s;
    uint8_t i;
    uint8_t *b = g_state_snap_buf;
    uint8_t *p;

    if (!&g_game) return;
    st = &g_game.state;

    b[0] = STATE_SNAP_VERSION_BYTE;
    for (i = 0; i < MAX_STATE_FLAGS / 8; i++) {
        b[STATE_SNAP_FLAGS_OFFSET + i] = st->flags.bytes[i];
    }
    p = &b[STATE_SNAP_VARIABLES_OFFSET];
    s = (const uint8_t *)st->variables.values;
    for (i = 0; i < STATE_SNAP_VARIABLES_SIZE; i++) {
        p[i] = s[i];
    }
    /* Currency: dense slots; report every slot (id = index + 1). */
    b[STATE_SNAP_CURRENCY_COUNT_OFF] = MAX_CURRENCIES;
    p = &b[STATE_SNAP_CURRENCY_ENTRY_OFF];
    s = (const uint8_t *)st->currency.amount;
    for (i = 0; i < MAX_CURRENCIES; i++) {
        p[0] = (uint8_t)(i + 1);
        p[1] = s[0];
        p[2] = s[1];
        s += 2;
        p += STATE_SNAP_CURRENCY_ENTRY_SIZE;
    }

    snap_section(b, STATE_SNAP_PARTY_OFFSET, STATE_SNAP_PARTY_OFFSET + 1,
                 st->party.count, (const uint8_t *)&st->party.members[0],
                 STATE_SNAP_PARTY_ENTRY_SIZE, MAX_PARTY_MEMBERS);
    snap_section(b, STATE_SNAP_INVENTORY_OFFSET, STATE_SNAP_INVENTORY_OFFSET + 1,
                 st->inventory.count, (const uint8_t *)&st->inventory.entries[0],
                 STATE_SNAP_INVENTORY_ENTRY_SIZE, MAX_INVENTORY_ITEMS);
    snap_section(b, STATE_SNAP_WORLD_OFFSET, STATE_SNAP_WORLD_OFFSET + 1,
                 st->world.count, (const uint8_t *)&st->world.actors[0],
                 STATE_SNAP_WORLD_ENTRY_SIZE, MAX_PERSISTENT_ACTORS);
    snap_section(b, STATE_SNAP_PROGRESSION_COUNT_OFF,
                 STATE_SNAP_PROGRESSION_ENTRY_OFF,
                 st->progression.count,
                 (const uint8_t *)&st->progression.entries[0],
                 STATE_SNAP_PROGRESSION_ENTRY_SIZE, MAX_PROGRESSION_TARGETS);

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

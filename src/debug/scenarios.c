#include "scenarios.h"
#include "rng.h"
#include "telemetry.h"
#include "game.h"
#include "screen.h"
#include "scene.h"
#include "story.h"
#include "dialogue.h"
#include "world.h"
#include "entity.h"
#include "actor.h"
#include "battle.h"
#include "ui.h"
#include "input.h"
#include "rpg/inventory.h"
#include "rpg/currency.h"
#include "rpg/progression.h"
#include "rpg/items.h"

extern Game g_game;

/* Legacy scenario-id slot, kept reserved.  All scenarios now load through
 * the declarative initial-state descriptor (g_scen_state_buf). */
volatile uint8_t g_scen_load = 0;
volatile uint8_t g_scen_load_state = 0;
uint8_t g_scen_state_buf[STATE_LOAD_DESC_SIZE];

/* Debug-action channel: the host writes a command + args here and sets
 * g_debug_action_pending to exercise a real mechanic deterministically
 * without the UI (add/remove item, currency, progress, buy, use). */
volatile uint8_t g_debug_action[6];
volatile uint8_t g_debug_action_pending;

enum {
    DBG_ACT_NONE = 0,
    DBG_ACT_ADD_ITEM = 1,
    DBG_ACT_REMOVE_ITEM = 2,
    DBG_ACT_ADD_CURRENCY = 3,
    DBG_ACT_ADD_PROGRESS = 4,
    DBG_ACT_BUY_ITEM = 5,
    DBG_ACT_USE_ITEM = 6,
    DBG_ACT_EQUIP_ITEM = 7
};

/* Shared post-setup: reset frame/flags/input/telemetry/audio. */
static void scenario_begin(uint32_t seed)
{
    uint8_t i;
    g_game.frame = 0;
    g_game.game_over_choice = 0;
    for (i = 0; i < (MAX_STATE_FLAGS / 8); i++) {
        g_game.state.flags.bytes[i] = 0;
    }
    rng_set_seed(seed);
    input_reset();
    telemetry_init();
    telemetry_set_frame_ptr(&g_game.frame);
    audio_play_music(MUSIC_OVERWORLD);
}

/* General declarative scenario loader.  Reads the initial-state descriptor
 * written by the host STATE_LOAD command (g_scen_state_buf), constructs the
 * canonical GameState and world, then starts the game in the requested
 * screen.  Setup must never emit gameplay telemetry (AGENTS.md): all state
 * is written directly into GameState, never through game_flag_set & co. */
static void scenario_load_state(void)
{
    const uint8_t *b = g_scen_state_buf;
    SceneId scene;
    MapId map;
    uint8_t x, y, facing;
    uint32_t seed;
    uint8_t screen;
    uint8_t dialogue_id;
    uint8_t start_battle;
    uint8_t i;
    uint8_t n;

    if (b[0] != STATE_LOAD_DESC_VERSION) return;

    screen = b[STATE_LOAD_DESC_SCREEN_OFF];
    scene = (SceneId)b[STATE_LOAD_DESC_SCENE_OFF];
    x = b[STATE_LOAD_DESC_PLAYER_X_OFF];
    y = b[STATE_LOAD_DESC_PLAYER_Y_OFF];
    facing = b[STATE_LOAD_DESC_PLAYER_FACING_OFF];
    seed = (uint32_t)b[STATE_LOAD_DESC_SEED_OFF]
         | ((uint32_t)b[STATE_LOAD_DESC_SEED_OFF + 1] << 8)
         | ((uint32_t)b[STATE_LOAD_DESC_SEED_OFF + 2] << 16)
         | ((uint32_t)b[STATE_LOAD_DESC_SEED_OFF + 3] << 24);
    dialogue_id = b[STATE_LOAD_DESC_DIALOGUE_ID_OFF];
    start_battle = b[STATE_LOAD_DESC_START_BATTLE_OFF];
    map = scene_id_to_map(scene);

    /* Canonical persistent state: default, then descriptor overrides. */
    game_state_init(&g_game.state);
    for (i = 0; i < STATE_LOAD_DESC_FLAGS_SIZE; i++) {
        g_game.state.flags.bytes[i] = b[STATE_LOAD_DESC_FLAGS_OFF + i];
    }
    for (i = 0; i < b[STATE_LOAD_DESC_VARIABLES_COUNT_OFF]; i++) {
        uint8_t vid;
        int16_t val;
        n = STATE_LOAD_DESC_VARIABLES_ENTRY_OFF + i * STATE_LOAD_DESC_VARIABLES_ENTRY_SIZE;
        vid = b[n];
        val = (int16_t)((int16_t)b[n + 1] | ((int16_t)b[n + 2] << 8));
        if (vid >= 1 && vid <= MAX_STATE_VARIABLES) {
            g_game.state.variables.values[vid - 1] = val;
        }
    }
    for (i = 0; i < b[STATE_LOAD_DESC_CURRENCY_COUNT_OFF]; i++) {
        uint8_t cid;
        int16_t amt;
        n = STATE_LOAD_DESC_CURRENCY_ENTRY_OFF + i * STATE_LOAD_DESC_CURRENCY_ENTRY_SIZE;
        cid = b[n];
        amt = (int16_t)((int16_t)b[n + 1] | ((int16_t)b[n + 2] << 8));
        if (cid >= 1 && cid <= MAX_CURRENCIES) {
            g_game.state.currency.amount[cid - 1] = amt;
        }
    }
    for (i = 0; i < b[STATE_LOAD_DESC_PARTY_COUNT_OFF]; i++) {
        n = STATE_LOAD_DESC_PARTY_ENTRY_OFF + i * STATE_LOAD_DESC_PARTY_ENTRY_SIZE;
        g_game.state.party.members[i].id = (CharacterId)b[n];
        g_game.state.party.members[i].hp = b[n + 1];
        g_game.state.party.members[i].max_hp = b[n + 2];
        g_game.state.party.count = (uint8_t)(i + 1);
    }
    for (i = 0; i < b[STATE_LOAD_DESC_INVENTORY_COUNT_OFF]; i++) {
        n = STATE_LOAD_DESC_INVENTORY_ENTRY_OFF + i * STATE_LOAD_DESC_INVENTORY_ENTRY_SIZE;
        g_game.state.inventory.entries[i].item_id = (ItemId)b[n];
        g_game.state.inventory.entries[i].quantity = b[n + 1];
        g_game.state.inventory.count = (uint8_t)(i + 1);
    }
    for (i = 0; i < b[STATE_LOAD_DESC_WORLD_COUNT_OFF]; i++) {
        n = STATE_LOAD_DESC_WORLD_ENTRY_OFF + i * STATE_LOAD_DESC_WORLD_ENTRY_SIZE;
        g_game.state.world.actors[i].actor_id = (ActorId)(b[n] | (b[n + 1] << 8));
        g_game.state.world.actors[i].state = b[n + 2];
        g_game.state.world.count = (uint8_t)(i + 1);
    }
    for (i = 0; i < b[STATE_LOAD_DESC_PROGRESSION_COUNT_OFF]; i++) {
        ProgressionTarget t;
        n = STATE_LOAD_DESC_PROGRESSION_ENTRY_OFF + i * STATE_LOAD_DESC_PROGRESSION_ENTRY_SIZE;
        t.type = b[n];
        t.id = (uint16_t)(b[n + 1] | (b[n + 2] << 8));
        progression_ensure(&g_game.state, t, b[n + 3],
                           (uint16_t)(b[n + 4] | (b[n + 5] << 8)));
    }
    g_game.state.equipment.weapon = (ItemId)b[STATE_LOAD_DESC_EQUIPMENT_OFF];

    /* Scene + world.  Persistent defeats are in state before the world is
     * (re)loaded, so actor_load_scene() skips defeated actors. */
    g_game.state.scene.scene_id = scene;
    g_game.state.scene.player_x = x;
    g_game.state.scene.player_y = y;
    g_game.state.scene.player_facing = facing;
    world_init(&g_game.world, &g_game.state);
    world_load_map(&g_game.world, map, &g_game.state);
    g_game.world.player.position.x = x;
    g_game.world.player.position.y = y;
    g_game.world.player.facing = (Direction)facing;
    g_game.world.encounter_actor_index = NO_ACTOR_INDEX;
    dialogue_init(&g_game.dialogue);

    scenario_begin(seed);

    /* scenario_begin() cleared the flags; re-apply directly (no telemetry). */
    for (i = 0; i < STATE_LOAD_DESC_FLAGS_SIZE; i++) {
        g_game.state.flags.bytes[i] = b[STATE_LOAD_DESC_FLAGS_OFF + i];
    }
    g_game.game_over_choice = b[STATE_LOAD_DESC_GAME_OVER_CHOICE_OFF];
    g_game.prev_screen = SCREEN_OVERWORLD;
    g_game.screen = SCREEN_OVERWORLD;

    if (b[STATE_LOAD_DESC_FONT_TEST_OFF]) {
        ui_draw_font_test();
    }
    if (dialogue_id != DIALOGUE_ID_NONE) {
        dialogue_start_def(&g_game.dialogue, (DialogueId)dialogue_id);
        g_game.screen = SCREEN_DIALOGUE;
    }
    if (start_battle) {
        uint8_t idx = 0;
        for (i = 0; i < MAX_WORLD_ACTORS; i++) {
            if (g_game.world.actors[i].active) {
                idx = i;
                break;
            }
        }
        battle_start(&g_game.battle, actor_enemy_name(g_game.world.actors[idx].id),
                     g_game.state.party.members[0].hp,
                     g_game.state.party.members[0].max_hp,
                     game_hero_attack(&g_game.state),
                     g_game.world.actors[idx].hp,
                     g_game.world.actors[idx].max_hp);
        g_game.screen = SCREEN_BATTLE;
        audio_play_music(MUSIC_BATTLE);
    }
    if (screen == SCREEN_GAME_OVER) {
        g_game.screen = SCREEN_GAME_OVER;
    }
    if (screen == SCREEN_THANKS) {
        g_game.screen = SCREEN_THANKS;
    }
    if (screen == SCREEN_ENDING) {
        g_game.screen = SCREEN_ENDING;
    }

    game_render_reset(&g_game);
    debug_snapshot();
}

/* Run a host-issued debug action through the real mechanic functions.
 * Unlike scenario setup, these ARE gameplay actions: they emit telemetry. */
static void debug_run_action(void)
{
    uint8_t action = g_debug_action[0];
    uint8_t a0 = g_debug_action[1];
    int16_t a1 = (int16_t)((uint16_t)g_debug_action[2]
                          | ((uint16_t)g_debug_action[3] << 8));
    uint8_t a2 = g_debug_action[4];
    ProgressionTarget target;
    ProgressionAddResult pres;

    switch (action) {
        case DBG_ACT_ADD_ITEM:
            inventory_add(&g_game.state.inventory, (ItemId)a0, a2);
            break;
        case DBG_ACT_REMOVE_ITEM:
            inventory_remove(&g_game.state.inventory, (ItemId)a0, a2);
            break;
        case DBG_ACT_ADD_CURRENCY:
            currency_add(&g_game.state, (CurrencyId)a0, a1);
            break;
        case DBG_ACT_ADD_PROGRESS:
            target.type = a0;
            target.id = (uint16_t)a1;
            pres = progression_add(&g_game.state, target, a2);
            if (pres.crossed) {
                game_on_level_up(&g_game.state, target, &pres);
            }
            break;
        case DBG_ACT_BUY_ITEM:
            item_purchase(&g_game.state, (ItemId)a0);
            break;
        case DBG_ACT_USE_ITEM:
            item_use(&g_game.state, (ItemId)a0, (CharacterId)a2);
            break;
        case DBG_ACT_EQUIP_ITEM:
            item_equip(&g_game.state, (ItemId)a0);
            break;
        default:
            break;
    }
    debug_snapshot();
}

void scenario_check_and_load(void)
{
    if (g_scen_load_state) {
        g_scen_load_state = 0;
        scenario_load_state();
    }
    if (g_debug_action_pending) {
        g_debug_action_pending = 0;
        debug_run_action();
    }
    g_scen_load = 0;
}

#include "scenarios.h"
#include "rng.h"
#include "telemetry.h"
#include "game.h"
#include "screen.h"
#include "story.h"
#include "dialogue.h"
#include "world.h"
#include "entity.h"

extern Game g_game;
volatile uint8_t g_scen_load = 0;

/* Shared post-setup: reset frame/flags/input/telemetry/audio. */
static void scenario_begin(uint32_t seed)
{
    g_game.frame = 0;
    g_game.story_flags = 0;
    g_game.game_over_choice = 0;
    rng_set_seed(seed);
    input_reset();
    telemetry_init();
    telemetry_set_frame_ptr(&g_game.frame);
    audio_play_music(MUSIC_OVERWORLD);
}

/* Standard overworld setup on a given scene with a given player spawn. */
static void overworld_setup(SceneId scene, MapId map, uint8_t x, uint8_t y,
                            uint8_t facing, uint32_t seed, uint32_t flags)
{
    g_game.screen = SCREEN_OVERWORLD;
    g_game.prev_screen = SCREEN_OVERWORLD;
    g_game.scene = scene;
    world_init(&g_game.world);
    world_load_map(&g_game.world, map);
    g_game.world.player.position.x = x;
    g_game.world.player.position.y = y;
    g_game.world.player.facing = facing;
    g_game.world.encounter_triggered = false;
    dialogue_init(&g_game.dialogue);
    scenario_begin(seed);
    g_game.story_flags = flags;
}

static void load_new_game(void)
{
    overworld_setup(SCENE_FIELD, MAP_FIELD, 4, 4, DIRECTION_DOWN, 42, 0);
    debug_snapshot();
}

static void load_first_encounter(void)
{
    overworld_setup(SCENE_FIELD, MAP_FIELD, 13, 8, DIRECTION_DOWN, 12345, 0);
    g_game.world.enemy.position.x = 14;
    g_game.world.enemy.position.y = 8;
    debug_snapshot();
}

static void load_town_arrival(void)
{
    overworld_setup(SCENE_FIELD, MAP_FIELD, 17, 7, DIRECTION_DOWN, 999, 0);
    debug_snapshot();
}

static void load_town_departure(void)
{
    overworld_setup(SCENE_TOWN, MAP_TOWN, 2, 7, DIRECTION_DOWN, 1000, STORY_FLAG_ARRIVED_TOWN);
    debug_snapshot();
}

static void load_town_reentry(void)
{
    overworld_setup(SCENE_FIELD, MAP_FIELD, 17, 7, DIRECTION_DOWN, 1001, STORY_FLAG_ARRIVED_TOWN);
    debug_snapshot();
}

static void load_mayor_encounter(void)
{
    overworld_setup(SCENE_TOWN, MAP_TOWN, 9, 5, DIRECTION_RIGHT, 1002, STORY_FLAG_ARRIVED_TOWN);
    debug_snapshot();
}

static void load_mayor_dialogue(void)
{
    overworld_setup(SCENE_TOWN, MAP_TOWN, 9, 5, DIRECTION_RIGHT, 1003, STORY_FLAG_ARRIVED_TOWN);
    debug_snapshot();
}

static void load_mayor_dialogue_movement_blocked(void)
{
    overworld_setup(SCENE_TOWN, MAP_TOWN, 9, 5, DIRECTION_RIGHT, 1004, STORY_FLAG_ARRIVED_TOWN);
    dialogue_start_def(&g_game.dialogue, DIALOGUE_ID_MAYOR_GREETING);
    g_game.screen = SCREEN_DIALOGUE;
    debug_snapshot();
}

static void load_guard_dialogue(void)
{
    overworld_setup(SCENE_TOWN, MAP_TOWN, 9, 8, DIRECTION_RIGHT, 1005, STORY_FLAG_ARRIVED_TOWN);
    debug_snapshot();
}

static void load_font_test(void)
{
    overworld_setup(SCENE_FIELD, MAP_FIELD, 4, 4, DIRECTION_DOWN, 1999, 0);
    ui_draw_font_test();
    debug_snapshot();
}

static void load_dialogue_render_test(void)
{
    overworld_setup(SCENE_TOWN, MAP_TOWN, 9, 5, DIRECTION_RIGHT, 2000, STORY_FLAG_ARRIVED_TOWN);
    dialogue_start_def(&g_game.dialogue, DIALOGUE_ID_MAYOR_GREETING);
    g_game.screen = SCREEN_DIALOGUE;
    debug_snapshot();
}

static void load_battle_attack(void)
{
    overworld_setup(SCENE_FIELD, MAP_FIELD, 13, 8, DIRECTION_DOWN, 3000, 0);
    g_game.world.enemy.position.x = 14;
    g_game.world.enemy.position.y = 8;
    debug_snapshot();
}

static void load_guard_interaction_distance(void)
{
    overworld_setup(SCENE_TOWN, MAP_TOWN, 8, 8, DIRECTION_RIGHT, 3001, STORY_FLAG_ARRIVED_TOWN);
    debug_snapshot();
}

static void load_game_over(void)
{
    overworld_setup(SCENE_FIELD, MAP_FIELD, 13, 8, DIRECTION_DOWN, 4000, 0);
    g_game.world.player.hp = 2;
    g_game.world.enemy.position.x = 14;
    g_game.world.enemy.position.y = 8;
    debug_snapshot();
}

/* ── Screen / scene boot scenarios ──────────────────────────────── */

static void load_overworld_boot(void)
{
    overworld_setup(SCENE_TOWN, MAP_TOWN, 8, 6, DIRECTION_DOWN, 5000, STORY_FLAG_ARRIVED_TOWN);
    debug_snapshot();
}

static void load_dialogue_boot(void)
{
    overworld_setup(SCENE_TOWN, MAP_TOWN, 9, 5, DIRECTION_RIGHT, 5001, STORY_FLAG_ARRIVED_TOWN);
    dialogue_start_def(&g_game.dialogue, DIALOGUE_ID_MAYOR_GREETING);
    g_game.screen = SCREEN_DIALOGUE;
    g_game.dialogue.current_line = 0;
    debug_snapshot();
}

static void load_battle_boot(void)
{
    overworld_setup(SCENE_FIELD, MAP_FIELD, 13, 8, DIRECTION_DOWN, 5002, 0);
    g_game.world.enemy.position.x = 14;
    g_game.world.enemy.position.y = 8;
    battle_start(&g_game.battle, g_game.world.player.hp, g_game.world.player.max_hp,
                 g_game.world.enemy.hp, g_game.world.enemy.max_hp);
    g_game.screen = SCREEN_BATTLE;
    audio_play_music(MUSIC_BATTLE);
    debug_snapshot();
}

static void load_game_over_boot(void)
{
    overworld_setup(SCENE_FIELD, MAP_FIELD, 4, 4, DIRECTION_DOWN, 5003, 0);
    g_game.game_over_choice = 0;
    g_game.screen = SCREEN_GAME_OVER;
    debug_snapshot();
}

static void load_thanks_boot(void)
{
    overworld_setup(SCENE_FIELD, MAP_FIELD, 4, 4, DIRECTION_DOWN, 5004, 0);
    g_game.screen = SCREEN_THANKS;
    debug_snapshot();
}

static void load_forest_boot(void)
{
    overworld_setup(SCENE_FOREST, MAP_FOREST, 8, 6, DIRECTION_DOWN, 5005, 0);
    debug_snapshot();
}

static void load_mountain_pass_boot(void)
{
    overworld_setup(SCENE_MOUNTAIN_PASS, MAP_MOUNTAIN_PASS, 8, 6, DIRECTION_DOWN, 5006, 0);
    debug_snapshot();
}

static void load_castle_boot(void)
{
    overworld_setup(SCENE_CASTLE, MAP_CASTLE, 8, 6, DIRECTION_DOWN, 5007, STORY_FLAG_ARRIVED_TOWN);
    debug_snapshot();
}

static void load_town_boot(void)
{
    overworld_setup(SCENE_TOWN, MAP_TOWN, 8, 6, DIRECTION_DOWN, 5008, STORY_FLAG_ARRIVED_TOWN);
    debug_snapshot();
}

/* ── World Actor scenarios ──────────────────────────────────────── */

static void load_actor_collision_blocking(void)
{
    overworld_setup(SCENE_TOWN, MAP_TOWN, 9, 5, DIRECTION_RIGHT, 6000, STORY_FLAG_ARRIVED_TOWN);
    debug_snapshot();
}

static void load_actor_shopkeeper(void)
{
    overworld_setup(SCENE_TOWN, MAP_TOWN, 9, 4, DIRECTION_UP, 6001, STORY_FLAG_ARRIVED_TOWN);
    debug_snapshot();
}

static void load_actor_bat(void)
{
    overworld_setup(SCENE_CASTLE, MAP_CASTLE, 11, 7, DIRECTION_RIGHT, 6002, STORY_FLAG_ARRIVED_TOWN);
    debug_snapshot();
}

void scenario_check_and_load(void)
{
    uint8_t sc = g_scen_load;
    if (sc == 0) return;

    g_scen_load = 0;
    switch (sc) {
        case 1:  load_new_game(); break;
        case 2:  load_first_encounter(); break;
        case 3:  load_town_arrival(); break;
        case 4:  load_town_departure(); break;
        case 5:  load_town_reentry(); break;
        case 6:  load_mayor_encounter(); break;
        case 7:  load_mayor_dialogue(); break;
        case 8:  load_mayor_dialogue_movement_blocked(); break;
        case 9:  load_guard_dialogue(); break;
        case 10: load_font_test(); break;
        case 11: load_dialogue_render_test(); break;
        case 12: load_battle_attack(); break;
        case 13: load_guard_interaction_distance(); break;
        case 14: load_game_over(); break;
        case 15: load_overworld_boot(); break;
        case 16: load_dialogue_boot(); break;
        case 17: load_battle_boot(); break;
        case 18: load_game_over_boot(); break;
        case 19: load_thanks_boot(); break;
        case 20: load_forest_boot(); break;
        case 21: load_mountain_pass_boot(); break;
        case 22: load_castle_boot(); break;
        case 23: load_town_boot(); break;
        case 24: load_actor_collision_blocking(); break;
        case 25: load_actor_shopkeeper(); break;
        case 26: load_actor_bat(); break;
    }
    game_render_reset(&g_game);
    debug_snapshot();
}

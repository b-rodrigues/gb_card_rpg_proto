#include "scenarios.h"
#include "rng.h"
#include "telemetry.h"
#include "game.h"
#include "story.h"
#include "world.h"
#include "entity.h"
#include "state.h"

extern Game g_game;
volatile uint8_t g_scen_load = 0;


static void load_new_game(void)
{
    World *w = &g_game.world;
    GameStateMachine *sm = &g_game.state_machine;

    sm->current = GAME_STATE_OVERWORLD;
    sm->previous = GAME_STATE_OVERWORLD;
    sm->state_changed = false;

    world_init(w);   /* sets player (4,4) hp=10, enemy (14,8) hp=5 */
    w->encounter_triggered = false;

    g_game.frame = 0;
    g_game.story_flags = 0;

    rng_set_seed(42);
    input_reset();
    telemetry_init();
    telemetry_set_frame_ptr(&g_game.frame);
    audio_play_music(MUSIC_OVERWORLD);
    debug_snapshot();
}

static void load_first_encounter(void)
{
    World *w = &g_game.world;
    GameStateMachine *sm = &g_game.state_machine;

    sm->current = GAME_STATE_OVERWORLD;
    sm->previous = GAME_STATE_OVERWORLD;
    sm->state_changed = false;

    world_init(w);   /* initializes map and both entities */
    w->player.position.x = 13;
    w->player.position.y = 8;
    w->enemy.position.x = 14;
    w->enemy.position.y = 8;
    w->encounter_triggered = false;

    g_game.frame = 0;
    g_game.story_flags = 0;

    rng_set_seed(12345);
    input_reset();
    telemetry_init();
    telemetry_set_frame_ptr(&g_game.frame);
    audio_play_music(MUSIC_OVERWORLD);
    debug_snapshot();
}

static void load_town_arrival(void)
{
    World *w = &g_game.world;
    GameStateMachine *sm = &g_game.state_machine;

    sm->current = GAME_STATE_OVERWORLD;
    sm->previous = GAME_STATE_OVERWORLD;
    sm->state_changed = false;

    world_init(w);   /* Initializes MAP_FIELD */
    w->player.position.x = 17;
    w->player.position.y = 7;
    w->encounter_triggered = false;

    g_game.frame = 0;
    g_game.story_flags = 0;

    rng_set_seed(999);
    input_reset();
    telemetry_init();
    telemetry_set_frame_ptr(&g_game.frame);
    audio_play_music(MUSIC_OVERWORLD);
    debug_snapshot();
}

static void load_town_departure(void)
{
    World *w = &g_game.world;
    GameStateMachine *sm = &g_game.state_machine;

    sm->current = GAME_STATE_OVERWORLD;
    sm->previous = GAME_STATE_OVERWORLD;
    sm->state_changed = false;

    world_init(w);
    world_load_map(w, MAP_TOWN);
    w->player.position.x = 2;
    w->player.position.y = 7;
    w->encounter_triggered = false;

    g_game.frame = 0;
    g_game.story_flags = STORY_FLAG_ARRIVED_TOWN;

    rng_set_seed(1000);
    input_reset();
    telemetry_init();
    telemetry_set_frame_ptr(&g_game.frame);
    audio_play_music(MUSIC_OVERWORLD);
    debug_snapshot();
}

static void load_town_reentry(void)
{
    World *w = &g_game.world;
    GameStateMachine *sm = &g_game.state_machine;

    sm->current = GAME_STATE_OVERWORLD;
    sm->previous = GAME_STATE_OVERWORLD;
    sm->state_changed = false;

    world_init(w);
    w->player.position.x = 17;
    w->player.position.y = 7;
    w->encounter_triggered = false;

    g_game.frame = 0;
    g_game.story_flags = STORY_FLAG_ARRIVED_TOWN;

    rng_set_seed(1001);
    input_reset();
    telemetry_init();
    telemetry_set_frame_ptr(&g_game.frame);
    audio_play_music(MUSIC_OVERWORLD);
    debug_snapshot();
}

static void load_mayor_encounter(void)
{
    World *w = &g_game.world;
    GameStateMachine *sm = &g_game.state_machine;

    sm->current = GAME_STATE_OVERWORLD;
    sm->previous = GAME_STATE_OVERWORLD;
    sm->state_changed = false;

    world_init(w);
    world_load_map(w, MAP_TOWN);
    w->player.position.x = 9;
    w->player.position.y = 5;
    w->encounter_triggered = false;

    g_game.frame = 0;
    g_game.story_flags = STORY_FLAG_ARRIVED_TOWN;

    rng_set_seed(1002);
    input_reset();
    telemetry_init();
    telemetry_set_frame_ptr(&g_game.frame);
    audio_play_music(MUSIC_OVERWORLD);
    debug_snapshot();
}

void scenario_check_and_load(void)
{
    uint8_t sc = g_scen_load;
    if (sc == 1) {
        g_scen_load = 0;
        load_new_game();
    } else if (sc == 2) {
        g_scen_load = 0;
        load_first_encounter();
    } else if (sc == 3) {
        g_scen_load = 0;
        load_town_arrival();
    } else if (sc == 4) {
        g_scen_load = 0;
        load_town_departure();
    } else if (sc == 5) {
        g_scen_load = 0;
        load_town_reentry();
    } else if (sc == 6) {
        g_scen_load = 0;
        load_mayor_encounter();
    }
}

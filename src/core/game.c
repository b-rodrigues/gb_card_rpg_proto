#include "game.h"
#include "screen.h"
#include "story.h"
#include "dialogue.h"
#include "interaction.h"
#include "telemetry.h"
#include "scenarios.h"
#include "content.h"
#include "rpg/progression.h"
#include "rpg/party.h"
#include "rpg/items.h"

void game_render_reset(Game *g)
{
    if (!g) return;
    g->render_cache.valid = false;
    g->render_cache.prev_screen = (ScreenId)255;
    g->render_cache.prev_map_id = (MapId)255;
    g->render_cache.prev_player_x = 255;
    g->render_cache.prev_player_y = 255;
    g->render_cache.prev_dialogue_active = false;
    g->render_cache.prev_dialogue_line = 255;
    g->render_cache.prev_dialogue_id = DIALOGUE_ID_NONE;
    g->render_cache.prev_battle_turn = (BattleTurn)255;
    g->render_cache.prev_player_hp = 255;
    g->render_cache.prev_enemy_hp = 255;
    g->render_cache.prev_battle_result = (BattleResult)255;
    g->render_cache.prev_game_over_choice = 255;
}

void game_init(Game *g)
{
    if (!g) return;
    g->frame = 0;
    g->game_over_choice = 0;
    g->shop_id = 1;
    g->screen = SCREEN_OVERWORLD;
    g->prev_screen = SCREEN_OVERWORLD;
    telemetry_init();
    telemetry_set_frame_ptr(&g->frame);
    game_new_game(&g->state);
    world_init(&g->world, &g->state);
    dialogue_init(&g->dialogue);
    audio_play_music(MUSIC_OVERWORLD);
    telemetry_emit(EVENT_MUSIC_CHANGED, MUSIC_OVERWORLD, 0, 0, 0);

    game_render_reset(g);
    game_render(g);

#ifdef DEBUG_BUILD
    debug_snapshot();
#endif
}

/* Reset the world to a fresh new-game state (used by the Continue? menu). */
void game_restart(Game *g)
{
    if (!g) return;
    g->frame = 0;
    g->game_over_choice = 0;
    g->shop_id = 1;
    g->screen = SCREEN_OVERWORLD;
    g->prev_screen = SCREEN_OVERWORLD;
    game_new_game(&g->state);
    world_init(&g->world, &g->state);
    dialogue_init(&g->dialogue);
    audio_play_music(MUSIC_OVERWORLD);
    telemetry_emit(EVENT_MUSIC_CHANGED, MUSIC_OVERWORLD, 0, 0, 0);
    game_render_reset(g);
}

void game_update(Game *g)
{
    if (!g) return;

#ifdef DEBUG_BUILD
    scenario_check_and_load();
#endif
    g->frame++;

    screen_update(g);
    scene_sync_from_world(g);
    /* The world player entity mirrors the canonical party HP so the
     * overworld HUD and the core snapshot reflect healing immediately. */
    g->world.player.hp = g->state.party.members[0].hp;
    g->world.player.max_hp = g->state.party.members[0].max_hp;

#ifdef DEBUG_BUILD
    debug_snapshot();
#endif
}

void game_render(Game *g)
{
    RenderCache *rc;

    if (!g) return;
    rc = &g->render_cache;

    /* Any frame that performs a full redraw (screen change, map change,
     * boot/restart) hides the sprite in real OAM first: the redraw takes
     * several display sweeps (blank then top-to-bottom redraw), and the
     * sprite must not float over the wipe at a stale position.  The
     * frame-boundary commit (ui_sprite_commit in main.c, after vsync)
     * reveals it at the new screen's position once the redraw is done. */
    if (!rc->valid || rc->prev_screen != g->screen) {
        ui_sprite_begin_transition();
    }
    screen_render(g);
}

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
#include "banked.h"

/* Boot-phase breadcrumb set by main.c (see main.c).  game_render uses it to
 * tell the initial boot redraw (called directly from game_init, before the
 * main loop's vsync) apart from screen-change redraws: only the boot redraw
 * toggles the LCD off (its writes would otherwise land mid-scanout and be
 * dropped by the PPU -- the blank-screen bug).  Screen-change / map-change
 * redraws run after the main loop's vsync() and land in VBlank, so they must
 * not toggle the LCD. */
extern volatile uint8_t g_boot_phase;

void game_render_reset(Game *g)
{
    if (!g) return;
    g->render_cache.valid = false;
    /* The "previous screen" becomes whatever screen we are leaving, not a
     * 255 sentinel: every screen renderer already forces a full redraw via
     * rc->valid == false, and prev_screen then records the OLD screen so
     * dialogue can tell whether the overworld is still on the display. */
    g->render_cache.prev_screen = g->prev_screen;
    /* The map about to be drawn always matches the current world map.  A
     * 255 sentinel here made every non-overworld screen look like a map
     * change every frame, re-hiding the sprite for the whole of each
     * battle/dialogue/menu frame on real hardware.  Gate crossings change
     * world.map_id WITHOUT a reset, so the mismatch still fires there. */
    g->render_cache.prev_map_id = g->world.map_id;
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
    /* Install the WRAM banked-copy trampoline before any banked content
     * (event/dialogue tables) can be read.  Runs here rather than in CRT0
     * init because the harness jumps straight to main(). */
    banked_copy_init();
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
     * reveals it at the new screen's position once the redraw is done.
     *
     * The three triggers are distinct:
     *  - rc->valid == false: screen change / boot / restart (reset);
     *  - prev_screen != g->screen: a transition whose reset pre-dates the
     *    render (both halves of a frame with a screen change);
     *  - world.map_id != prev_map_id: a gate crossing.  This goes through
     *    world_change_map() without a screen change and without resetting
     *    the render cache, so overworld_screen_render() wipes the display
     *    based only on the map_id mismatch.  The condition mirrors that
     *    branch.
     *
     * On steady non-overworld frames (battle/dialogue/menu) none of the
     * three fire: prev_map_id was initialized to the current map at the
     * last reset and the map does not change while those screens are up,
     * so the sprite is NOT re-hidden every frame.  That per-frame re-hide
     * (from the old 255 prev_map_id sentinel) made the sprite invisible
     * for the whole fight/discussion on real hardware. */
    if (!rc->valid || rc->prev_screen != g->screen ||
        g->world.map_id != rc->prev_map_id) {
        ui_sprite_begin_transition();
        /* Full redraw.  Only the BOOT redraw runs with the LCD off
         * (g_boot_phase < 4): it is called directly from game_init with no
         * vsync() before it, so its writes would otherwise land mid-scanout
         * and be dropped by the PPU.  Screen-change / map-change redraws run
         * after the main loop's vsync() and land in VBlank, so they must NOT
         * toggle the LCD (turning it off after vsync() returned at LY==145
         * desyncs the PPU so the next vsync() cannot resync).  The sprite is
         * committed by the frame-boundary ui_sprite_commit() in main.c. */
        if (g_boot_phase < 4) {
            ui_lcd_off();
            screen_render(g);
            ui_lcd_on();
            return;
        }
    }
    screen_render(g);
}

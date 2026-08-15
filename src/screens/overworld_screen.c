#include "game.h"
#include "screen.h"
#include "scene.h"
#include "interaction.h"
#include "event.h"
#include "telemetry.h"
#include "audio.h"
#include "content.h"

/* Last drawn overworld camera offset, for redrawing the terrain tile window
 * when the camera scrolls (a file-static rather than a RenderCache field so
 * the Game struct layout stays untouched). */
static uint8_t s_prev_scroll_x;
static uint8_t s_prev_scroll_y;
static uint8_t s_prev_hero_ring_x;
static uint8_t s_prev_hero_ring_y;
static uint8_t s_hero_ring_valid;

static void start_battle_from_world(Game *g)
{
    uint8_t idx = g->world.encounter_actor_index;
    if (idx == NO_ACTOR_INDEX) return;

    /* Hero HP is authoritative in the party state; the world entity is the
     * runtime engine copy. */
    battle_start(&g->battle,
                 g->world.actors[idx].display_name ? g->world.actors[idx].display_name : "ENEMY",
                 g->state.party.members[0].hp,
                 g->state.party.members[0].max_hp,
                 game_hero_attack(&g->state),
                 g->world.actors[idx].hp, g->world.actors[idx].max_hp);
    audio_play_music(MUSIC_BATTLE);
    telemetry_emit(EVENT_MUSIC_CHANGED, MUSIC_BATTLE, 0, 0, 0);
    telemetry_emit(EVENT_ACTOR_COMBAT_START, (uint8_t)g->world.actors[idx].id, 0, 0, 0);
    screen_change(g, SCREEN_BATTLE);
    /* encounter_actor_index stays set until world_on_battle_end() */
}

void overworld_screen_update(Game *g)
{
    int8_t dx = 0;
    int8_t dy = 0;
    WorldMoveResult move_res = MOVE_RESULT_NONE;
    ActorEngageResult engage = ENGAGE_NONE;

    /* A committed move may resolve a map change or an encounter; resolve
     * those before reading fresh input. */
    move_res = world_update_move(&g->world, &g->state);

    /* Keep the camera on the player: scroll the view window when the player
     * crosses its edge, clamped to the scene bounds. */
    world_update_scroll(&g->world);

    if (input_pressed(INPUT_START)) {
        g->item_menu_index = 0;
        g->item_menu_tab = 0;
        screen_change(g, SCREEN_ITEM);
        return;
    }

    if (move_res == MOVE_RESULT_MAP_CHANGED) {
        g->world.map_changed = false;
        scene_update_from_map(g);
        event_resolve_map_enter(g, g->world.map_id);
        return;
    }
    if (move_res == MOVE_RESULT_ENCOUNTER) {
        start_battle_from_world(g);
        return;
    }

    /* Hold-to-move: input_held starts a move whenever the previous one has
     * finished animating; a held button stays active across frames (a fresh
     * press is just the first held frame). */
    if (g->world.move_state == MOVE_STATE_MOVING) {
        return;
    }

    if (input_held(INPUT_UP))    dy = -1;
    if (input_held(INPUT_DOWN))  dy = 1;
    if (input_held(INPUT_LEFT))  dx = -1;
    if (input_held(INPUT_RIGHT)) dx = 1;

    if (dx != 0 || dy != 0) {
        move_res = world_try_begin_move(&g->world, dx, dy, &g->state);
    }

    if (input_pressed(INPUT_A)) {
        engage = interaction_try_facing(g);
    } else if (move_res == MOVE_RESULT_BLOCKED) {
        engage = interaction_try_bump(g, dx, dy);
    }

    if (engage == ENGAGE_DIALOGUE) {
        screen_change(g, SCREEN_DIALOGUE);
        return;
    } else if (engage == ENGAGE_BATTLE) {
        start_battle_from_world(g);
        return;
    } else if (engage == ENGAGE_SHOP) {
        g->item_menu_index = 0;
        screen_change(g, SCREEN_SHOP);
        return;
    }
}

void overworld_screen_render(Game *g)
{
    RenderCache *rc;
    uint8_t px, py;
    uint8_t curr_player_tile_x, curr_player_tile_y;

    if (!g) return;
    rc = &g->render_cache;

    /* SCX/SCY is applied after any entering-edge tile writes below.  This
     * keeps the newly visible edge off-screen while it is being populated. */
    px = (uint8_t)(world_player_px(&g->world) - g->world.camera_px_x);
    py = (uint8_t)(world_player_py(&g->world) - g->world.camera_px_y);

    /* Map transition, cache reset, or return from Battle/Dialogue */
    if (!rc->valid || rc->prev_screen != SCREEN_OVERWORLD ||
        g->world.map_id != rc->prev_map_id) {
        ui_update_camera(&g->world);
        ui_draw_world_full(&g->world);
        telemetry_emit(EVENT_RENDER_SCREEN, (uint8_t)SCREEN_OVERWORLD, 0,
                       (uint8_t)g->world.map_id, 0);
        rc->valid = true;
        rc->prev_screen = SCREEN_OVERWORLD;
        rc->prev_map_id = g->world.map_id;
        s_prev_scroll_x = g->world.scroll_x;
        s_prev_scroll_y = g->world.scroll_y;
        s_hero_ring_valid = 0;
        curr_player_tile_x = (uint8_t)(g->world.scroll_x +
                                       ((world_player_px(&g->world) -
                                         g->world.camera_px_x + 4) >> 3));
        curr_player_tile_y = (uint8_t)(g->world.scroll_y +
                                       ((world_player_py(&g->world) -
                                         g->world.camera_px_y + 4) >> 3));
        ui_update_player_position(&g->world, 255, 255,
                                  curr_player_tile_x, curr_player_tile_y);
        s_prev_hero_ring_x = curr_player_tile_x;
        s_prev_hero_ring_y = curr_player_tile_y;
        s_hero_ring_valid = 1;
        rc->prev_player_x = px;
        rc->prev_player_y = py;
        rc->prev_dialogue_active = false;
        rc->prev_dialogue_line = 255;
        rc->prev_dialogue_id = DIALOGUE_ID_NONE;
        return;
    }

    /* Camera crossed a tile boundary.  Scrolling updates SCX/SCY with zero
     * VRAM writes because the full 32-column background ring is pre-populated
     * on LCD-safe full map redraws. */
    if (g->world.scroll_x != s_prev_scroll_x ||
        g->world.scroll_y != s_prev_scroll_y) {
        s_prev_scroll_x = g->world.scroll_x;
        s_prev_scroll_y = g->world.scroll_y;
        rc->prev_player_x = px;
        rc->prev_player_y = py;
    }

    ui_update_camera(&g->world);

    /* Compute the camera-relative hero tile in the background ring.  This
     * moves @ with the viewport instead of leaving it attached to a stale
     * world tile while SCX/SCY changes. */
    curr_player_tile_x = (uint8_t)(g->world.scroll_x +
                                   ((world_player_px(&g->world) -
                                     g->world.camera_px_x + 4) >> 3));
    curr_player_tile_y = (uint8_t)(g->world.scroll_y +
                                   ((world_player_py(&g->world) -
                                     g->world.camera_px_y + 4) >> 3));

    if (!s_hero_ring_valid || curr_player_tile_x != s_prev_hero_ring_x ||
        curr_player_tile_y != s_prev_hero_ring_y) {
        ui_update_player_position(&g->world,
                                   s_hero_ring_valid ? s_prev_hero_ring_x : 255,
                                   s_hero_ring_valid ? s_prev_hero_ring_y : 255,
                                   curr_player_tile_x, curr_player_tile_y);
        s_prev_hero_ring_x = curr_player_tile_x;
        s_prev_hero_ring_y = curr_player_tile_y;
        s_hero_ring_valid = 1;
    }

    if (px != rc->prev_player_x || py != rc->prev_player_y) {
        rc->prev_player_x = px;
        rc->prev_player_y = py;
    }
}

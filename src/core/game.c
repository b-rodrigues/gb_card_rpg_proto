#include "game.h"
#include "screen.h"
#include "story.h"
#include "dialogue.h"
#include "interaction.h"
#include "telemetry.h"
#include "scenarios.h"
#include "rpg/progression.h"
#include "rpg/party.h"
#include "rpg/items.h"

#define HERO_BASE_ATTACK 3

uint8_t game_hero_attack(const GameState *state)
{
    const ItemDefinition *def;
    if (!state) return HERO_BASE_ATTACK;
    def = item_get_def(state->equipment.weapon);
    if (def && def->kind == ITEM_KIND_WEAPON) {
        return (uint8_t)(HERO_BASE_ATTACK + def->attack_bonus);
    }
    return HERO_BASE_ATTACK;
}

void game_on_level_up(GameState *state, ProgressionTarget target,
                      const ProgressionAddResult *result)
{
    CharacterState *hero;
    uint8_t gained;
    uint8_t i;

    if (!state || !result || !result->crossed) return;
    gained = (uint8_t)(result->level_after - result->level_before);
    if (gained == 0) return;

    if (target.type == PROG_TYPE_HERO) {
        hero = party_get_member(&state->party, CHARACTER_HERO);
        if (!hero) return;
        for (i = 0; i < gained; i++) {
            if (hero->max_hp < 253) {
                hero->max_hp = (uint8_t)(hero->max_hp + 2);
            }
        }
        hero->hp = hero->max_hp;
    }
    /* Other target types have no game-specific consequence yet. */
}

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
    g->screen = SCREEN_OVERWORLD;
    g->prev_screen = SCREEN_OVERWORLD;
    telemetry_init();
    telemetry_set_frame_ptr(&g->frame);
    game_state_init(&g->state);
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
    g->screen = SCREEN_OVERWORLD;
    g->prev_screen = SCREEN_OVERWORLD;
    game_state_init(&g->state);
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

#ifdef DEBUG_BUILD
    debug_snapshot();
#endif
}

void game_render(Game *g)
{
    if (!g) return;
    screen_render(g);
}

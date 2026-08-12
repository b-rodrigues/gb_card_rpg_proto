#ifndef GAME_H
#define GAME_H

#include "screen.h"
#include "world.h"
#include "battle.h"
#include "audio.h"
#include "input.h"
#include "ui.h"

#include "dialogue.h"
#include "rpg/state.h"
#include "rpg/progression.h"

typedef struct {
    bool valid;
    ScreenId prev_screen;
    MapId prev_map_id;
    uint8_t prev_player_x;
    uint8_t prev_player_y;
    bool prev_dialogue_active;
    uint8_t prev_dialogue_line;
    DialogueId prev_dialogue_id;
    BattleTurn prev_battle_turn;
    uint8_t prev_player_hp;
    uint8_t prev_enemy_hp;
    BattleResult prev_battle_result;
    uint8_t prev_game_over_choice;
} RenderCache;

typedef struct Game {
    ScreenId screen;      /* currently active screen */
    ScreenId prev_screen; /* previous screen (for transitions) */
    GameState state;      /* canonical persistent RPG state */
    World world;
    Battle battle;
    DialogueState dialogue;
    uint32_t frame;
    uint8_t game_over_choice;  /* 0 = YES, 1 = NO on the continue prompt */
    uint8_t item_menu_index;   /* cursor into the active tab's list */
    uint8_t item_menu_tab;     /* 0 = ITEM, 1 = EQUIP, 2 = QUEST, 3 = STATUS */
    uint8_t shop_message;      /* 0 = none, 1 = bought, 2 = not enough gold */
    RenderCache render_cache;
} Game;

extern Game g_game;

void game_init(Game *g);
void game_restart(Game *g);
void game_update(Game *g);
void game_render(Game *g);
void game_render_reset(Game *g);

/* Game-specific consequence of a generic progression level-up.  Called by
 * whoever granted the progress when ProgressionAddResult.crossed is true.
 * The generic progression engine never knows what a level-up means. */
void game_on_level_up(GameState *state, ProgressionTarget target,
                      const ProgressionAddResult *result);

/* Derived hero attack: base 3 plus the equipped weapon's attack bonus. */
uint8_t game_hero_attack(const GameState *state);

#endif /* GAME_H */

#ifndef GAME_CONTENT_H
#define GAME_CONTENT_H

#include "rpg/state.h"
#include "rpg/progression.h"
#include "screen.h"

/* The game layer: everything that makes THIS game (content tables, named
 * ids, stat derivation, initial state, post-battle decisions).  The engine
 * (src/rpg, src/core, src/world, src/battle, src/ui) never references these
 * functions directly except through the hooks it is designed to call. */

/* Register all game content with the engine: story flag bound, event table,
 * dialogue table, and per-map actor tables.  Must run before gameplay. */
void game_content_init(void);

/* Build the canonical persistent state for a brand-new game (hero party,
 * starting scene/position, chapter variable, starting gold). */
void game_new_game(GameState *state);

/* Derived hero attack: base 3 plus the equipped weapon's attack bonus. */
uint8_t game_hero_attack(const GameState *state);

/* Game-specific consequence of a generic progression level-up. */
void game_on_level_up(GameState *state, ProgressionTarget target,
                      const ProgressionAddResult *result);

/* Decide the next screen after a battle victory (e.g. the ending once the
 * final boss is defeated, otherwise the overworld).  Called by the shared
 * battle screen. */
ScreenId game_screen_after_victory(const Game *g);

/* Generic enemy-sprite index (0=slime, 1=bat, 2=boss) for the battle
 * renderer, mapped from the actor's entity id.  The engine stores this on
 * Battle.enemy_gfx and never branches on game ids itself. */
uint8_t game_enemy_gfx(EntityId id);

/* Content registration helpers, implemented in the content modules. */
void game_events_register(void);
void game_dialogue_register(void);
void game_actors_register(void);
void game_items_register(void);
void game_quest_register(void);

#endif /* GAME_CONTENT_H */

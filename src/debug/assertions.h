#ifndef ASSERTIONS_H
#define ASSERTIONS_H

#include "game.h"
#include "telemetry.h"

typedef enum { ASSERT_PASS, ASSERT_FAIL } AssertResult;

AssertResult assert_game_state(const Game *g, GameState expected);
AssertResult assert_player_hp(const Game *g, uint8_t expected_hp);
AssertResult assert_player_position(const Game *g, uint8_t x, uint8_t y);
AssertResult assert_enemy_hp(const Game *g, uint8_t expected_hp);
AssertResult assert_enemy_active(const Game *g, bool expected);
AssertResult assert_event_occurred(GameEventType type);
AssertResult assert_event_not_occurred(GameEventType type);

#endif /* ASSERTIONS_H */

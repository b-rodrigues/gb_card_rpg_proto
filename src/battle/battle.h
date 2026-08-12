#ifndef BATTLE_H
#define BATTLE_H

#include "combatant.h"
#include <stdbool.h>

typedef enum {
    BATTLE_TURN_PLAYER,
    BATTLE_TURN_ENEMY_DELAY,
    BATTLE_TURN_ENEMY,
    BATTLE_TURN_RESULT
} BattleTurn;

typedef enum {
    BATTLE_ACTION_ATTACK,
    BATTLE_ACTION_RUN
} BattleAction;

typedef enum {
    BATTLE_RESULT_NONE,
    BATTLE_RESULT_VICTORY,
    BATTLE_RESULT_DEFEAT
} BattleResult;

typedef struct {
    Combatant player;
    Combatant enemy;
    BattleTurn turn;
    BattleResult result;
    uint8_t delay_timer;
    bool battle_over;
} Battle;

void battle_start(Battle *b, const char *enemy_name, uint8_t player_hp,
                  uint8_t player_max_hp, uint8_t player_attack,
                  uint8_t enemy_hp, uint8_t enemy_max_hp);
void battle_execute_action(Battle *b, BattleAction action);
void battle_update(Battle *b);

#endif /* BATTLE_H */

#include "battle.h"

void battle_start(Battle *b, uint8_t player_hp, uint8_t player_max_hp, uint8_t enemy_hp, uint8_t enemy_max_hp)
{
    if (!b) return;
    combatant_init(&b->player, "Hero", player_hp, player_max_hp);
    combatant_init(&b->enemy, "Slime", enemy_hp, enemy_max_hp);
    b->turn = BATTLE_TURN_PLAYER;
    b->result = BATTLE_RESULT_NONE;
    b->delay_timer = 0;
    b->battle_over = false;
}

void battle_execute_action(Battle *b, BattleAction action)
{
    uint8_t dmg;
    if (!b || b->turn != BATTLE_TURN_PLAYER || b->battle_over) return;

    if (action == BATTLE_ACTION_ATTACK) {
        dmg = 3;
        combatant_take_damage(&b->enemy, dmg);
        
        if (combatant_is_dead(&b->enemy)) {
            b->result = BATTLE_RESULT_VICTORY;
            b->turn = BATTLE_TURN_RESULT;
            b->battle_over = true;
        } else {
            b->turn = BATTLE_TURN_ENEMY_DELAY;
            b->delay_timer = 20; /* ~0.3s pause before enemy turn */
        }
    } else if (action == BATTLE_ACTION_RUN) {
        b->result = BATTLE_RESULT_VICTORY;
        b->turn = BATTLE_TURN_RESULT;
        b->battle_over = true;
    }
}

void battle_update(Battle *b)
{
    uint8_t dmg;
    if (!b || b->battle_over) return;

    if (b->turn == BATTLE_TURN_ENEMY_DELAY) {
        if (b->delay_timer > 0) {
            b->delay_timer--;
        } else {
            b->turn = BATTLE_TURN_ENEMY;
        }
    } else if (b->turn == BATTLE_TURN_ENEMY) {
        dmg = 2;
        combatant_take_damage(&b->player, dmg);

        if (combatant_is_dead(&b->player)) {
            b->result = BATTLE_RESULT_DEFEAT;
            b->turn = BATTLE_TURN_RESULT;
            b->battle_over = true;
        } else {
            b->turn = BATTLE_TURN_PLAYER;
        }
    }
}

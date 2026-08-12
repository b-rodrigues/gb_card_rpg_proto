#include "battle.h"
#include "telemetry.h"

void battle_start(Battle *b, const char *enemy_name, uint8_t player_hp,
                  uint8_t player_max_hp, uint8_t player_attack,
                  uint8_t enemy_hp, uint8_t enemy_max_hp)
{
    if (!b) return;
    combatant_init(&b->player, "Hero", player_hp, player_max_hp);
    combatant_init(&b->enemy, enemy_name ? enemy_name : "Enemy", enemy_hp, enemy_max_hp);
    b->player.attack = player_attack;
    b->enemy.attack = 2;
    b->turn = BATTLE_TURN_PLAYER;
    b->result = BATTLE_RESULT_NONE;
    b->delay_timer = 0;
    b->battle_over = false;
    telemetry_emit(EVENT_BATTLE_STARTED, 0, 0, 0, 0);
}

void battle_execute_action(Battle *b, BattleAction action)
{
    uint8_t dmg;
    if (!b || b->turn != BATTLE_TURN_PLAYER || b->battle_over) return;

    if (action == BATTLE_ACTION_ATTACK) {
        telemetry_emit(EVENT_BATTLE_ACTION, BATTLE_ACTION_ATTACK, 0, 0, 0);
        dmg = b->player.attack;
        combatant_take_damage(&b->enemy, dmg);
        telemetry_emit(EVENT_DAMAGE_DEALT, dmg, 0, 0, 0);
        
        if (combatant_is_dead(&b->enemy)) {
            telemetry_emit(EVENT_ENTITY_DEFEATED, 1, 0, 0, 0);
            b->result = BATTLE_RESULT_VICTORY;
            b->turn = BATTLE_TURN_RESULT;
            b->battle_over = true;
            telemetry_emit(EVENT_BATTLE_WON, 0, 0, 0, 0);
        } else {
            b->turn = BATTLE_TURN_ENEMY_DELAY;
            b->delay_timer = 20; /* ~0.3s pause before enemy turn */
        }
    } else if (action == BATTLE_ACTION_RUN) {
        telemetry_emit(EVENT_BATTLE_ACTION, BATTLE_ACTION_RUN, 0, 0, 0);
        b->result = BATTLE_RESULT_FLED;
        b->turn = BATTLE_TURN_RESULT;
        b->battle_over = true;
        telemetry_emit(EVENT_BATTLE_FLED, 0, 0, 0, 0);
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
        telemetry_emit(EVENT_DAMAGE_RECEIVED, dmg, 0, 0, 0);

        if (combatant_is_dead(&b->player)) {
            telemetry_emit(EVENT_ENTITY_DEFEATED, 0, 0, 0, 0);
            b->result = BATTLE_RESULT_DEFEAT;
            b->turn = BATTLE_TURN_RESULT;
            b->battle_over = true;
            telemetry_emit(EVENT_BATTLE_LOST, 0, 0, 0, 0);
        } else {
            b->turn = BATTLE_TURN_PLAYER;
        }
    }
}

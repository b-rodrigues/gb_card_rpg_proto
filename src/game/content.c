#include "content.h"
#include "game_ids.h"
#include "story.h"
#include "event.h"
#include "dialogue.h"
#include "actor.h"
#include "rpg/items.h"
#include "rpg/party.h"
#include "core/game.h"

#define HERO_BASE_ATTACK 3
#define HERO_START_HP    10
#define HERO_START_GOLD  20

void game_content_init(void)
{
    story_init(STORY_FLAG_ID_COUNT);
    game_events_register();
    game_dialogue_register();
    game_actors_register();
    game_items_register();
    game_quest_register();
}

void game_new_game(GameState *state)
{
    if (!state) return;
    game_state_zero(state);

    state->scene.scene_id = SCENE_FIELD;
    state->scene.player_x = 4;
    state->scene.player_y = 4;
    state->scene.player_facing = (uint8_t)DIRECTION_DOWN;

    state->party.count = 1;
    state->party.members[0].id = CHARACTER_HERO;
    state->party.members[0].hp = HERO_START_HP;
    state->party.members[0].max_hp = HERO_START_HP;

    state->variables.values[VARIABLE_ID_CHAPTER - 1] = 1;
    state->currency.amount[CURRENCY_ID_GOLD - 1] = HERO_START_GOLD;
}

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

ScreenId game_screen_after_victory(const Game *g)
{
    if (!g) return SCREEN_OVERWORLD;
    if (game_variable_get(&g->state, VARIABLE_ID_ENDING_SHOWN) != 0) {
        return SCREEN_ENDING;
    }
    return SCREEN_OVERWORLD;
}

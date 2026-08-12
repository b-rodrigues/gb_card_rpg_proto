#include "rpg/state.h"
#include "telemetry.h"

static bool flag_id_valid(FlagId flag)
{
    return (flag >= 1 && flag <= MAX_STATE_FLAGS);
}

static bool variable_id_valid(VariableId variable)
{
    return (variable >= 1 && variable <= MAX_STATE_VARIABLES);
}

void game_state_reset(GameState *state)
{
    uint8_t i;
    if (!state) return;

    state->scene.scene_id = SCENE_FIELD;
    state->scene.player_x = 4;
    state->scene.player_y = 4;
    state->scene.player_facing = (uint8_t)DIRECTION_DOWN;

    state->party.count = 1;
    state->party.members[0].id = CHARACTER_HERO;
    state->party.members[0].hp = 10;
    state->party.members[0].max_hp = 10;
    for (i = 1; i < MAX_PARTY_MEMBERS; i++) {
        state->party.members[i].id = CHARACTER_NONE;
        state->party.members[i].hp = 0;
        state->party.members[i].max_hp = 0;
    }

    state->inventory.count = 0;
    for (i = 0; i < MAX_INVENTORY_ITEMS; i++) {
        state->inventory.entries[i].item_id = ITEM_NONE;
        state->inventory.entries[i].quantity = 0;
    }

    for (i = 0; i < (MAX_STATE_FLAGS / 8); i++) {
        state->flags.bytes[i] = 0;
    }

    for (i = 0; i < MAX_STATE_VARIABLES; i++) {
        state->variables.values[i] = 0;
    }
    state->variables.values[VARIABLE_ID_CHAPTER - 1] = 1;

    for (i = 0; i < MAX_CURRENCIES; i++) {
        state->currency.amount[i] = 0;
    }
    state->currency.amount[CURRENCY_ID_GOLD - 1] = 20; /* the hero starts with 20 gold */

    state->world.count = 0;
    for (i = 0; i < MAX_PERSISTENT_ACTORS; i++) {
        state->world.actors[i].actor_id = 0;
        state->world.actors[i].state = (uint8_t)ACTOR_STATE_ALIVE;
    }

    state->progression.count = 0;
    for (i = 0; i < MAX_PROGRESSION_TARGETS; i++) {
        state->progression.entries[i].target.type = (uint8_t)PROG_TYPE_NONE;
        state->progression.entries[i].target.id = 0;
        state->progression.entries[i].state.level = 0;
        state->progression.entries[i].state.progress = 0;
    }

    state->equipment.weapon = ITEM_NONE;
}

void game_state_init(GameState *state)
{
    game_state_reset(state);
}

bool game_flag_is_set(const GameState *state, FlagId flag)
{
    uint8_t byte, bit;
    if (!state || !flag_id_valid(flag)) return false;
    byte = (uint8_t)((flag - 1) >> 3);
    bit = (uint8_t)((flag - 1) & 0x07);
    return (state->flags.bytes[byte] & (uint8_t)(1U << bit)) != 0;
}

void game_flag_set(GameState *state, FlagId flag)
{
    uint8_t byte, bit;
    if (!state || !flag_id_valid(flag)) return;
    byte = (uint8_t)((flag - 1) >> 3);
    bit = (uint8_t)((flag - 1) & 0x07);
    if (!(state->flags.bytes[byte] & (uint8_t)(1U << bit))) {
        state->flags.bytes[byte] |= (uint8_t)(1U << bit);
        telemetry_emit(EVENT_STORY_FLAG_SET, (uint8_t)flag, 0, 0, 0);
    }
}

void game_flag_clear(GameState *state, FlagId flag)
{
    uint8_t byte, bit;
    if (!state || !flag_id_valid(flag)) return;
    byte = (uint8_t)((flag - 1) >> 3);
    bit = (uint8_t)((flag - 1) & 0x07);
    if (state->flags.bytes[byte] & (uint8_t)(1U << bit)) {
        state->flags.bytes[byte] &= (uint8_t)~(1U << bit);
        telemetry_emit(EVENT_STORY_FLAG_CLEARED, (uint8_t)flag, 0, 0, 0);
    }
}

int16_t game_variable_get(const GameState *state, VariableId variable)
{
    if (!state || !variable_id_valid(variable)) return 0;
    return state->variables.values[variable - 1];
}

void game_variable_set(GameState *state, VariableId variable, int16_t value)
{
    if (!state || !variable_id_valid(variable)) return;
    state->variables.values[variable - 1] = value;
    telemetry_emit(EVENT_VARIABLE_SET, (uint8_t)variable,
                   (uint8_t)(value & 0xFF), (uint8_t)((value >> 8) & 0xFF), 0);
}

void game_variable_add(GameState *state, VariableId variable, int16_t amount)
{
    int16_t new_value;
    if (!state || !variable_id_valid(variable)) return;
    new_value = (int16_t)(state->variables.values[variable - 1] + amount);
    state->variables.values[variable - 1] = new_value;
    telemetry_emit(EVENT_VARIABLE_SET, (uint8_t)variable,
                   (uint8_t)(new_value & 0xFF), (uint8_t)((new_value >> 8) & 0xFF), 0);
}

bool game_world_actor_is_defeated(const GameState *state, ActorId actor_id)
{
    uint8_t i;
    if (!state || actor_id == 0) return false;
    for (i = 0; i < state->world.count; i++) {
        if (state->world.actors[i].actor_id == actor_id) {
            return (state->world.actors[i].state == (uint8_t)ACTOR_STATE_DEFEATED);
        }
    }
    return false;
}

void game_world_set_actor_state(GameState *state, ActorId actor_id,
                                ActorStateId actor_state)
{
    uint8_t i;
    if (!state || actor_id == 0) return;

    for (i = 0; i < state->world.count; i++) {
        if (state->world.actors[i].actor_id == actor_id) {
            if (state->world.actors[i].state != (uint8_t)actor_state) {
                state->world.actors[i].state = (uint8_t)actor_state;
                telemetry_emit(EVENT_ACTOR_STATE_CHANGE,
                               (uint8_t)(actor_id & 0xFF),
                               (uint8_t)((actor_id >> 8) & 0xFF),
                               (uint8_t)actor_state, 0);
            }
            return;
        }
    }

    if (state->world.count >= MAX_PERSISTENT_ACTORS) return;
    state->world.actors[state->world.count].actor_id = actor_id;
    state->world.actors[state->world.count].state = (uint8_t)actor_state;
    state->world.count++;
    telemetry_emit(EVENT_ACTOR_STATE_CHANGE,
                   (uint8_t)(actor_id & 0xFF),
                   (uint8_t)((actor_id >> 8) & 0xFF),
                   (uint8_t)actor_state, 0);
}

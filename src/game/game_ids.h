#ifndef GAME_IDS_H
#define GAME_IDS_H

/* Game-specific semantic IDs for this RPG's content.  The engine exposes
 * generic slots (GameState flags/variables/currency indexed by 1-based
 * numeric ids); the game layer names the ones this game uses.  A different
 * RPG built on the same engine defines its own named ids here. */

/* Story flags.  Each maps to bit (id-1) of GameState.flags.bytes[].
 * STORY_FLAG_ID_COUNT is the exclusive upper bound passed to story_init(). */
typedef enum {
    STORY_FLAG_ID_ARRIVED_TOWN = 1,
    STORY_FLAG_ID_MET_MAYOR    = 2,
    STORY_FLAG_ID_COUNT        = 3
} StoryFlagId;

/* Named variables.  VARIABLE_ID_x - 1 indexes VariableState.values[]. */
typedef enum {
    VARIABLE_ID_CHAPTER            = 1,
    VARIABLE_ID_MONSTERS_DEFEATED  = 2,   /* global total (all kills count) */
    VARIABLE_ID_QUEST_MONSTER_HUNT = 3,   /* 0 = NOT_STARTED, 1 = ACTIVE, 2 = COMPLETE */
    VARIABLE_ID_ENDING_SHOWN       = 4,   /* set when the final boss is defeated */
    VARIABLE_ID_MERCHANT_QUEST     = 5    /* 0 = not started, 1 = amulet found, 2 = complete */
} VariableIdNamed;

/* Named currencies.  CURRENCY_ID_x - 1 indexes CurrencyState.amount[]. */
typedef enum {
    CURRENCY_ID_GOLD = 1
} CurrencyIdNamed;

#endif /* GAME_IDS_H */

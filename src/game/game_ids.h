#ifndef GAME_IDS_H
#define GAME_IDS_H

#include "entity.h"
#include "event.h"
#include "dialogue.h"
#include "rpg/state.h"

/* Game-specific semantic IDs for this RPG's content.  The engine exposes
 * generic slots (GameState flags/variables/currency indexed by 1-based
 * numeric ids) and the shared identity vocabulary (entity/event/dialogue/
 * item types).  The game layer names the ones this game uses: entity/event/
 * dialogue/item ids live in the per-game range starting at the engine's
 * *_FIRST_GAME bases, so a different RPG built on the same engine defines
 * its own ids here without ever touching the engine headers. */

/* ── Entity types (engine range: NONE=0, PLAYER=1; game range >= 0x80) ── */
#define ENTITY_ID_SLIME       (ENTITY_ID_FIRST_GAME + 0)
#define ENTITY_ID_MAYOR       (ENTITY_ID_FIRST_GAME + 1)
#define ENTITY_ID_GUARD       (ENTITY_ID_FIRST_GAME + 2)
#define ENTITY_ID_SHOPKEEPER  (ENTITY_ID_FIRST_GAME + 3)
#define ENTITY_ID_BAT         (ENTITY_ID_FIRST_GAME + 4)
#define ENTITY_ID_SLIME_LORD  (ENTITY_ID_FIRST_GAME + 5)
#define ENTITY_ID_MERCHANT    (ENTITY_ID_FIRST_GAME + 6)
#define ENTITY_ID_AMULET      (ENTITY_ID_FIRST_GAME + 7)

/* ── Items (engine range: NONE=0; game range >= 0x80) ── */
#define ITEM_POTION  (ITEM_FIRST_GAME + 0)
#define ITEM_BOMB    (ITEM_FIRST_GAME + 1)
#define ITEM_ETHER   (ITEM_FIRST_GAME + 2)
#define ITEM_SWORD   (ITEM_FIRST_GAME + 3)
#define ITEM_AMULET  (ITEM_FIRST_GAME + 4)
#define ITEM_NUT     (ITEM_FIRST_GAME + 5)

/* ── Events (engine range: NONE=0; game range >= 0x80) ── */
#define EVENT_ID_TOWN_ARRIVAL    (EVENT_ID_FIRST_GAME + 0)
#define EVENT_ID_QUEST_START     (EVENT_ID_FIRST_GAME + 1)
#define EVENT_ID_QUEST_ACTIVE    (EVENT_ID_FIRST_GAME + 2)
#define EVENT_ID_QUEST_COMPLETE  (EVENT_ID_FIRST_GAME + 3)
#define EVENT_ID_QUEST_DONE      (EVENT_ID_FIRST_GAME + 4)
#define EVENT_ID_GUARD_AFTER_MAYOR (EVENT_ID_FIRST_GAME + 5)
#define EVENT_ID_GUARD_GREETING  (EVENT_ID_FIRST_GAME + 6)
#define EVENT_ID_MONSTER_DEFEATED (EVENT_ID_FIRST_GAME + 7)
#define EVENT_ID_BOSS_DEFEATED   (EVENT_ID_FIRST_GAME + 8)
#define EVENT_ID_MERCHANT_INTRO  (EVENT_ID_FIRST_GAME + 9)
#define EVENT_ID_MERCHANT_DELIVER (EVENT_ID_FIRST_GAME + 10)
#define EVENT_ID_AMULET_PICKUP   (EVENT_ID_FIRST_GAME + 11)

/* ── Dialogues (engine range: NONE=0; game range >= 0x80) ── */
#define DIALOGUE_ID_MAYOR_GREETING    (DIALOGUE_ID_FIRST_GAME + 0)
#define DIALOGUE_ID_GUARD_GREETING    (DIALOGUE_ID_FIRST_GAME + 1)
#define DIALOGUE_ID_SHOPKEEPER_GREETING (DIALOGUE_ID_FIRST_GAME + 2)
#define DIALOGUE_ID_MAYOR_INTRO       (DIALOGUE_ID_FIRST_GAME + 3)
#define DIALOGUE_ID_GUARD_AFTER_MAYOR (DIALOGUE_ID_FIRST_GAME + 4)
#define DIALOGUE_ID_QUEST_ACTIVE      (DIALOGUE_ID_FIRST_GAME + 5)
#define DIALOGUE_ID_QUEST_COMPLETE    (DIALOGUE_ID_FIRST_GAME + 6)
#define DIALOGUE_ID_QUEST_DONE        (DIALOGUE_ID_FIRST_GAME + 7)
#define DIALOGUE_ID_MERCHANT_INTRO    (DIALOGUE_ID_FIRST_GAME + 8)
#define DIALOGUE_ID_MERCHANT_THANKS   (DIALOGUE_ID_FIRST_GAME + 9)
#define DIALOGUE_ID_AMULET_FOUND      (DIALOGUE_ID_FIRST_GAME + 10)
#define DIALOGUE_ID_AMULET_NOTHING    (DIALOGUE_ID_FIRST_GAME + 11)

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

/* ── Banked content ────────────────────────────────────────────────
 * The event and dialogue content tables are compiled into MBC5 ROM bank
 * GAME_CONTENT_BANK (see events_content.c / dialogue_content.c, which use
 * `#pragma bank 2`).  Bank 2 is used because the project links with -yo8:
 * bank 0 legitimately spans the full 32 KB (file 0x0000-0x7FFF), so bank 1
 * (file 0x4000-0x7FFF) overlaps bank 0's second half and sdldgb's
 * "Multiple write" overwrites the tables with bank-0 code.  Bank 2
 * (file 0x8000-0xBFFF) is clear.  The engine reads the tables through WRAM
 * scratch copies (core/event.c, core/dialogue.c, banked_copy in crt0.s), so
 * no gameplay code ever reads banked data directly.  The *_COUNT values are
 * compile-time-asserted against the tables in the content files. */
#define GAME_CONTENT_BANK 2
#define GAME_EVENT_COUNT 12
#define GAME_DIALOGUE_COUNT 12

#endif /* GAME_IDS_H */

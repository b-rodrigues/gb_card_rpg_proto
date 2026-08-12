#include "event.h"
#include "game_ids.h"

/* ── Event table (game content) ────────────────────────────────────
 * First match in table order wins.  For INTERACT events, more specific
 * conditions must precede the default fallback.
 *
 * Mayor quest (MONSTER HUNT), expressed as data, not game code:
 *   QUEST_START    : first meeting (quest NOT_STARTED) starts the quest:
 *                    MAYOR_INTRO dialogue, MET_MAYOR, quest=ACTIVE.
 *   QUEST_COMPLETE : quest ACTIVE and >= 3 monsters defeated -> reward
 *                    dialogue, give SWORD, quest=COMPLETE (given once).
 *   QUEST_ACTIVE   : quest ACTIVE (fewer than 3 defeated) -> "still working".
 *   QUEST_DONE     : quest COMPLETE -> already-rewarded dialogue.
 *   MONSTER_DEFEATED : every hostile defeat increments the global
 *                    MONSTERS_DEFEATED counter (no quest gating), so kills
 *                    before the quest starts still count.
 */
static const EventDefinition g_events[] = {
    {
        EVENT_ID_TOWN_ARRIVAL, EVENT_TRIGGER_MAP_ENTER, ENTITY_ID_NONE, MAP_TOWN,
        1,
        {{ EVENT_COND_FLAG, STORY_FLAG_ID_ARRIVED_TOWN, 0, false, false }},
        1,
        {{ EVENT_ACTION_SET_FLAG, STORY_FLAG_ID_ARRIVED_TOWN, 0, 0 }}
    },
    {
        EVENT_ID_QUEST_START, EVENT_TRIGGER_INTERACT, ENTITY_ID_MAYOR, EVENT_MAP_ANY,
        1,
        {{ EVENT_COND_VARIABLE, VARIABLE_ID_QUEST_MONSTER_HUNT, 0, false, false }},
        3,
        {
            { EVENT_ACTION_DIALOGUE, DIALOGUE_ID_MAYOR_INTRO, 0, 0 },
            { EVENT_ACTION_SET_FLAG, STORY_FLAG_ID_MET_MAYOR, 0, 0 },
            { EVENT_ACTION_SET_VARIABLE, VARIABLE_ID_QUEST_MONSTER_HUNT, 1, 0 }
        }
    },
    {
        EVENT_ID_QUEST_COMPLETE, EVENT_TRIGGER_INTERACT, ENTITY_ID_MAYOR, EVENT_MAP_ANY,
        2,
        {
            { EVENT_COND_VARIABLE, VARIABLE_ID_QUEST_MONSTER_HUNT, 1, false, false },
            { EVENT_COND_VARIABLE, VARIABLE_ID_MONSTERS_DEFEATED, 3, false, true }
        },
        3,
        {
            { EVENT_ACTION_DIALOGUE, DIALOGUE_ID_QUEST_COMPLETE, 0, 0 },
            { EVENT_ACTION_ADD_ITEM, ITEM_SWORD, 1, 0 },
            { EVENT_ACTION_SET_VARIABLE, VARIABLE_ID_QUEST_MONSTER_HUNT, 2, 0 }
        }
    },
    {
        EVENT_ID_QUEST_ACTIVE, EVENT_TRIGGER_INTERACT, ENTITY_ID_MAYOR, EVENT_MAP_ANY,
        1,
        {{ EVENT_COND_VARIABLE, VARIABLE_ID_QUEST_MONSTER_HUNT, 1, false, false }},
        1,
        {{ EVENT_ACTION_DIALOGUE, DIALOGUE_ID_QUEST_ACTIVE, 0, 0 }}
    },
    {
        EVENT_ID_QUEST_DONE, EVENT_TRIGGER_INTERACT, ENTITY_ID_MAYOR, EVENT_MAP_ANY,
        1,
        {{ EVENT_COND_VARIABLE, VARIABLE_ID_QUEST_MONSTER_HUNT, 2, false, false }},
        1,
        {{ EVENT_ACTION_DIALOGUE, DIALOGUE_ID_QUEST_DONE, 0, 0 }}
    },
    {
        EVENT_ID_BOSS_DEFEATED, EVENT_TRIGGER_ACTOR_DEFEATED, ENTITY_ID_SLIME_LORD, EVENT_MAP_ANY,
        0,
        {{ EVENT_COND_NONE, 0, 0, false, false }},
        1,
        {{ EVENT_ACTION_SET_VARIABLE, VARIABLE_ID_ENDING_SHOWN, 1, 0 }}
    },
    {
        EVENT_ID_MONSTER_DEFEATED, EVENT_TRIGGER_ACTOR_DEFEATED, ENTITY_ID_NONE, EVENT_MAP_ANY,
        0,
        {{ EVENT_COND_NONE, 0, 0, false, false }},
        1,
        {{ EVENT_ACTION_ADD_VARIABLE, VARIABLE_ID_MONSTERS_DEFEATED, 1, 0 }}
    },
    {
        EVENT_ID_GUARD_AFTER_MAYOR, EVENT_TRIGGER_INTERACT, ENTITY_ID_GUARD, EVENT_MAP_ANY,
        1,
        {{ EVENT_COND_FLAG, STORY_FLAG_ID_MET_MAYOR, 0, true, false }},
        1,
        {{ EVENT_ACTION_DIALOGUE, DIALOGUE_ID_GUARD_AFTER_MAYOR, 0, 0 }}
    },
    {
        EVENT_ID_GUARD_GREETING, EVENT_TRIGGER_INTERACT, ENTITY_ID_GUARD, EVENT_MAP_ANY,
        1,
        {{ EVENT_COND_FLAG, STORY_FLAG_ID_MET_MAYOR, 0, false, false }},
        1,
        {{ EVENT_ACTION_DIALOGUE, DIALOGUE_ID_GUARD_GREETING, 0, 0 }}
    },
    /* ── Lost Merchant quest (data-driven fetch/deliver) ─────────────
     * Quest state is the MERCHANT_QUEST variable: 0 = not started,
     * 1 = amulet found (ACTIVE), 2 = complete.  DELIVER must precede INTRO
     * (more specific first).  After delivery no merchant event matches, so
     * the interaction falls back to the merchant's SHOP interaction. */
    {
        EVENT_ID_MERCHANT_DELIVER, EVENT_TRIGGER_INTERACT, ENTITY_ID_MERCHANT, EVENT_MAP_ANY,
        2,
        {
            { EVENT_COND_ITEM_COUNT, ITEM_AMULET, 1, false, true },
            { EVENT_COND_VARIABLE, VARIABLE_ID_MERCHANT_QUEST, 1, false, false }
        },
        4,
        {
            { EVENT_ACTION_DIALOGUE, DIALOGUE_ID_MERCHANT_THANKS, 0, 0 },
            { EVENT_ACTION_ADD_CURRENCY, CURRENCY_ID_GOLD, 15, 0 },
            { EVENT_ACTION_REMOVE_ITEM, ITEM_AMULET, 1, 0 },
            { EVENT_ACTION_SET_VARIABLE, VARIABLE_ID_MERCHANT_QUEST, 2, 0 }
        }
    },
    {
        EVENT_ID_MERCHANT_INTRO, EVENT_TRIGGER_INTERACT, ENTITY_ID_MERCHANT, EVENT_MAP_ANY,
        1,
        {{ EVENT_COND_VARIABLE, VARIABLE_ID_MERCHANT_QUEST, 0, false, false }},
        1,
        {{ EVENT_ACTION_DIALOGUE, DIALOGUE_ID_MERCHANT_INTRO, 0, 0 }}
    },
    {
        EVENT_ID_AMULET_PICKUP, EVENT_TRIGGER_INTERACT, ENTITY_ID_AMULET, EVENT_MAP_ANY,
        1,
        {{ EVENT_COND_VARIABLE, VARIABLE_ID_MERCHANT_QUEST, 0, false, false }},
        3,
        {
            { EVENT_ACTION_DIALOGUE, DIALOGUE_ID_AMULET_FOUND, 0, 0 },
            { EVENT_ACTION_ADD_ITEM, ITEM_AMULET, 1, 0 },
            { EVENT_ACTION_SET_VARIABLE, VARIABLE_ID_MERCHANT_QUEST, 1, 0 }
        }
    }
};

void game_events_register(void)
{
    event_init(g_events, (uint8_t)(sizeof(g_events) / sizeof(g_events[0])));
}

#include "dialogue.h"
#include "game_ids.h"

/* ── Dialogue table (game content) ───────────────────────────────── */
static const DialogueDefinition g_dialogues[] = {
    {
        DIALOGUE_ID_MAYOR_GREETING,
        "MAYOR:",
        2,
        {"Hello! I am Mayor.", "Welcome to town!", "", ""},
        0
    },
    {
        DIALOGUE_ID_GUARD_GREETING,
        "GUARD:",
        2,
        {"Halt! Keep peace.", "Watch for slimes.", "", ""},
        0
    },
    {
        DIALOGUE_ID_SHOPKEEPER_GREETING,
        "SHOP:",
        2,
        {"Welcome to my shop.", "Rest a while, friend.", "", ""},
        0
    },
    {
        DIALOGUE_ID_MAYOR_INTRO,
        "MAYOR:",
        3,
        {"I am the Mayor.", "Slimes menace the forest.", "Please help us!", ""},
        0
    },
    {
        DIALOGUE_ID_GUARD_AFTER_MAYOR,
        "GUARD:",
        2,
        {"The Mayor trusts you.", "Welcome, hero!", "", ""},
        0
    },
    {
        DIALOGUE_ID_QUEST_ACTIVE,
        "MAYOR:",
        2,
        {"Still monsters about.", "Defeat them all!", "", ""},
        0
    },
    {
        DIALOGUE_ID_QUEST_COMPLETE,
        "MAYOR:",
        8,
        {"You did it!", "Take this Sword!",
         "I have felt a", "disturbance in the",
         "ether: I believe", "that something",
         "dreadful waits for", "you at the Castle."},
        0
    },
    {
        DIALOGUE_ID_QUEST_DONE,
        "MAYOR:",
        2,
        {"The Sword suits you.", "Go forth, hero!", "", ""},
        0
    },
    {
        DIALOGUE_ID_MERCHANT_INTRO,
        "MERCHANT:",
        3,
        {"A thief stole my", "family heirloom!", "Find it in the Forest!"},
        0
    },
    {
        DIALOGUE_ID_MERCHANT_THANKS,
        "MERCHANT:",
        3,
        {"You found it!", "Thank you, hero!", "My shop is open now."},
        0
    },
    {
        DIALOGUE_ID_AMULET_FOUND,
        "",
        2,
        {"You found the", "Lost Amulet!"},
        0
    },
    {
        DIALOGUE_ID_AMULET_NOTHING,
        "",
        1,
        {"Nothing here now."},
        0
    }
};

void game_dialogue_register(void)
{
    dialogue_register(g_dialogues,
                      (uint8_t)(sizeof(g_dialogues) / sizeof(g_dialogues[0])));
}

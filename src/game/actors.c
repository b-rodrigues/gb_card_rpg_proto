#include "actor.h"
#include "game_ids.h"
#include "ui.h"

/* ── Scene-owned actor definitions (game content) ──────────────────
 *
 * Friendly actors are pure static definitions.  Hostile actors are
 * spawned into World.actors runtime slots by actor_load_scene(), so a
 * scene can hold several hostile actors at once.  Each hostile definition
 * carries a stable ActorId (unique across scenes) so its defeat can be
 * recorded persistently in GameState.world and survive scene reloads.
 *
 * Every actor carries a real-tileset rendering: enemies/boss reference the
 * enemy sprite tilesets, NPCs use placeholder letter tiles (assets/npc_*.png,
 * see docs/graphics.md).  tile/tile_w/tile_h are VRAM presentation data;
 * visual stays the ASCII semantic char for the harness view.
 */

static const WorldActorDefinition g_town_actors[] = {
    {
        0, ENTITY_ID_MAYOR, 10, 5, DIRECTION_DOWN,
        ACTOR_FLAG_BLOCKING | ACTOR_FLAG_INTERACTABLE,
        'M', UI_NPC_TILE_BASE + 0, 1, 1, "MAYOR", INTERACTION_DIALOGUE, 0, DIALOGUE_ID_MAYOR_GREETING, BATTLE_NONE, 0, 0, 0, 0, 0, 0
    },
    {
        0, ENTITY_ID_GUARD, 10, 8, DIRECTION_DOWN,
        ACTOR_FLAG_BLOCKING | ACTOR_FLAG_INTERACTABLE,
        'G', UI_NPC_TILE_BASE + 1, 1, 1, "GUARD", INTERACTION_DIALOGUE, 0, DIALOGUE_ID_GUARD_GREETING, BATTLE_NONE, 0, 0, 0, 0, 0, 0
    },
    {
        0, ENTITY_ID_SHOPKEEPER, 9, 3, DIRECTION_DOWN,
        ACTOR_FLAG_BLOCKING | ACTOR_FLAG_INTERACTABLE,
        'S', UI_NPC_TILE_BASE + 2, 1, 1, "SHOPKEEPER", INTERACTION_SHOP, 1, DIALOGUE_ID_NONE, BATTLE_NONE, 0, 0, 0, 0, 0, 0
    },
    /* Lost Merchant: quest dialogue is event-driven; his shop (id 2)
     * becomes reachable only after the amulet is returned.  Distinct 'E'
     * letter tile so the two 'M'-spawned NPC placeholders differ. */
    {
        0, ENTITY_ID_MERCHANT, 11, 3, DIRECTION_DOWN,
        ACTOR_FLAG_BLOCKING | ACTOR_FLAG_INTERACTABLE,
        'E', UI_NPC_TILE_BASE + 3, 1, 1, "MERCHANT", INTERACTION_SHOP, 2, DIALOGUE_ID_NONE, BATTLE_NONE, 0, 0, 0, 0, 0, 0
    }
};

static const WorldActorDefinition g_field_actors[] = {
    {
        1, ENTITY_ID_SLIME, 14, 8, DIRECTION_DOWN,
        ACTOR_FLAG_HOSTILE | ACTOR_FLAG_BLOCKING | ACTOR_FLAG_INTERACTABLE,
        'E', UI_ENEMY_SLIME_TILE, 1, 1, "SLIME", INTERACTION_COMBAT, 0, DIALOGUE_ID_NONE, BATTLE_SLIME, 5, 5, 5, CURRENCY_ID_GOLD, 0, 0
    }
};

static const WorldActorDefinition g_forest_actors[] = {
    {
        2, ENTITY_ID_SLIME, 10, 8, DIRECTION_DOWN,
        ACTOR_FLAG_HOSTILE | ACTOR_FLAG_BLOCKING | ACTOR_FLAG_INTERACTABLE,
        'E', UI_ENEMY_SLIME_TILE, 1, 1, "SLIME", INTERACTION_COMBAT, 0, DIALOGUE_ID_NONE, BATTLE_SLIME, 6, 6, 5, CURRENCY_ID_GOLD, 0, 0
    },
    {
        3, ENTITY_ID_BAT, 7, 4, DIRECTION_DOWN,
        ACTOR_FLAG_HOSTILE | ACTOR_FLAG_BLOCKING | ACTOR_FLAG_INTERACTABLE,
        'V', UI_ENEMY_BAT_TILE, 1, 1, "BAT", INTERACTION_COMBAT, 0, DIALOGUE_ID_NONE, BATTLE_BAT, 4, 4, 8, CURRENCY_ID_GOLD, 0, 0
    },
    /* The Lost Amulet: a quest-item pickup.  The AMULET_PICKUP event adds it
     * on first interact; afterwards the fallback dialogue plays. */
    {
        0, ENTITY_ID_AMULET, 16, 10, DIRECTION_DOWN,
        ACTOR_FLAG_BLOCKING | ACTOR_FLAG_INTERACTABLE,
        '?', UI_NPC_TILE_BASE + 4, 1, 1, "AMULET", INTERACTION_DIALOGUE, 0, DIALOGUE_ID_AMULET_NOTHING, BATTLE_NONE, 0, 0, 0, 0, 0, 0
    }
};

static const WorldActorDefinition g_mountain_pass_actors[] = {
    {
        4, ENTITY_ID_SLIME, 14, 7, DIRECTION_DOWN,
        ACTOR_FLAG_HOSTILE | ACTOR_FLAG_BLOCKING | ACTOR_FLAG_INTERACTABLE,
        'E', UI_ENEMY_SLIME_TILE, 1, 1, "SLIME", INTERACTION_COMBAT, 0, DIALOGUE_ID_NONE, BATTLE_SLIME, 8, 8, 5, CURRENCY_ID_GOLD, 0, 0
    }
};

static const WorldActorDefinition g_castle_actors[] = {
    {
        5, ENTITY_ID_BAT, 12, 7, DIRECTION_DOWN,
        ACTOR_FLAG_HOSTILE | ACTOR_FLAG_BLOCKING | ACTOR_FLAG_INTERACTABLE,
        'V', UI_ENEMY_BAT_TILE, 1, 1, "BAT", INTERACTION_COMBAT, 0, DIALOGUE_ID_NONE, BATTLE_BAT, 4, 4, 8, CURRENCY_ID_GOLD, 0, 0
    },
    /* Lord of Slimes: the final boss.  Appears only once the Monster Hunt
     * quest is COMPLETE (the sword is in the inventory).  Renders as a 2x2
     * tile block (the 16x16 sprite) whose footprint covers (10,5)-(11,6);
     * only the anchor cell (10,5) carries collision/encounter. */
    {
        6, ENTITY_ID_SLIME_LORD, 10, 5, DIRECTION_DOWN,
        ACTOR_FLAG_HOSTILE | ACTOR_FLAG_BLOCKING | ACTOR_FLAG_INTERACTABLE,
        'L', UI_ENEMY_BOSS_TILE_BASE, 2, 2, "LORD OF SLIMES", INTERACTION_COMBAT, 0, DIALOGUE_ID_NONE, BATTLE_NONE, 25, 25, 50, CURRENCY_ID_GOLD,
        VARIABLE_ID_QUEST_MONSTER_HUNT, 2
    }
};

static const WorldActorTable g_actor_tables[] = {
    { MAP_TOWN,          g_town_actors,
        (uint8_t)(sizeof(g_town_actors) / sizeof(g_town_actors[0])) },
    { MAP_FIELD,         g_field_actors,
        (uint8_t)(sizeof(g_field_actors) / sizeof(g_field_actors[0])) },
    { MAP_FOREST,        g_forest_actors,
        (uint8_t)(sizeof(g_forest_actors) / sizeof(g_forest_actors[0])) },
    { MAP_MOUNTAIN_PASS, g_mountain_pass_actors,
        (uint8_t)(sizeof(g_mountain_pass_actors) / sizeof(g_mountain_pass_actors[0])) },
    { MAP_CASTLE,        g_castle_actors,
        (uint8_t)(sizeof(g_castle_actors) / sizeof(g_castle_actors[0])) }
};

void game_actors_register(void)
{
    actor_register_tables(g_actor_tables,
                          (uint8_t)(sizeof(g_actor_tables) / sizeof(g_actor_tables[0])));
}

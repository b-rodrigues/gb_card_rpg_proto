#include "actor.h"
#include "game_ids.h"

/* ── Actor registration (game layer) ───────────────────────────────
 * The actor content tables live in a banked ROM region (actors_content.c,
 * `#pragma bank GAME_CONTENT_BANK`).  This file stays in the fixed bank
 * and registers the table + count + bank with the engine.
 */

extern const WorldActorTable g_actor_tables[];
#define GAME_ACTOR_TABLE_COUNT 5

void game_actors_register(void)
{
    actor_register_tables(g_actor_tables, GAME_ACTOR_TABLE_COUNT, GAME_CONTENT_BANK);
}

#include "scenarios.h"
#include "rng.h"
#include "telemetry.h"

void scenario_load(ScenarioId id, Game *g)
{
    if (!g) return;
    
    switch (id) {
        case SCENARIO_NEW_GAME:
            game_init(g);
            g->world.player.position.x = 4;
            g->world.player.position.y = 4;
            g->world.player.hp = 10;
            g->world.enemy.position.x = 14;
            g->world.enemy.position.y = 8;
            g->world.enemy.hp = 5;
            state_set(&g->state_machine, GAME_STATE_OVERWORLD);
            g->frame = 0;
            g->story_flags = 0;
            rng_set_seed(42);
            telemetry_init();
            telemetry_set_frame_ptr(&g->frame);
            break;
            
        case SCENARIO_FIRST_ENCOUNTER:
            game_init(g);
            g->world.player.position.x = 13;
            g->world.player.position.y = 8;
            g->world.player.hp = 10;
            g->world.enemy.position.x = 14;
            g->world.enemy.position.y = 8;
            g->world.enemy.hp = 5;
            g->world.encounter_triggered = false;
            state_set(&g->state_machine, GAME_STATE_OVERWORLD);
            rng_set_seed(12345);
            telemetry_init();
            telemetry_set_frame_ptr(&g->frame);
            break;
    }
}

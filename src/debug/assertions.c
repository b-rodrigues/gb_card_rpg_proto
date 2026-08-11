#include "assertions.h"

AssertResult assert_screen(const Game *g, ScreenId expected)
{
    return g && g->screen == expected ? ASSERT_PASS : ASSERT_FAIL;
}

AssertResult assert_scene(const Game *g, SceneId expected)
{
    return g && g->state.scene.scene_id == expected ? ASSERT_PASS : ASSERT_FAIL;
}

AssertResult assert_player_hp(const Game *g, uint8_t expected_hp)
{
    return g && g->world.player.hp == expected_hp ? ASSERT_PASS : ASSERT_FAIL;
}

AssertResult assert_player_position(const Game *g, uint8_t x, uint8_t y)
{
    return g && g->world.player.position.x == x && g->world.player.position.y == y ? ASSERT_PASS : ASSERT_FAIL;
}

AssertResult assert_enemy_hp(const Game *g, uint8_t expected_hp)
{
    uint8_t i;
    if (!g) return ASSERT_FAIL;
    for (i = 0; i < MAX_WORLD_ACTORS; i++) {
        if (g->world.actors[i].active) {
            return g->world.actors[i].hp == expected_hp ? ASSERT_PASS : ASSERT_FAIL;
        }
    }
    return ASSERT_FAIL;
}

AssertResult assert_enemy_active(const Game *g, bool expected)
{
    uint8_t i;
    if (!g) return ASSERT_FAIL;
    for (i = 0; i < MAX_WORLD_ACTORS; i++) {
        if (g->world.actors[i].active) {
            return expected ? ASSERT_PASS : ASSERT_FAIL;
        }
    }
    return expected ? ASSERT_FAIL : ASSERT_PASS;
}

AssertResult assert_event_occurred(GameEventType type)
{
    uint8_t i;
    const GameEvent *events = telemetry_get_events();
    uint8_t count = telemetry_get_count();
    
    for (i = 0; i < count; i++) {
        if (events[i].type == type) {
            return ASSERT_PASS;
        }
    }
    return ASSERT_FAIL;
}

AssertResult assert_event_not_occurred(GameEventType type)
{
    return assert_event_occurred(type) == ASSERT_PASS ? ASSERT_FAIL : ASSERT_PASS;
}

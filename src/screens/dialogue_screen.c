#include "game.h"
#include "screen.h"
#include "dialogue.h"
#include "interaction.h"
#include "telemetry.h"

void dialogue_screen_update(Game *g)
{
    if (!g || !g->dialogue.active) return;

    if (input_pressed(INPUT_A) || input_pressed(INPUT_START)) {
        if (!dialogue_next(&g->dialogue)) {
            interaction_on_dialogue_end(&g->dialogue, &g->state);
            screen_change(g, SCREEN_OVERWORLD);
        }
    }
}

void dialogue_screen_render(Game *g)
{
    RenderCache *rc;

    if (!g) return;
    rc = &g->render_cache;

    /* On first entering dialogue, draw the world behind the box, then the
     * box.  Dialogues start from the overworld, whose last full redraw
     * already left the map on the display (rc->prev_screen ==
     * SCREEN_OVERWORLD after game_render_reset), so redrawing it here
     * would be a needless full-screen wipe; the box simply draws over the
     * HUD region.  Only a dialogue that is not preceded by the overworld
     * (scenario boot, forced via render_cache in scenarios.c) establishes
     * the world itself. */
    if (!rc->valid || rc->prev_screen != SCREEN_DIALOGUE) {
        if (rc->prev_screen != SCREEN_OVERWORLD) {
            ui_draw_world_full(&g->world);
        }
        ui_draw_dialogue(&g->dialogue);
        telemetry_emit(EVENT_RENDER_DIALOGUE, (uint8_t)g->dialogue.id,
                       g->dialogue.current_line, 0, 0);
        rc->valid = true;
        rc->prev_screen = SCREEN_DIALOGUE;
        rc->prev_dialogue_active = true;
        rc->prev_dialogue_line = g->dialogue.current_line;
        rc->prev_dialogue_id = g->dialogue.id;
        return;
    }

    /* Dialogue line or id changed: redraw the box with LCD off so window
     * tilemap writes (0x9C00) land in VRAM bank 0 during VBlank without
     * being silently dropped by the PPU in scanout modes 2/3. */
    if (g->dialogue.current_line != rc->prev_dialogue_line ||
        g->dialogue.id != rc->prev_dialogue_id) {
        ui_lcd_off();
        ui_draw_dialogue(&g->dialogue);
        ui_sprite_commit();
        ui_lcd_on();
        telemetry_emit(EVENT_RENDER_DIALOGUE, (uint8_t)g->dialogue.id,
                       g->dialogue.current_line, 0, 0);
        rc->prev_dialogue_line = g->dialogue.current_line;
        rc->prev_dialogue_id = g->dialogue.id;
    }
}

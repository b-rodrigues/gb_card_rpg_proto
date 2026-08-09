#include "game.h"
#include "sound.h"
#include <stdlib.h>

Game g_game;

/* Simple pseudo-random number generator for Game Boy */
static uint8_t next_random_piece(void) {
    g_game.random_seed = (g_game.random_seed * 109 + 89) % 251;
    return g_game.random_seed % NUM_PIECE_TYPES;
}

void game_init(void) {
    g_game.state = GAME_STATE_TITLE;
    g_game.menu_index = 0;
    g_game.random_seed = 42;
    render_title_screen(g_game.menu_index);
}

void game_start_play(void) {
    render_playfield_layout();
    board_init();
    g_game.state = GAME_STATE_PLAYING;
    g_game.score = 0;
    g_game.lines = 0;
    g_game.fall_timer = 0;
    g_game.fall_speed = 30; /* Fall every 30 frames (~0.5s) */

    g_game.hold_type = NO_PIECE;
    g_game.can_hold = 1;

    g_game.next_type = next_random_piece();
    piece_spawn(&g_game.current_piece, next_random_piece());

    render_board();
    render_hold_piece(g_game.hold_type);
    render_next_piece(g_game.next_type);
}

static void try_move_piece(int8_t dx, int8_t dy) {
    Piece test;
    piece_copy(&test, &g_game.current_piece);
    test.x += dx;
    test.y += dy;
    if (!piece_collides(&test)) {
        render_clear_piece(&g_game.current_piece);
        piece_copy(&g_game.current_piece, &test);
        sound_play_move();
    }
}

static void try_rotate_piece(int8_t dir) {
    Piece test;
    piece_copy(&test, &g_game.current_piece);
    test.rotation = (test.rotation + dir + 4) % 4;
    if (!piece_collides(&test)) {
        render_clear_piece(&g_game.current_piece);
        piece_copy(&g_game.current_piece, &test);
        sound_play_move();
    }
}

static void try_hold_piece(void) {
    if (!g_game.can_hold) return;

    render_clear_piece(&g_game.current_piece);

    if (g_game.hold_type == NO_PIECE) {
        g_game.hold_type = g_game.current_piece.type;
        piece_spawn(&g_game.current_piece, g_game.next_type);
        g_game.next_type = next_random_piece();
        render_next_piece(g_game.next_type);
    } else {
        uint8_t temp = g_game.current_piece.type;
        piece_spawn(&g_game.current_piece, g_game.hold_type);
        g_game.hold_type = temp;
    }

    g_game.can_hold = 0;
    render_hold_piece(g_game.hold_type);
    sound_play_move();

    if (piece_collides(&g_game.current_piece)) {
        g_game.state = GAME_STATE_GAME_OVER;
    }
}

static void lock_and_spawn(void) {
    uint8_t cleared;
    piece_lock(&g_game.current_piece);
    sound_play_drop();
    cleared = board_check_and_clear_lines();
    if (cleared > 0) {
        sound_play_clear();
        g_game.lines += cleared;
        g_game.score += cleared * 100;
        /* Increase fall speed slightly as score increases */
        if (g_game.fall_speed > 10) {
            g_game.fall_speed--;
        }
    }
    render_board();

    /* Re-enable hold feature for the next falling piece */
    g_game.can_hold = 1;

    /* Spawn new piece using next_type, then generate new next_type */
    piece_spawn(&g_game.current_piece, g_game.next_type);
    g_game.next_type = next_random_piece();
    render_next_piece(g_game.next_type);

    if (piece_collides(&g_game.current_piece)) {
        g_game.state = GAME_STATE_GAME_OVER;
    }
}

static void hard_drop_piece(void) {
    uint8_t drop_distance = 0;
    Piece test;
    piece_copy(&test, &g_game.current_piece);

    while (1) {
        test.y += 1;
        if (piece_collides(&test)) {
            break;
        }
        drop_distance++;
    }

    if (drop_distance > 0) {
        render_clear_piece(&g_game.current_piece);
        g_game.current_piece.y += drop_distance;
        g_game.score += drop_distance;
    }

    lock_and_spawn();
}

void game_update(void) {
    g_game.random_seed++;

    if (g_game.state == GAME_STATE_TITLE) {
        if (input_is_pressed(J_UP) || input_is_pressed(J_DOWN)) {
            g_game.menu_index ^= 1;
            sound_play_move();
            render_title_menu_update(g_game.menu_index);
            return;
        }
        if (input_is_pressed(J_A) || input_is_pressed(J_START)) {
            sound_play_move();
            if (g_game.menu_index == 0) {
                game_start_play();
            } else {
                g_game.state = GAME_STATE_OPTIONS;
                render_options_screen(sound_get_music_enabled());
            }
            return;
        }
        return;
    }

    if (g_game.state == GAME_STATE_OPTIONS) {
        if (input_is_pressed(J_UP) || input_is_pressed(J_DOWN) || 
            input_is_pressed(J_LEFT) || input_is_pressed(J_RIGHT) || 
            input_is_pressed(J_A)) {
            sound_set_music_enabled(!sound_get_music_enabled());
            sound_play_move();
            render_options_screen(sound_get_music_enabled());
            return;
        }
        if (input_is_pressed(J_B) || input_is_pressed(J_START)) {
            sound_play_move();
            g_game.state = GAME_STATE_TITLE;
            render_title_screen(g_game.menu_index);
            return;
        }
        return;
    }

    if (g_game.state == GAME_STATE_GAME_OVER) {
        if (input_is_pressed(J_START) || input_is_pressed(J_SELECT) || input_is_pressed(J_A)) {
            g_game.state = GAME_STATE_TITLE;
            render_title_screen(g_game.menu_index);
        }
        return;
    }

    if (g_game.state == GAME_STATE_PAUSED) {
        /* Press Start to resume */
        if (input_is_pressed(J_START)) {
            g_game.state = GAME_STATE_PLAYING;
            render_board();
            return;
        }
        /* Press Select to restart */
        if (input_is_pressed(J_SELECT)) {
            g_game.state = GAME_STATE_TITLE;
            render_title_screen(g_game.menu_index);
            return;
        }
        return;
    }

    /* Start button pauses the game */
    if (input_is_pressed(J_START)) {
        g_game.state = GAME_STATE_PAUSED;
        return;
    }

    /* Select button holds/swaps the current piece */
    if (input_is_pressed(J_SELECT)) {
        try_hold_piece();
        return;
    }

    /* Up button triggers hard drop */
    if (input_is_pressed(J_UP)) {
        hard_drop_piece();
        return;
    }

    /* Process player input */
    if (input_is_pressed(J_LEFT)) {
        try_move_piece(-1, 0);
    }
    if (input_is_pressed(J_RIGHT)) {
        try_move_piece(1, 0);
    }
    if (input_is_pressed(J_A)) {
        try_rotate_piece(1);
    }
    if (input_is_pressed(J_B)) {
        try_rotate_piece(-1);
    }

    /* Soft drop on Down press or hold */
    if (input_is_pressed(J_DOWN) || (input_is_down(J_DOWN) && (g_game.fall_timer % 3 == 0))) {
        Piece test;
        piece_copy(&test, &g_game.current_piece);
        test.y += 1;
        if (!piece_collides(&test)) {
            render_clear_piece(&g_game.current_piece);
            piece_copy(&g_game.current_piece, &test);
            g_game.fall_timer = 0;
        } else {
            lock_and_spawn();
            return;
        }
    }

    /* Automatic falling */
    g_game.fall_timer++;
    if (g_game.fall_timer >= g_game.fall_speed) {
        Piece test;
        piece_copy(&test, &g_game.current_piece);
        g_game.fall_timer = 0;
        test.y += 1;
        if (!piece_collides(&test)) {
            render_clear_piece(&g_game.current_piece);
            piece_copy(&g_game.current_piece, &test);
        } else {
            lock_and_spawn();
        }
    }
}

void game_render(void) {
    if (g_game.state == GAME_STATE_PLAYING) {
        render_piece(&g_game.current_piece);
    }
    render_ui(g_game.score, g_game.lines, g_game.state);
}

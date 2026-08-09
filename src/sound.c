#include "sound.h"
#include <gb/gb.h>

#define NOTE_C4   1547
#define NOTE_D4   1602
#define NOTE_E4   1650
#define NOTE_F4   1673
#define NOTE_G4   1714
#define NOTE_A4   1750
#define NOTE_B4   1783
#define NOTE_C5   1798
#define NOTE_REST 0

typedef struct {
    uint16_t pitch;
    uint8_t duration;
} Note;

/* "Ah! Les crocodiles" melody notes */
static const Note CROCODILE_TUNE[] = {
    /* "Un cro-co-dile, en par-tant pour la guerre" */
    {NOTE_C4, 15}, {NOTE_C4, 15}, {NOTE_C4, 15}, {NOTE_C4, 15},
    {NOTE_D4, 15}, {NOTE_E4, 15}, {NOTE_E4, 15}, {NOTE_E4, 15},
    {NOTE_F4, 15}, {NOTE_G4, 15}, {NOTE_G4, 15}, {NOTE_G4, 15},
    {NOTE_G4, 15}, {NOTE_F4, 15}, {NOTE_E4, 15}, {NOTE_D4, 15},

    /* "Di-sait au re-voir à ses pe-tits en-fants" */
    {NOTE_E4, 15}, {NOTE_F4, 15}, {NOTE_E4, 15}, {NOTE_D4, 15},
    {NOTE_C4, 30}, {NOTE_REST, 15},

    /* "Ah! les cro-co, les cro-co, les cro-co-diles" */
    {NOTE_G4, 15}, {NOTE_C4, 15}, {NOTE_C4, 15}, {NOTE_C4, 15},
    {NOTE_E4, 15}, {NOTE_E4, 15}, {NOTE_E4, 15}, {NOTE_G4, 15},
    {NOTE_G4, 15}, {NOTE_G4, 15},

    /* "Sur les bords du Nil, ils ont dis-pa-ru, n'en par-lons plus!" */
    {NOTE_G4, 15}, {NOTE_F4, 15}, {NOTE_E4, 15}, {NOTE_D4, 15},
    {NOTE_E4, 15}, {NOTE_F4, 15}, {NOTE_E4, 15}, {NOTE_D4, 15},
    {NOTE_C4, 45}, {NOTE_REST, 30}
};

#define TUNE_LENGTH 40

static uint8_t current_note_idx = 0;
static uint8_t note_timer = 0;
static uint8_t music_enabled = 1;

void sound_init(void) {
    NR52_REG = 0x80; /* Enable Master Sound */
    NR50_REG = 0x77; /* Set Master Volume Left & Right to max */
    NR51_REG = 0xFF; /* Enable all 4 channels to Left & Right speakers */

    current_note_idx = 0;
    note_timer = 0;
    music_enabled = 1;

    CRITICAL {
        add_VBL(sound_update);
    }
}

void sound_set_music_enabled(uint8_t enabled) {
    music_enabled = enabled;
    if (!music_enabled) {
        NR22_REG = 0x00; /* Mute channel 2 immediately */
        NR24_REG = 0x80;
    }
}

uint8_t sound_get_music_enabled(void) {
    return music_enabled;
}

void sound_update(void) {
    Note n;
    if (!music_enabled) {
        return;
    }

    if (note_timer == 0) {
        n = CROCODILE_TUNE[current_note_idx];
        if (n.pitch == NOTE_REST) {
            NR22_REG = 0x00; /* Mute channel 2 */
            NR24_REG = 0x80;
        } else {
            NR21_REG = 0x80; /* 50% duty cycle */
            NR22_REG = 0x83; /* Volume 8, envelope fade out */
            NR23_REG = (uint8_t)(n.pitch & 0xFF);
            NR24_REG = 0x80 | (uint8_t)((n.pitch >> 8) & 0x07); /* Trigger note */
        }
        note_timer = n.duration;
        current_note_idx = (current_note_idx + 1) % TUNE_LENGTH;
    } else {
        note_timer--;
    }
}

void sound_play_move(void) {
    NR10_REG = 0x00;
    NR11_REG = 0x40;
    NR12_REG = 0x41;
    NR13_REG = 0x00;
    NR14_REG = 0x87; /* Quick high pulse beep on Channel 1 */
}

void sound_play_drop(void) {
    NR10_REG = 0x00;
    NR11_REG = 0x80;
    NR12_REG = 0xA1;
    NR13_REG = 0x20;
    NR14_REG = 0x85; /* Low thud on Channel 1 */
}

void sound_play_clear(void) {
    NR10_REG = 0x16; /* Pitch sweep up */
    NR11_REG = 0x80;
    NR12_REG = 0xF3;
    NR13_REG = 0x50;
    NR14_REG = 0x86; /* Fanfare sweep on Channel 1 */
}

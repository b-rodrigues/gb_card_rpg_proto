#include "audio.h"

static MusicTrack current_track = MUSIC_NONE;
static uint8_t step_counter = 0;
static uint8_t note_index = 0;

/* Note pitch frequency lookup table for GB Channel 1/2 */
#define NOTE_C4  0x02C0
#define NOTE_D4  0x0370
#define NOTE_EB4 0x03C0
#define NOTE_E4  0x0400
#define NOTE_F4  0x0440
#define NOTE_FS4 0x0480
#define NOTE_G4  0x04C0
#define NOTE_A4  0x0540
#define NOTE_B4  0x05C0
#define NOTE_C5  0x0600
#define NOTE_D5  0x0660
#define NOTE_EB5 0x0680
#define NOTE_E5  0x06A0

/* Overworld track: 16-note loop */
static const uint16_t overworld_notes[16] = {
    NOTE_C4, NOTE_E4, NOTE_G4, NOTE_C5,
    NOTE_G4, NOTE_E4, NOTE_C4, NOTE_G4,
    NOTE_D4, NOTE_F4, NOTE_A4, NOTE_D5,
    NOTE_A4, NOTE_F4, NOTE_D4, NOTE_G4
};

/* Battle track: 16-note fast aggressive loop */
static const uint16_t battle_notes[16] = {
    NOTE_C4, NOTE_EB4, NOTE_G4, NOTE_C4,
    NOTE_FS4, NOTE_G4, NOTE_FS4, NOTE_EB4,
    NOTE_C4, NOTE_EB4, NOTE_G4, NOTE_FS4,
    NOTE_C5, NOTE_FS4, NOTE_EB4, NOTE_FS4
};

static void play_channel1_note(uint16_t freq)
{
    if (freq == 0) return;
    NR10_REG = 0x00;           /* Sweep off */
    NR11_REG = 0x80;           /* Duty cycle 50% */
    NR12_REG = 0x92;           /* Volume 9, envelope decay 2 */
    NR13_REG = (uint8_t)(freq & 0xFF);
    NR14_REG = 0x80 | (uint8_t)((freq >> 8) & 0x07); /* Trigger note */
}

void audio_init(void)
{
    NR50_REG = 0x77;           /* Master volume max */
    NR51_REG = 0xFF;           /* Route to stereo */
    NR52_REG = 0x80;           /* APU ON */
    current_track = MUSIC_NONE;
    step_counter = 0;
    note_index = 0;
}

void audio_play_music(MusicTrack track)
{
    if (current_track == track) return;
    current_track = track;
    step_counter = 0;
    note_index = 0;
}

void audio_stop_music(void)
{
    current_track = MUSIC_NONE;
    NR12_REG = 0x00; /* Mute CH1 */
}

void audio_update(void)
{
    uint8_t tempo_speed;
    uint16_t freq;

    if (current_track == MUSIC_NONE) return;

    tempo_speed = (current_track == MUSIC_BATTLE) ? 6 : 12;

    step_counter++;
    if (step_counter >= tempo_speed) {
        step_counter = 0;

        if (current_track == MUSIC_OVERWORLD) {
            freq = overworld_notes[note_index & 0x0F];
        } else {
            freq = battle_notes[note_index & 0x0F];
        }

        play_channel1_note(freq);
        note_index++;
    }
}

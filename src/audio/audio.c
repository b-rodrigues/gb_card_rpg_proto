#include "audio.h"

static MusicTrack current_track = MUSIC_NONE;
static uint8_t step_counter = 0;
static uint8_t note_index = 0;

/* Standard 11-bit Game Boy APU Pitch Frequencies: f_gb = 2048 - (131072 / hz) */
#define REST     0x0000

#define NOTE_G3  0x0540
#define NOTE_A3  0x05D6
#define NOTE_AS3 0x0618
#define NOTE_B3  0x0654

#define NOTE_C4  0x060B
#define NOTE_CS4 0x0627
#define NOTE_D4  0x0641
#define NOTE_DS4 0x065A
#define NOTE_E4  0x0672
#define NOTE_F4  0x0688
#define NOTE_FS4 0x069D
#define NOTE_G4  0x06B1
#define NOTE_GS4 0x06C4
#define NOTE_A4  0x06D6
#define NOTE_AS4 0x06E6
#define NOTE_B4  0x06F6

#define NOTE_C5  0x0705
#define NOTE_CS5 0x0713
#define NOTE_D5  0x0720
#define NOTE_DS5 0x072D
#define NOTE_E5  0x0739
#define NOTE_F5  0x0744
#define NOTE_FS5 0x074E
#define NOTE_G5  0x0758
#define NOTE_GS5 0x0762
#define NOTE_A5  0x076B
#define NOTE_AS5 0x0773
#define NOTE_B5  0x077B
#define NOTE_C6  0x0782

/* Overworld Track: Mozart's "Lacrimosa" (Requiem in D Minor, K. 626) */
static const uint16_t lacrimosa_notes[32] = {
    NOTE_D4,  NOTE_CS4, NOTE_D4,  NOTE_E4,
    NOTE_F4,  NOTE_G4,  NOTE_A4,  NOTE_AS4,
    NOTE_A4,  NOTE_G4,  NOTE_F4,  NOTE_E4,
    NOTE_D4,  NOTE_CS4, NOTE_D4,  REST,

    NOTE_F4,  NOTE_E4,  NOTE_F4,  NOTE_G4,
    NOTE_A4,  NOTE_AS4, NOTE_C5,  NOTE_D5,
    NOTE_C5,  NOTE_AS4, NOTE_A4,  NOTE_G4,
    NOTE_F4,  NOTE_E4,  NOTE_D4,  REST
};

/* Battle Track: Vivaldi's "The Four Seasons - Summer" (Presto, 3rd Movement) */
static const uint16_t summer_notes[32] = {
    NOTE_G5,  NOTE_D5,  NOTE_AS4, NOTE_G4,
    NOTE_G5,  NOTE_D5,  NOTE_AS4, NOTE_G4,
    NOTE_FS5, NOTE_D5,  NOTE_A4,  NOTE_FS4,
    NOTE_FS5, NOTE_D5,  NOTE_A4,  NOTE_FS4,

    NOTE_G5,  NOTE_G5,  NOTE_G5,  NOTE_AS5,
    NOTE_AS5, NOTE_AS5, NOTE_A5,  NOTE_A5,
    NOTE_A5,  NOTE_FS5, NOTE_FS5, NOTE_FS5,
    NOTE_G5,  NOTE_D5,  NOTE_AS4, NOTE_G4
};

static void play_channel1_note(uint16_t freq)
{
    if (freq == REST) {
        NR12_REG = 0x00; /* Mute volume */
        NR14_REG = 0x80; /* Trigger note */
        return;
    }
    NR10_REG = 0x00;                                /* Sweep off */
    NR11_REG = 0x80;                                /* Duty cycle 50% */
    NR12_REG = 0xF2;                                /* Volume 15, envelope decay 2 */
    NR13_REG = (uint8_t)(freq & 0xFF);              /* Low frequency byte */
    NR14_REG = 0x80 | (uint8_t)((freq >> 8) & 0x07);/* Trigger note */
}

void audio_init(void)
{
    NR52_REG = 0x80;           /* Master APU ON */
    NR50_REG = 0x77;           /* Volume Max (7 left, 7 right) */
    NR51_REG = 0xFF;           /* Route all sound channels to stereo */
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
    NR12_REG = 0x00;
}

void audio_update(void)
{
    uint8_t tempo_speed;
    uint16_t freq;

    if (current_track == MUSIC_NONE) return;

    /* Overworld (Lacrimosa): ~12 VBlanks per note (solemn legato tempo) */
    /* Battle (Summer Presto): ~3 VBlanks per note (furious presto tempo) */
    tempo_speed = (current_track == MUSIC_BATTLE) ? 3 : 12;

    step_counter++;
    if (step_counter >= tempo_speed) {
        step_counter = 0;

        if (current_track == MUSIC_OVERWORLD) {
            freq = lacrimosa_notes[note_index & 0x1F];
        } else {
            freq = summer_notes[note_index & 0x1F];
        }

        play_channel1_note(freq);
        note_index++;
    }
}

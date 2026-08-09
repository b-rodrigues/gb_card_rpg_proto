#include "audio.h"

static MusicTrack current_track = MUSIC_NONE;
static uint8_t step_counter = 0;
static uint8_t note_index = 0;

/*
 * Game Boy APU 11-bit frequency register values.
 * Formula: register = 2048 - round(131072 / freq_hz)
 * The APU plays: freq_hz = 131072 / (2048 - register)
 * Values verified by computation script.
 */
#define REST     0x0000

/* Octave 3 */
#define NOTE_A3   0x05AC  /* 220.0 Hz */
#define NOTE_AS3  0x05CE  /* 233.1 Hz */
#define NOTE_B3   0x05ED  /* 246.9 Hz */

/* Octave 4 */
#define NOTE_C4   0x060B  /* 261.6 Hz */
#define NOTE_CS4  0x0627  /* 277.2 Hz */
#define NOTE_D4   0x0642  /* 293.7 Hz */
#define NOTE_DS4  0x065B  /* 311.1 Hz */
#define NOTE_E4   0x0672  /* 329.6 Hz */
#define NOTE_F4   0x0689  /* 349.2 Hz */
#define NOTE_FS4  0x069E  /* 370.0 Hz */
#define NOTE_G4   0x06B2  /* 392.0 Hz */
#define NOTE_GS4  0x06C4  /* 415.3 Hz */
#define NOTE_A4   0x06D6  /* 440.0 Hz */
#define NOTE_AS4  0x06E7  /* 466.2 Hz */
#define NOTE_B4   0x06F7  /* 493.9 Hz */

/* Octave 5 */
#define NOTE_C5   0x0706  /* 523.2 Hz */
#define NOTE_CS5  0x0714  /* 554.4 Hz */
#define NOTE_D5   0x0721  /* 587.3 Hz */
#define NOTE_DS5  0x072D  /* 622.2 Hz */
#define NOTE_E5   0x0739  /* 659.3 Hz */
#define NOTE_F5   0x0744  /* 698.5 Hz */
#define NOTE_FS5  0x074F  /* 740.0 Hz */
#define NOTE_G5   0x0759  /* 784.0 Hz */
#define NOTE_GS5  0x0762  /* 830.6 Hz */
#define NOTE_A5   0x076B  /* 880.0 Hz */
#define NOTE_AS5  0x0773  /* 932.3 Hz */
#define NOTE_B5   0x077B  /* 987.8 Hz */

/* Octave 6 */
#define NOTE_C6   0x0783  /* 1046.5 Hz */

/*
 * Overworld: Mozart's "Lacrimosa" (Requiem K.626)
 * D minor, solemn descending theme, 32-note loop
 */
static const uint16_t lacrimosa_notes[32] = {
    /* Bars 1-2: Opening theme D-C#-D ascending to F */
    NOTE_D4,  NOTE_D4,  NOTE_CS4, NOTE_D4,
    NOTE_E4,  NOTE_F4,  NOTE_F4,  REST,
    /* Bars 3-4: Ascending cry A-Bb-A resolving G */
    NOTE_A4,  NOTE_A4,  NOTE_AS4, NOTE_A4,
    NOTE_G4,  NOTE_F4,  NOTE_E4,  REST,
    /* Bars 5-6: Repeat opening motif higher */
    NOTE_F4,  NOTE_F4,  NOTE_E4,  NOTE_F4,
    NOTE_G4,  NOTE_A4,  NOTE_A4,  REST,
    /* Bars 7-8: Descending resolution to D */
    NOTE_G4,  NOTE_F4,  NOTE_E4,  NOTE_F4,
    NOTE_E4,  NOTE_D4,  NOTE_D4,  REST
};

/*
 * Battle: Vivaldi's "Summer" Presto (Four Seasons, RV 315 3rd mvt.)
 * G minor, furious repeated-note storm motif, 32-note loop
 */
static const uint16_t summer_notes[32] = {
    /* Bars 1-2: Hammered G minor arpeggio */
    NOTE_G4,  NOTE_G4,  NOTE_AS4, NOTE_AS4,
    NOTE_D5,  NOTE_D5,  NOTE_G5,  NOTE_G5,
    /* Bars 3-4: Descending run */
    NOTE_G5,  NOTE_FS5, NOTE_E5,  NOTE_D5,
    NOTE_CS5, NOTE_D5,  NOTE_A4,  NOTE_A4,
    /* Bars 5-6: Repeated staccato figure */
    NOTE_AS4, NOTE_AS4, NOTE_A4,  NOTE_A4,
    NOTE_G4,  NOTE_G4,  NOTE_FS4, NOTE_FS4,
    /* Bars 7-8: Rising fury back to top */
    NOTE_G4,  NOTE_A4,  NOTE_AS4, NOTE_D5,
    NOTE_E5,  NOTE_FS5, NOTE_G5,  REST
};

static void play_note(uint16_t freq)
{
    if (freq == REST) {
        /* Silence: zero the volume envelope, retrigger to apply */
        NR12_REG = 0x00;
        NR14_REG = 0x80;
        return;
    }
    NR10_REG = 0x00;                                 /* No sweep */
    NR11_REG = 0x80;                                 /* 50% duty cycle */
    NR12_REG = 0xF1;                                 /* Vol 15, decay pace 1 */
    NR13_REG = (uint8_t)(freq & 0xFF);               /* Freq low 8 bits */
    NR14_REG = 0x80 | (uint8_t)((freq >> 8) & 0x07); /* Trigger + freq high 3 bits */
}

void audio_init(void)
{
    NR52_REG = 0x80;  /* APU master enable (MUST be first) */
    NR50_REG = 0x77;  /* Master volume max L+R */
    NR51_REG = 0xFF;  /* Route all channels to both speakers */
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
    NR14_REG = 0x80;
}

void audio_update(void)
{
    uint8_t tempo;
    uint16_t freq;

    if (current_track == MUSIC_NONE) return;

    /*
     * Tempo in VBlank ticks per note step.
     * VBlank fires at ~59.73 Hz.
     * Lacrimosa: 10 ticks/note -> ~6 notes/sec (solemn, legato)
     * Summer Presto: 4 ticks/note -> ~15 notes/sec (furious)
     */
    tempo = (current_track == MUSIC_BATTLE) ? 4 : 10;

    step_counter++;
    if (step_counter >= tempo) {
        step_counter = 0;

        if (current_track == MUSIC_OVERWORLD) {
            freq = lacrimosa_notes[note_index & 0x1F];
        } else {
            freq = summer_notes[note_index & 0x1F];
        }

        play_note(freq);
        note_index++;
    }
}

MusicTrack audio_get_current_track(void)
{
    return current_track;
}

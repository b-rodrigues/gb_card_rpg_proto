#ifndef AUDIO_H
#define AUDIO_H

#include <gb/gb.h>
#include <stdint.h>

typedef enum {
    MUSIC_NONE,
    MUSIC_OVERWORLD,
    MUSIC_BATTLE
} MusicTrack;

void audio_init(void);
void audio_play_music(MusicTrack track);
void audio_update(void);

MusicTrack audio_get_current_track(void);

#ifdef DEBUG_BUILD
/* Per-music-clock-tick counter, incremented once per audio_update() call
 * (i.e. once per ISR tick).  Host-side tools (tools/verify_music.py) sample
 * it every frame to assert the music clock never stalls during LCD-off
 * screen/map transitions (see AGENTS.md Music contract).  Not present in
 * release builds. */
extern volatile uint16_t g_audio_ticks;
#endif

#endif /* AUDIO_H */

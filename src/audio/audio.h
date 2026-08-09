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
void audio_stop_music(void);
void audio_update(void);

MusicTrack audio_get_current_track(void);

#endif /* AUDIO_H */

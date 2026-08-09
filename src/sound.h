#ifndef SOUND_H
#define SOUND_H

#include <stdint.h>

void sound_init(void);
void sound_update(void);
void sound_set_music_enabled(uint8_t enabled);
uint8_t sound_get_music_enabled(void);
void sound_play_move(void);
void sound_play_drop(void);
void sound_play_clear(void);

#endif /* SOUND_H */

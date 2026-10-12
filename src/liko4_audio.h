#pragma once
#include <stdbool.h>

void liko4_audio_init(const char *data_dir);      // грузит все N.wav / N.ogg
void liko4_audio_close(void);

// SFX
void sfx(int index);
void sfx_ex(int index, float volume);
void sfx_stop_all(void);

// Music
void music(int index);
void music_stop(void);
void music_pause(void);
void music_resume(void);
void music_volume(float v);
void music_update(void);

// Tone synthesis (WASM-4 style, 4 channels)
//   frequency: Hz
//   duration:  in frames (60 per second)
//   volume:    0-100
//   flags:     channel + waveform (low 2 bits)
//              0 = pulse 12.5%
//              1 = pulse 25%
//              2 = triangle
//              3 = noise
void tone(int frequency, int duration, int volume, int flags);
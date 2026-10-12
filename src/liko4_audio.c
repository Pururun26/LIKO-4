#include "liko4_audio.h"
#include "liko4_conf.h"
#include "raylib.h"

#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

// ============================================================
// Состояние
// ============================================================

// SFX
static Sound sfx_slots[LIKO4_MAX_SFX];
static bool  sfx_used [LIKO4_MAX_SFX];

// Music
static Music music_slots[LIKO4_MAX_MUSIC];
static bool  music_used [LIKO4_MAX_MUSIC];
static int   current_music = -1;

// Tone channels (WASM-4 style)
#define TONE_SAMPLE_RATE 44100
#define TONE_CHANNELS    4

typedef struct {
    Sound sound;
    bool  used;
} ToneChannel;

static ToneChannel tone_channels[TONE_CHANNELS] = {0};
static bool        tone_seeded = false;

// ============================================================
// Инициализация
// ============================================================

void liko4_audio_init(const char *data_dir) {
    InitAudioDevice();
    if (!IsAudioDeviceReady()) {
        TraceLog(LOG_ERROR, "LIKO-4: Audio device failed to initialize!");
        return;
    }

    // SFX: 0.wav, 1.wav, 2.wav, ...
    for (int i = 0; i < LIKO4_MAX_SFX; i++) {
        char path[1024];
        snprintf(path, sizeof(path), "%s%s%d.wav",
                 data_dir, LIKO4_SOUNDS_PATH, i);

        if (!FileExists(path)) continue;

        Sound s = LoadSound(path);
        if (s.frameCount == 0) continue;

        sfx_slots[i] = s;
        sfx_used[i]  = true;
    }

    // Music: 0.ogg, 1.ogg, ...
    for (int i = 0; i < LIKO4_MAX_MUSIC; i++) {
        char path[1024];
        snprintf(path, sizeof(path), "%s%s%d.ogg",
                 data_dir, LIKO4_MUSIC_PATH, i);

        if (!FileExists(path)) continue;

        Music m = LoadMusicStream(path);
        if (m.ctxData == NULL) continue;

        music_slots[i] = m;
        music_used[i]  = true;
    }

    TraceLog(LOG_INFO, "LIKO-4: Audio ready.");
}

void liko4_audio_close(void) {
    if (!IsAudioDeviceReady()) return;

    // Tone channels
    for (int i = 0; i < TONE_CHANNELS; i++) {
        if (tone_channels[i].used) {
            StopSound(tone_channels[i].sound);
            UnloadSound(tone_channels[i].sound);
            tone_channels[i].used = false;
        }
    }

    // SFX
    for (int i = 0; i < LIKO4_MAX_SFX; i++) {
        if (sfx_used[i]) UnloadSound(sfx_slots[i]);
    }

    // Music
    for (int i = 0; i < LIKO4_MAX_MUSIC; i++) {
        if (music_used[i]) UnloadMusicStream(music_slots[i]);
    }

    CloseAudioDevice();
}

// ============================================================
// SFX
// ============================================================

void sfx_ex(int index, float volume) {
    if (index < 0 || index >= LIKO4_MAX_SFX) return;
    if (!sfx_used[index]) return;
    SetSoundVolume(sfx_slots[index], volume);
    PlaySound(sfx_slots[index]);
}

void sfx(int index) {
    sfx_ex(index, 1.0f);
}

void sfx_stop_all(void) {
    for (int i = 0; i < LIKO4_MAX_SFX; i++) {
        if (sfx_used[i]) StopSound(sfx_slots[i]);
    }
}

// ============================================================
// Music
// ============================================================

void music(int index) {
    if (index < 0 || index >= LIKO4_MAX_MUSIC) return;
    if (!music_used[index]) return;

    // Уже играет этот трек — ничего не делаем
    if (current_music == index) return;

    // Останавливаем текущую, если была
    if (current_music >= 0) {
        StopMusicStream(music_slots[current_music]);
    }

    current_music = index;
    music_slots[index].looping = true;
    PlayMusicStream(music_slots[index]);
}

void music_stop(void) {
    if (current_music < 0) return;
    StopMusicStream(music_slots[current_music]);
    current_music = -1;
}

void music_pause(void) {
    if (current_music < 0) return;
    PauseMusicStream(music_slots[current_music]);
}

void music_resume(void) {
    if (current_music < 0) return;
    ResumeMusicStream(music_slots[current_music]);
}

void music_volume(float v) {
    if (current_music < 0) return;
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;
    SetMusicVolume(music_slots[current_music], v);
}

void music_update(void) {
    if (current_music < 0) return;
    UpdateMusicStream(music_slots[current_music]);
}

// ============================================================
// Tone synthesis (WASM-4 style, 4 channels)
//   Канал 0: pulse 12.5%
//   Канал 1: pulse 25%
//   Канал 2: triangle
//   Канал 3: noise
// Один звук на канал. Новый звук вытесняет старый.
// ============================================================

// --- Генераторы волн ---

static void tone_gen_pulse(short *samples, int count,
                           float freq, int volume, float duty) {
    for (int i = 0; i < count; i++) {
        float t     = (float)i / TONE_SAMPLE_RATE;
        float phase = fmodf(t * freq, 1.0f);
        float v     = (phase < duty) ? 1.0f : -1.0f;
        samples[i]  = (short)(v * volume * 327.67f);
    }
}

static void tone_gen_triangle(short *samples, int count,
                              float freq, int volume) {
    for (int i = 0; i < count; i++) {
        float t     = (float)i / TONE_SAMPLE_RATE;
        float phase = fmodf(t * freq, 1.0f);
        float v     = 2.0f * fabsf(2.0f * phase - 1.0f) - 1.0f;
        samples[i]  = (short)(v * volume * 327.67f);
    }
}

static void tone_gen_noise(short *samples, int count, int volume) {
    for (int i = 0; i < count; i++) {
        float v    = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
        samples[i] = (short)(v * volume * 327.67f);
    }
}

// --- Публичная функция ---

void tone(int frequency, int duration, int volume, int flags) {
    if (duration <= 0 || volume <= 0) return;
    if (!IsAudioDeviceReady()) return;

    if (!tone_seeded) {
        srand((unsigned)time(NULL));
        tone_seeded = true;
    }

    int channel  = flags & 3;   // 0..3
    int waveform = flags & 3;   // waveform == channel

    float freq         = (float)frequency;
    float duration_sec = duration / 60.0f;
    int   sample_count = (int)(TONE_SAMPLE_RATE * duration_sec);
    if (sample_count <= 0) return;

    short *samples = (short *)malloc(sample_count * sizeof(short));
    if (!samples) return;

    switch (waveform) {
        case 0: tone_gen_pulse(samples, sample_count, freq, volume, 0.125f); break;
        case 1: tone_gen_pulse(samples, sample_count, freq, volume, 0.250f); break;
        case 2: tone_gen_triangle(samples, sample_count, freq, volume);      break;
        case 3: tone_gen_noise(samples, sample_count, volume);               break;
    }

    Wave wave = {
        .frameCount = sample_count,
        .sampleRate = TONE_SAMPLE_RATE,
        .sampleSize = 16,
        .channels   = 1,
        .data       = samples
    };

    Sound s = LoadSoundFromWave(wave);
    free(samples);
    if (s.frameCount == 0) return;

    // Вытесняем старый звук на этом канале
    if (tone_channels[channel].used) {
        StopSound(tone_channels[channel].sound);
        UnloadSound(tone_channels[channel].sound);
        tone_channels[channel].used = false;
    }

    tone_channels[channel].sound = s;
    tone_channels[channel].used  = true;
    PlaySound(s);
}
#ifndef TEST_LIBRETRO_AUDIO_H
#define TEST_LIBRETRO_AUDIO_H

/* A 440 Hz test tone, or silence, submitted every frame at 48 kHz stereo. */

#include <stdbool.h>
#include <stdint.h>

#include "libretro.h"

#define AUDIO_SAMPLE_RATE     48000.0
#define AUDIO_MAX_FRAMES_PER_RUN 1600   /* 48000 / 30, the slowest frame rate option */
#define AUDIO_TONE_HZ         440.0

typedef enum AudioChannels
{
    AUDIO_CHANNELS_OFF,
    AUDIO_CHANNELS_LEFT,
    AUDIO_CHANNELS_RIGHT,
    AUDIO_CHANNELS_BOTH,
    AUDIO_CHANNELS_COUNT
} AudioChannels;

typedef struct AudioState
{
    bool          playing;                       /* false = submit silence (keeps audio timing going) */
    AudioChannels channels;
    bool          use_single_sample_callback;   /* retro_audio_sample instead of the batch callback */
    double        phase;                         /* 0..1, carried across frames so the tone is continuous */
    unsigned      frames_per_run;                /* 48000 / fps (800 at 60 fps); 0 = 800 */
    int16_t       buffer[AUDIO_MAX_FRAMES_PER_RUN * 2];   /* interleaved stereo for one frame */
} AudioState;

/* Submits exactly frames_per_run stereo frames (silence when not playing or channels is OFF). */
void audio_run_frame(AudioState* audio, retro_audio_sample_t sample, retro_audio_sample_batch_t batch);

const char* audio_channels_name(AudioChannels channels);

#endif

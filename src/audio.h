#ifndef TEST_LIBRETRO_AUDIO_H
#define TEST_LIBRETRO_AUDIO_H

/*
 * A 440 Hz test tone, or silence, submitted every frame in stereo at the
 * sample rate option (32000, 44100, 48000 or 96000 Hz), so a frontend's
 * resampler is exercised both up and down. A rate that doesn't divide by the
 * frame rate (32000 / 60 = 533.3) gives frames of different lengths, adding
 * up exactly to the rate each second, like real cores.
 */

#include <stdbool.h>
#include <stdint.h>

#include "libretro.h"

#define AUDIO_SAMPLE_RATE_DEFAULT 48000u
#define AUDIO_MAX_FRAMES_PER_RUN  3201   /* 96000 / 30 (highest rate, slowest frame rate) + 1 for the fraction */
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
    unsigned      sample_rate;                   /* Hz; 0 = AUDIO_SAMPLE_RATE_DEFAULT */
    unsigned      fps;                           /* the core's frame rate; 0 = 60 */
    double        frames_owed;                   /* the fraction of a frame carried to the next run */
    int16_t       buffer[AUDIO_MAX_FRAMES_PER_RUN * 2];   /* interleaved stereo for one frame */
} AudioState;

/* Sets the rate and frame rate (the tone continues without a jump). */
void audio_set_timing(AudioState* audio, unsigned sample_rate, unsigned fps);

/* sample_rate / fps: the stereo frames one retro_run carries on average. */
double audio_frames_per_run(const AudioState* audio);

/* Submits one frame's worth of stereo frames (silence when not playing or channels is OFF). */
void audio_run_frame(AudioState* audio, retro_audio_sample_t sample, retro_audio_sample_batch_t batch);

const char* audio_channels_name(AudioChannels channels);

#endif

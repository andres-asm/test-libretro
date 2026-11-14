#include "audio.h"

#include <math.h>

/* Quarter of full scale: clearly audible, not painful. */
#define AUDIO_AMPLITUDE 8000.0

const char* audio_channels_name(AudioChannels channels)
{
    switch (channels)
    {
        case AUDIO_CHANNELS_OFF:   return "off";
        case AUDIO_CHANNELS_LEFT:  return "left only";
        case AUDIO_CHANNELS_RIGHT: return "right only";
        case AUDIO_CHANNELS_BOTH:  return "both";
        default:                   return "?";
    }
}

static unsigned rate_of(const AudioState* audio)
{
    return audio->sample_rate > 0 ? audio->sample_rate : AUDIO_SAMPLE_RATE_DEFAULT;
}

void audio_set_timing(AudioState* audio, unsigned sample_rate, unsigned fps)
{
    audio->sample_rate = sample_rate;
    audio->fps = fps;
    audio->frames_owed = 0.0;
}

double audio_frames_per_run(const AudioState* audio)
{
    return (double)rate_of(audio) / (double)(audio->fps > 0 ? audio->fps : 60u);
}

void audio_run_frame(AudioState* audio, retro_audio_sample_t sample, retro_audio_sample_batch_t batch)
{
    int16_t* buffer = audio->buffer;
    bool tone = audio->playing && audio->channels != AUDIO_CHANNELS_OFF;
    const double two_pi = 6.283185307179586;
    const double step = AUDIO_TONE_HZ / (double)rate_of(audio);
    unsigned frame = 0;
    unsigned frames = 0;

    /* Whole frames now, the fraction next time. */
    audio->frames_owed += audio_frames_per_run(audio);
    frames = (unsigned)audio->frames_owed;
    audio->frames_owed -= (double)frames;
    if (frames > AUDIO_MAX_FRAMES_PER_RUN)
    {
        frames = AUDIO_MAX_FRAMES_PER_RUN;
    }

    for (frame = 0; frame < frames; frame++)
    {
        int16_t value = 0;
        int16_t left = 0;
        int16_t right = 0;

        if (tone)
        {
            value = (int16_t)(sin(audio->phase * two_pi) * AUDIO_AMPLITUDE);
            audio->phase += step;
            if (audio->phase >= 1.0)
            {
                audio->phase -= 1.0;
            }
        }

        if (tone && (audio->channels == AUDIO_CHANNELS_LEFT || audio->channels == AUDIO_CHANNELS_BOTH))
        {
            left = value;
        }
        if (tone && (audio->channels == AUDIO_CHANNELS_RIGHT || audio->channels == AUDIO_CHANNELS_BOTH))
        {
            right = value;
        }

        buffer[frame * 2] = left;
        buffer[frame * 2 + 1] = right;
    }

    if (audio->use_single_sample_callback)
    {
        for (frame = 0; frame < frames; frame++)
        {
            sample(buffer[frame * 2], buffer[frame * 2 + 1]);
        }
    }
    else
    {
        /* The frontend may take fewer frames than offered; keep handing over the rest. */
        size_t done = 0;
        while (done < frames)
        {
            size_t taken = batch(buffer + done * 2, frames - done);
            if (taken == 0)
            {
                break;
            }
            done += taken;
        }
    }
}

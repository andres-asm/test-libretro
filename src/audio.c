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

void audio_run_frame(AudioState* audio, retro_audio_sample_t sample, retro_audio_sample_batch_t batch)
{
    int16_t* buffer = audio->buffer;
    bool tone = audio->playing && audio->channels != AUDIO_CHANNELS_OFF;
    const double two_pi = 6.283185307179586;
    const double step = AUDIO_TONE_HZ / AUDIO_SAMPLE_RATE;
    unsigned frame = 0;

    for (frame = 0; frame < AUDIO_FRAMES_PER_RUN; frame++)
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
        for (frame = 0; frame < AUDIO_FRAMES_PER_RUN; frame++)
        {
            sample(buffer[frame * 2], buffer[frame * 2 + 1]);
        }
    }
    else
    {
        /* The frontend may take fewer frames than offered; keep handing over the rest. */
        size_t done = 0;
        while (done < AUDIO_FRAMES_PER_RUN)
        {
            size_t taken = batch(buffer + done * 2, AUDIO_FRAMES_PER_RUN - done);
            if (taken == 0)
            {
                break;
            }
            done += taken;
        }
    }
}

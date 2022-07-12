#include "sound.h"
#include "resources.h"
#include <stdlib.h>
#include "log.h"
#include "string.h"
#include "math.h"
#include "limits.h"
#include "time.h"
#include "music.h"

#include "SDL2/SDL.h"

#include "mix.h"
#include "dsp.h"

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"

#include "portaudio.h"

#include "circbuf.h"

#define TSF_IMPLEMENTATION
#include "tsf.h"

#define TML_IMPLEMENTATION
#include "tml.h"

const char *audioDriver;

struct wav change_samplerate(struct wav w, int rate)
{
    SDL_AudioStream *stream = SDL_NewAudioStream(AUDIO_S16, w.ch, w.samplerate, AUDIO_S16, w.ch, rate);
    SDL_AudioStreamPut(stream, w.data, w.frames*w.ch*sizeof(short));

    int oldframes = w.frames;
    w.frames *= (float)rate/w.samplerate;

    w.samplerate = rate;
    int samples = sizeof(short)*w.ch*w.frames;
    short *new = malloc(samples);
    SDL_AudioStreamGet(stream, new, samples);

    free(w.data);
    w.data = new;
    SDL_FreeAudioStream(stream);

    return w;
}

static int patestCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData)
{
    short *out = (short*)outputBuffer;

    bus_fill_buffers(outputBuffer, framesPerBuffer);

    return 0;
}

void check_pa_err(PaError e)
{
    if (e != paNoError) {
        YughError("PA Error: %s", Pa_GetErrorText(e));
        exit(1);
    }
}

static PaStream *stream_def;

void normalize_gain(struct wav *w, double lv)
{
    short tarmax = db2short(lv);
    short max = 0;
    short *s = w->data;
    for (int i = 0; i < w->frames; i++) {
        for (int j = 0; j < w->ch; j++) {
            max = (abs(s[i*w->ch + j]) > max) ? abs(s[i*w->ch + j]) : max;
        }
    }

    w->gain = log10((float)tarmax/max) * 20;
}

struct osc sin600;
struct osc sin20;
struct dsp_ammod dspammod;
struct dsp_delay dspdel;
struct wav s600wav;
struct sound s600wavsound;

void sound_init()
{
     PaError err = Pa_Initialize();
    check_pa_err(err);

    int numDevices = Pa_GetDeviceCount();
    const PaDeviceInfo *deviceInfo;

    for (int i = 0; i < numDevices; i++) {
        deviceInfo = Pa_GetDeviceInfo(i);

       // printf("Device %i: channels %i, sample rate %f, name %s\n", i, deviceInfo->maxOutputChannels, deviceInfo->defaultSampleRate, deviceInfo->name);
    }

    PaStreamParameters outparams;


/*
    outparams.channelCount = 2;
    outparams.device = 19;
    outparams.sampleFormat = paInt16;
    outparams.suggestedLatency = Pa_GetDeviceInfo(outparams.device)->defaultLowOutputLatency;
    outparams.hostApiSpecificStreamInfo = NULL;
    */

    //err = Pa_OpenStream(&stream_def, NULL, &outparams, 48000, 4096, paNoFlag, patestCallback, &data);
    err = Pa_OpenDefaultStream(&stream_def, 0, 2, paInt16, SAMPLERATE, BUF_FRAMES, patestCallback, NULL);
    check_pa_err(err);

    err = Pa_StartStream(stream_def);
    check_pa_err(err);
}

void audio_open(const char *device)
{
    //Mix_OpenAudioDevice(44100, MIX_DEFAULT_FORMAT, 2, 2048, device, 0);
}

void audio_close()
{
    //Mix_CloseAudio();
}

struct wav make_sound(const char *wav)
{
    struct wav mwav;
    mwav.data = drwav_open_file_and_read_pcm_frames_s16("sounds/alert.wav", &mwav.ch, &mwav.samplerate, &mwav.frames, NULL);

    if (mwav.samplerate != SAMPLERATE) {
        mwav = change_samplerate(mwav, 48000);
    }

    mwav.gain = 0;

    normalize_gain(&mwav, -3);

    return mwav;
}

struct sound play_sound(struct wav *wav)
{
    struct sound new;
    new.loop = 0;
    new.frame = 0;
    new.gain = 0;
    new.data = wav;

    // TODO: Make filter to send to mixer

    return new;

}

struct music make_music(const char *mp3)
{
    drmp3 new;
    if (!drmp3_init_file(&new, mp3, NULL)) {
        YughError("Could not open mp3 file %s.", mp3);
    }

    //printf("CIrcus mp3 channels: %ui, samplerate: %ui\n", mp3.channels, mp3.sampleRate);
}




void audio_init()
{
    //audioDriver = SDL_GetAudioDeviceName(0,0);
}


void close_audio_device(int device)
{
    //SDL_CloseAudioDevice(device);
}

int open_device(const char *adriver)
{

/*
    SDL_AudioSpec audio_spec;
    SDL_memset(&audio_spec, 0, sizeof(audio_spec));
    audio_spec.freq = SAMPLERATE;
    audio_spec.format = AUDIO_F32;
    audio_spec.channels = 2;
    audio_spec.samples = BUF_FRAMES;
    int dev = (int) SDL_OpenAudioDevice(adriver, 0, &audio_spec, NULL, 0);
    SDL_PauseAudioDevice(dev, 0);

    return dev;
*/
    return 0;
}

void sound_fillbuf(struct sound *s, short *buf, int n)
{
    short *in = s->data->data;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < 2; j++) {
            buf[i*2+j] = in[s->frame+j];
        }
        s->frame++;
        if (s->frame == s->data->frames) {
            s->frame = 0;
            if (s->loop > 0) {
                s->loop--;
            }
        }
    }
}

struct soundstream soundstream_make()
{
    struct soundstream new;
    new.buf = circbuf_init(sizeof(short), BUF_FRAMES*CHANNELS*2);
    return new;
}

void soundstream_fillbuf(struct soundstream *s, short *buf, int n)
{
    int max = s->buf.write - s->buf.read;
    int lim = (max < n*CHANNELS) ? max : n*CHANNELS;
    for (int i = 0; i < lim; i++) {
        buf[i] = cbuf_shift(&s->buf);
    }
}

float short2db(short val)
{
    return 20*log10(abs((double)val) / SHRT_MAX);
}

short db2short(float db)
{
    return pow(10, db/20.f) * SHRT_MAX;
}

short short_gain(short val, float db)
{
    return (short)(pow(10, db/20.f) * val);
}

float pct2db(float pct)
{
    if (pct <= 0) return -72.f;

    return 10*log2(pct);
}
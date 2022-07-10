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

struct sound *mus_cur;


struct circbuf vidbuf;

        drmp3 mp3;

struct wav mwav;
struct sound wavsound;

struct wav sin440;
struct sound a440;


int vidplaying = 0;

struct wav change_samplerate(struct wav w, int rate)
{
    printf("Going from sr %i to sr %i.\n", w.samplerate, rate);
    SDL_AudioStream *stream = SDL_NewAudioStream(AUDIO_S16, w.ch, w.samplerate, AUDIO_S16, w.ch, rate);
    SDL_AudioStreamPut(stream, w.data, w.frames*w.ch*sizeof(short));

    int oldframes = w.frames;
    w.frames *= (float)rate/w.samplerate;

    printf("Went from %i to %i frames.\n", oldframes, w.frames);
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
    /* Cast data passed through stream to our structure. */
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
    short tarmax = pow(10, lv/20.f) * SHRT_MAX;
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
    vidbuf = circbuf_init(sizeof(short), 262144);

    mwav.data = drwav_open_file_and_read_pcm_frames_s16("sounds/alert.wav", &mwav.ch, &mwav.samplerate, &mwav.frames, NULL);

    mwav.gain = 0;

    printf("Loaded wav: ch %i, sr %i, fr %i.\n", mwav.ch, mwav.samplerate, mwav.frames);

   // mwav = change_samplerate(mwav, 48000);

    //mwav = gen_square(1, 150, 48000, 2);

    wavsound.data = &mwav;
    wavsound.loop = 1;

    normalize_gain(&mwav, -3);

    //play_sound(&wavsound);

    sin440 = gen_sine(0.4f, 30, SAMPLERATE, 2);
    a440.data = &sin440;
    a440.loop = 1;
    //play_sound(&a440);

    printf("Playing wav with %i frames.\n", wavsound.data->frames);

    sin600.f = tri_phasor;
    sin600.p = phasor_make(SAMPLERATE, 200);



    sin20.f = square_phasor;
    sin20.p.sr = SAMPLERATE;
    sin20.p.freq = 4;

    s600wav = gen_sine(0.6f, 600, SAMPLERATE, CHANNELS);

    s600wavsound.loop = -1;
    s600wavsound.data = &s600wav;


    struct dsp_filter s600;
    s600.data = &s600wavsound;
    s600.filter = sound_fillbuf;

    struct dsp_filter s20;
    s20.data = &sin20;
    s20.filter = osc_fillbuf;

    struct dsp_filter am_filter;


    dspammod.ina = s600;
    dspammod.inb = s20;

    am_filter.filter = am_mod;
    am_filter.data = &dspammod;

    dspdel = dsp_delay_make(150);
    dspdel.in = am_filter;
    struct dsp_filter del_filter;
    del_filter.filter = dsp_delay_filbuf;
    del_filter.data = &dspdel;

    //first_free_bus(s600);

    struct dsp_filter wn;
    wn.filter = gen_pinknoise;
    //first_free_bus(wn);

/*
    if (!drmp3_init_file(&mp3, "sounds/circus.mp3", NULL)) {
        YughError("Could not open mp3.",0);
    }

    printf("CIrcus mp3 channels: %ui, samplerate: %ui\n", mp3.channels, mp3.sampleRate);
*/

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




    play_song("", "");
}

void audio_open(const char *device)
{
    //Mix_OpenAudioDevice(44100, MIX_DEFAULT_FORMAT, 2, 2048, device, 0);
}

void audio_close()
{
    //Mix_CloseAudio();
}

struct sound *make_sound(const char *wav)
{

    struct sound *new = calloc(1, sizeof(struct sound));
  /*  ma_result res = ma_sound_init_from_file(&engine, wav, 0, NULL, NULL, &new->sound);

    if (res != MA_SUCCESS) {
        printf("HONO!!!!");
    }
*/
    return new;
}

struct sound *make_music(const char *ogg)
{

    struct sound *sound = calloc(1, sizeof(struct sound));
   // ma_result res = ma_sound_init_from_file(&engine, ogg, 0, NULL, &mus_grp, &sound->sound);

    return sound;
}



void play_music(struct sound *music)
{
    //ma_sound_start(&music->sound);
    //music->state = MUS_PLAY;
   // mus_cur = music;
}

void music_set(struct sound *music)
{

}

void music_volume(unsigned char vol)
{
  //  ma_sound_group_set_volume(&mus_grp, (float)vol/127);
}

int music_playing()
{
    //return ma_sound_is_playing(&mus_cur->sound);
    return 0;
}

int music_paused()
{
  //  return mus_cur->state == MUS_PAUSE;
  return 0;
}

void music_resume()
{
  //  ma_sound_start(&mus_cur->sound);
}

void music_pause()
{
    //ma_sound_stop(&mus_cur->sound);
    //mus_cur->state = MUS_PAUSE;
}

void music_stop()
{
   // ma_sound_stop(&mus_cur->sound);
 //   mus_cur->state = MUS_STOP;
 //   ma_sound_seek_to_pcm_frame(&mus_cur->sound, 0);
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

void play_sound(struct sound *sound)
{
    sound->frame = 0;
    //struct bus *b = first_free_bus(sound, sound_fillbuf);
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


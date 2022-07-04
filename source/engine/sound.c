#include "sound.h"
#include "resources.h"
#include <stdlib.h>
#include "log.h"
#include "string.h"
#include "math.h"
#include "limits.h"
#include "time.h"

#include "dsp.h"


#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"

#include "portaudio.h"

#include "circbuf.h"


const char *audioDriver;

struct sound *mus_cur;

#define BUSFRAMES 15000

struct circbuf vidbuf;

        drmp3 mp3;

struct wav mwav;
struct sound wavsound;


int vidplaying = 0;

static int patestCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData)
{
    /* Cast data passed through stream to our structure. */
    short *out = (short*)outputBuffer;

/*
    int f = 0;

    static int interp = 0;

    for (int i = 0; i < framesPerBuffer; i++) {
        *(out++) = *(short*)(mwav.data++);
        *(out++) = *(short*)(mwav.data++);
    }
    */


    if (wavsound.play) {
        //clock_t start = clock();
        //wavsound.data->gain = -6;

        float mult = powf(10.f, (float)wavsound.data->gain/20.f);
        printf("Mult is %f\n", mult);
        short *s = (short*)wavsound.data->data;
        for (int i = 0; i < framesPerBuffer; i++) {
            out[i*2] = s[wavsound.frame++] * mult;
            if (wavsound.frame == wavsound.data->frames) wavsound.frame = 0;
            out[i*2+1] = s[wavsound.frame++] * mult;
            if (wavsound.frame == wavsound.data->frames) wavsound.frame = 0;
        }

        /*
        static int end = 0;
        end = wavsound.data->frames - wavsound.frame;
        if (end >= framesPerBuffer) {
            memcpy(out, &s[wavsound.frame*2], framesPerBuffer * 2 * sizeof(short));
            wavsound.frame += framesPerBuffer;
        } else {
            memcpy(out, &s[wavsound.frame*2], end * 2 * sizeof(short));
            wavsound.frame = framesPerBuffer - end;
            memcpy(out+(end*2), s, wavsound.frame *2*sizeof(short));
        }
*/
        //printf("Time took is %f.\n", (double)(clock() - start)/CLOCKS_PER_SEC);
    }




    if (!vidplaying) return 0;

    for (int i = 0; i < framesPerBuffer; i++) {
       //short a[2] = {0, 0};

        //a[0] += *(short*)cbuf_shift(&vidbuf) * 5;
        //a[1] += *(short*)cbuf_shift(&vidbuf) * 5;

        out[i*2] += cbuf_shift(&vidbuf) * 5;
        out[i*2+1] += cbuf_shift(&vidbuf) * 5;
    }

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

void sound_init()
{
    vidbuf = circbuf_init(sizeof(short), 262144);

    mwav.data = drwav_open_file_and_read_pcm_frames_s16("sounds/alert.wav", &mwav.ch, &mwav.samplerate, &mwav.frames, NULL);

    mwav.gain = 0;

    printf("Loaded wav: ch %i, sr %i, fr %i.\n", mwav.ch, mwav.samplerate, mwav.frames);

    mwav = gen_square(1, 150, 48000, 2);

/*
    short *tdata = mwav.data;
    mwav.frames /= 2;
    short *newdata = calloc(mwav.frames * 2, sizeof(short));
    for (int i = 0; i < mwav.frames; i++) {
        newdata[i] = tdata[i*2];
    }
*/

    wavsound.data = &mwav;
    wavsound.loop = 1;
    wavsound.play = 1;

    normalize_gain(&mwav, -3);

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
    err = Pa_OpenDefaultStream(&stream_def, 0, 2, paInt16, 48000, 2048, patestCallback, NULL);
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

void play_sound(struct sound *sound)
{
    //ma_sound_set_volume(&sound->sound, (float)sound->volume/127);
  //  ma_sound_start(&sound->sound);
   // sound->state = MUS_PLAY;
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

void play_raw(int device, void *data, int size)
{

    float *d = data;
    short t;
    for (int i = 0; i < size; i++) {
        t = (short)(d[i]*32767);
        cbuf_push(&vidbuf, t);
    }

    vidplaying = 1;
    /*
    for (int i = 0; i < size; i++) {
        short temp = (short)(d[i] * 32767);
        cbuf_append(&vidbuf, &temp, 1);
    }
    */
}

void close_audio_device(int device)
{
    //SDL_CloseAudioDevice(device);
}

void clear_raw(int device)
{
   //SDL_ClearQueuedAudio(device);
}

int open_device(const char *adriver)
{

/*
    SDL_AudioSpec audio_spec;
    SDL_memset(&audio_spec, 0, sizeof(audio_spec));
    audio_spec.freq = 48000;
    audio_spec.format = AUDIO_F32;
    audio_spec.channels = 2;
    audio_spec.samples = 4096;
    int dev = (int) SDL_OpenAudioDevice(adriver, 0, &audio_spec, NULL, 0);
    SDL_PauseAudioDevice(dev, 0);

    return dev;
*/
    return 0;
}

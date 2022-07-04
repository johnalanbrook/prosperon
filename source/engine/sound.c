#include "sound.h"
#include "resources.h"
#include <stdlib.h>
#include "log.h"
#include "string.h"


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

short *get_sound_frame(struct sound *s, int sr) {
    s->frame = (s->frame+3) % s->w->frames;
    return &s->w->data[s->frame];
}

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
/*
    if (wavsound.play) {
        for (int i = 0; i < framesPerBuffer; i++) {
            short *f = get_sound_frame(&wavsound, 48000);
            out[i*2] += f[0];
            out[i*2+1] += f[1];
        }
    }
    */
    if (!vidplaying) return 0;

    for (int i = 0; i < framesPerBuffer; i++) {
       //short a[2] = {0, 0};

        //a[0] += *(short*)cbuf_shift(&vidbuf) * 5;
        //a[1] += *(short*)cbuf_shift(&vidbuf) * 5;

        *(out++) = cbuf_shift(&vidbuf) * 5;
        *(out++) = cbuf_shift(&vidbuf) * 5;
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



void sound_init()
{
    vidbuf = circbuf_init(sizeof(short), 262144);

    mwav.data = drwav_open_file_and_read_pcm_frames_s16("sounds/alert.wav", &mwav.ch, &mwav.samplerate, &mwav.frames, NULL);

    printf("Loaded wav: ch %i, sr %i, fr %i.\n", mwav.ch, mwav.samplerate, mwav.frames);

    wavsound.w = &mwav;
    wavsound.loop = 1;
    wavsound.play = 1;

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
    err = Pa_OpenDefaultStream(&stream_def, 0, 2, paInt16, 48000, 256, patestCallback, NULL);
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

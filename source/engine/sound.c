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

//ma_engine engine;

const char *audioDriver;

struct sound *mus_cur;
//ma_sound_group mus_grp;

typedef struct
{
    float left_phase;
    float right_phase;
} paTestData;

struct circbuf vidbuf;

short HalfSecond[22400];
short *shorthead;


   unsigned int ch;
    unsigned int srate;
    drwav_uint64 curcmf = 0;
    drwav_uint64 totalpcmf;
    float *psamps;

        drmp3 mp3;

 float inbuf[4096*2];
 float filtbuf[3763*2];

static int patestCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData)
{
    /* Cast data passed through stream to our structure. */
    short *out = (short*)outputBuffer;

    int f = 0;

    static int interp = 0;

    for (int i = 0; i < framesPerBuffer; i++) {
       short a[2] = {0, 0};

        a[0] += *(short*)cbuf_take(&vidbuf) * 5;
        a[1] += *(short*)cbuf_take(&vidbuf) * 5;

        *(out++) = a[0];
        *(out++) = a[1];
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

static paTestData data;
static PaStream *stream_def;

void sound_init()
{
    vidbuf = circbuf_init(sizeof(short), 163864);

    drwav wav;
    if (!drwav_init_file(&wav, "sounds/alert.wav", NULL)) {
        YughError("Could not open wav.",0);
    }

    //drwav_int32 *wavdec = malloc(wav.totalPCMFrameCount * wav.channels * sizeof(drwav_int32));
    //size_t samps = drwav_read_pcm_frames_s32(&wav, wav.totalPCMFrameCount, wavdec);


    psamps = drwav_open_file_and_read_pcm_frames_s16("sounds/alert.wav", &ch, &srate, &totalpcmf, NULL);

    printf("WAV is: %i channels, %i samplerate, %l frames.\n", ch, srate, totalpcmf);


    if (!drmp3_init_file(&mp3, "sounds/circus.mp3", NULL)) {
        YughError("Could not open mp3.",0);
    }

    printf("CIrcus mp3 channels: %ui, samplerate: %ui\n", mp3.channels, mp3.sampleRate);


        PaError err = Pa_Initialize();
    check_pa_err(err);

    int numDevices = Pa_GetDeviceCount();
    const PaDeviceInfo *deviceInfo;

    for (int i = 0; i < numDevices; i++) {
        deviceInfo = Pa_GetDeviceInfo(i);

        printf("Device %i: channels %i, sample rate %f, name %s\n", i, deviceInfo->maxOutputChannels, deviceInfo->defaultSampleRate, deviceInfo->name);
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
    err = Pa_OpenDefaultStream(&stream_def, 0, 2, paInt16, 48000, 4096, patestCallback, NULL);
    check_pa_err(err);

    err = Pa_StartStream(stream_def);
    check_pa_err(err);

    //Pa_Sleep(1000);

    //check_pa_err(Pa_StopStream(stream));



/*

    ma_result result;
    ma_device device;
    ma_device_config cnf;

    cnf = ma_device_config_init(ma_device_type_playback);
    cnf.playback.format = ma_format_f32;
    cnf.playback.channels = 2;
    cnf.sampleRate = 48000;
    cnf.dataCallback = data_callback;

    result = ma_device_init(NULL, &cnf, &device);
    if (result != MA_SUCCESS) {
        YughError("Did not initialize audio playback!!",0);
    }


    result = ma_device_start(&device);
   if (result != MA_SUCCESS) {
       printf("Failed to start playback device.\n");
   }
*/

/*
    ma_engine_config enginecnf = ma_engine_config_init();
    enginecnf.pDevice = &device;

    ma_result result = ma_engine_init(&enginecnf, &engine);
    if (result != MA_SUCCESS) {
        YughError("Miniaudio did not start properly.",1);
        exit(1);
    }

    ma_sound_group_init(&engine, 0, NULL, &mus_grp);
*/
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
    short t[size];
    for (int i = 0; i < size; i++) {
        t[i] = d[i]*32767;
    }
    cbuf_append(&vidbuf, t, size);
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

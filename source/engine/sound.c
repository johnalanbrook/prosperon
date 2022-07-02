#include "sound.h"
#include "resources.h"
#include <stdlib.h>
#include "log.h"

ma_engine engine;

const char *audioDriver;

struct sound *mus_cur;
ma_sound_group mus_grp;

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    printf("audio cb\n");
    // In playback mode copy data to pOutput. In capture mode read data from pInput. In full-duplex mode, both
    // pOutput and pInput will be valid and you can move data from pInput into pOutput. Never process more than
    // frameCount frames.
    ma_engine_read_pcm_frames(&engine, pOutput, frameCount, NULL);
}

void sound_init()
{


    ma_device_config cnf = ma_device_config_init(ma_device_type_playback);
    cnf.playback.format = ma_format_f32;
    cnf.playback.channels = 2;
    cnf.sampleRate = 48000;
    cnf.dataCallback = data_callback;
    cnf.pUserData = mus_cur;

    ma_device device;
    if (ma_device_init(NULL, &cnf, &device) != MA_SUCCESS) {
        YughError("Did not initialize audio playback!!",0);
    }


   if (ma_device_start(&device) != MA_SUCCESS) {
       printf("Failed to start playback device.\n");
   }



    ma_engine_config enginecnf = ma_engine_config_init();
    enginecnf.pDevice = &device;

    ma_result result = ma_engine_init(&enginecnf, &engine);
    if (result != MA_SUCCESS) {
        YughError("Miniaudio did not start properly.",1);
        exit(1);
    }

    ma_sound_group_init(&engine, 0, NULL, &mus_grp);

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
    ma_result res = ma_sound_init_from_file(&engine, wav, 0, NULL, NULL, &new->sound);

    if (res != MA_SUCCESS) {
        printf("HONO!!!!");
    }

    return new;
}

struct sound *make_music(const char *ogg)
{
    struct sound *sound = calloc(1, sizeof(struct sound));
    ma_result res = ma_sound_init_from_file(&engine, ogg, 0, NULL, &mus_grp, &sound->sound);

    return sound;
}

void play_sound(struct sound *sound)
{
    //ma_sound_set_volume(&sound->sound, (float)sound->volume/127);
    ma_sound_start(&sound->sound);
    sound->state = MUS_PLAY;
}

void play_music(struct sound *music)
{
    ma_sound_start(&music->sound);
    music->state = MUS_PLAY;
    mus_cur = music;
}

void music_set(struct sound *music)
{

}

void music_volume(unsigned char vol)
{
    ma_sound_group_set_volume(&mus_grp, (float)vol/127);
}

int music_playing()
{
    return ma_sound_is_playing(&mus_cur->sound);
}

int music_paused()
{
    return mus_cur->state == MUS_PAUSE;
}

void music_resume()
{
    ma_sound_start(&mus_cur->sound);
}

void music_pause()
{
    ma_sound_stop(&mus_cur->sound);
    mus_cur->state = MUS_PAUSE;
}

void music_stop()
{
    ma_sound_stop(&mus_cur->sound);
    mus_cur->state = MUS_STOP;
    ma_sound_seek_to_pcm_frame(&mus_cur->sound, 0);
}


void audio_init()
{
    //audioDriver = SDL_GetAudioDeviceName(0,0);
}

void play_raw(int device, void *data, int size)
{
    //SDL_QueueAudio(device, data, size);
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

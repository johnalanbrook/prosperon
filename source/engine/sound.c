#define MINIAUDIO_IMPLEMENTATION


#include "sound.h"
#include "resources.h"

const char *audioDriver;

static int mus_ch = -1;

void sound_init()
{
/*
    int flags = MIX_INIT_MP3 | MIX_INIT_OGG;
    int err = Mix_Init(flags);
    if ((err&flags) != flags) {
        printf("MIX did not init!!");
    }
*/
    //mus_ch = Mix_AllocateChannels(1);

    //Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
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
    //new->sound = Mix_LoadWAV(wav);

    return new;
}

struct music *make_music(const char *ogg)
{
    struct music *sound = calloc(1, sizeof(struct music));
    //sound->music = Mix_LoadMUS(make_path(ogg));

    return sound;
}

void play_sound(struct sound *sound)
{
    //Mix_VolumeChunk(sound->sound, sound->volume);
    //Mix_PlayChannel(-1, sound->sound, 0);
}

void play_music(struct sound *music)
{
    //Mix_PlayChannel(mus_ch, music->sound, -1);
}

void music_set(struct sound *music)
{

}

void music_volume(unsigned char vol)
{
    //Mix_Volume(mus_ch, vol);
}

int music_playing()
{
    //return Mix_Playing(mus_ch);
    return 0;
}

int music_paused()
{
    //return Mix_Paused(mus_ch);
    return 0;
}

void music_resume()
{
    //Mix_Resume(mus_ch);
}

void music_pause()
{
    //Mix_Pause(mus_ch);
}

void music_stop()
{
    //Mix_HaltChannel(mus_ch);
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

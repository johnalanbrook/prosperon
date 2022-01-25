#include "sound.h"

const char *audioDriver;

static int mus_ch = -1;

void sound_init()
{
    int flags = MIX_INIT_MP3 | MIX_INIT_OGG;
    int err = Mix_Init(flags);
    if (err&flags != flags) {
        printf("MIX did not init!!");
    }

    mus_ch = Mix_AllocateChannels(1);
}

void audio_open(const char *device)
{
    Mix_OpenAudioDevice(44100, MIX_DEFAULT_FORMAT, 2, 2048, device, 0);
}

void audio_close()
{
    Mix_CloseAudio();
}

struct sound *make_sound(const char *wav)
{
    struct sound *new = calloc(1, sizeof(struct sound));
    new->sound = Mix_LoadWAV(wav);

    return new;
}

struct music *make_music(const char *ogg)
{
    struct music *sound = calloc(1, sizeof(struct music));
    sound->music = Mix_LoadMUS(make_path(ogg));

    return sound;
}

void play_sound(struct sound *sound)
{
    Mix_VolumeChunk(sound->sound, sound->volume);
    Mix_PlayChannel(-1, sound->sound, 0);
}

void play_music(struct sound *music)
{
    Mix_PlayChannel(mus_ch, music->sound, -1);
}

void music_set(struct sound *music)
{

}

void music_volume(unsigned char vol)
{
    Mix_Volume(mus_ch, vol);
}

int music_playing()
{
    return Mix_Playing(mus_ch);
}

int music_paused()
{
    return Mix_Paused(mus_ch);
}

void music_resume()
{
    Mix_Resume(mus_ch);
}

void music_pause()
{
    Mix_Pause(mus_ch);
}

void music_stop()
{
    Mix_HaltChannel(mus_ch);
}


void audio_init()
{
    audioDriver = SDL_GetAudioDeviceName(0,0);
}
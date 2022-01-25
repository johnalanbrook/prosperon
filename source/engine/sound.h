#ifndef SOUND_H
#define SOUND_H

#include <SDL2/SDL_mixer.h>



struct sound {
    Mix_Chunk *sound;
    unsigned char volume;
};

struct music {
    Mix_Music *music;
    unsigned char volume;
};

struct player {

};

extern const char *audioDriver;

void sound_init();
void audio_open(const char *device);
void audio_close();

struct sound *make_sound(const char *wav);
struct music *make_music(const char *ogg);

void play_sound(struct sound *sound);

const char *get_audio_driver();

void play_music(struct sound *music);
void music_set(struct sound *music);
int music_playing();
int music_paused();
void music_volume(unsigned char vol);
void music_resume();
void music_pause();
void music_stop();

void audio_init();

#endif
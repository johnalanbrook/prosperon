#ifndef SOUND_H
#define SOUND_H

#include <miniaudio.h>



struct Mix_Chunk {
    int i;
};

struct Mix_Music {
    int i;
};

enum MUS {
    MUS_STOP,
    MUS_PLAY,
    MUS_PAUSE
};

struct sound {
    ma_sound sound;
    enum MUS state;
    unsigned char volume;
};

extern const char *audioDriver;

void sound_init();
void audio_open(const char *device);
void audio_close();

struct sound *make_sound(const char *wav);
struct sound *make_music(const char *ogg);

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

void close_audio_device(int device);
void clear_raw(int device);
void play_raw(int device, void *data, int size);
int open_device(const char *adriver);

void audio_init();

#endif

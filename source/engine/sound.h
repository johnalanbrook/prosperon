#ifndef SOUND_H
#define SOUND_H

#include "circbuf.h"

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

struct soundstream {
    struct circbuf buf;
};

struct soundstream soundstream_make();

struct sound {
    int loop;
    int frame;
    struct wav *data;
    unsigned char volume;
};

struct wav {
    unsigned int ch;
    unsigned int samplerate;
    unsigned int frames;
    double gain;
    void *data;
};

extern const char *audioDriver;

void sound_init();
void audio_open(const char *device);
void audio_close();

void sound_fillbuf(struct sound *s, short *buf, int n);

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

void soundstream_fillbuf(struct soundstream *stream, short *buf, int n);

void close_audio_device(int device);
int open_device(const char *adriver);

void audio_init();

#endif

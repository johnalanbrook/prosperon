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
    float gain;

    struct wav *data;
};

struct wav {
    unsigned int ch;
    unsigned int samplerate;
    unsigned int frames;
    float gain;

    void *data;
};

struct music {

};

extern const char *audioDriver;

void sound_init();
void audio_open(const char *device);
void audio_close();

void sound_fillbuf(struct sound *s, short *buf, int n);

struct wav make_sound(const char *wav);
struct sound *make_music(const char *ogg);

void play_sound(struct wav wav);


const char *get_audio_driver();

void soundstream_fillbuf(struct soundstream *stream, short *buf, int n);

void close_audio_device(int device);
int open_device(const char *adriver);

float short2db(short val);
short db2short(float db);
float pct2db(float pct);
short short_gain(short val, float db);

void audio_init();



#endif

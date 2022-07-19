#ifndef SOUND_H
#define SOUND_H

struct circbuf;
struct SDL_AudioStream;

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
    struct circbuf *buf;
};

struct soundconvstream {
 //   SDL_AudioStream *srconv;
    void *data;
};

struct soundstream *soundstream_make();

struct sound {
    int loop;
    int frame;
    int playing;
    float gain;

    struct wav *data;
    struct bus *bus;
};

struct wav {
    unsigned int ch;
    unsigned int samplerate;
    unsigned int frames;
    float gain; /* In dB */

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
void wav_norm_gain(struct wav *w, double lv);
struct sound *play_sound(struct wav *wav);

int sound_playing(const struct sound *s);
int sound_paused(const struct sound *s);
int sound_stopped(const struct sound *s);
int sound_finished(const struct sound *s);
void sound_pause(struct sound *s);
void sound_resume(struct sound *s);
void sound_stop(struct sound *s);



struct music make_music(const char *ogg);




const char *get_audio_driver();

void soundstream_fillbuf(struct soundstream *stream, short *buf, int n);

void close_audio_device(int device);
int open_device(const char *adriver);

float short2db(short val);
short db2short(float db);
short short_gain(short val, float db);

float pct2db(float pct);
float pct2mult(float pct);

void audio_init();



#endif

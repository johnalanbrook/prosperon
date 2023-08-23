#ifndef SOUND_H
#define SOUND_H

struct circbuf;

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

struct soundstream *soundstream_make();

/* A playing sound */
struct sound {
    int loop; /* How many times to loop */
    unsigned int frame; /* Pointing to the current frame on the wav */
    int playing;
    float gain;

    struct wav *data;
    struct bus *bus;

    void (*endcb)(struct sound*);
};

/* Represents a sound file */
struct wav {
    unsigned int ch;
    unsigned int samplerate;
    unsigned long long frames;
    float gain; /* In dB */

    void *data;
};

struct mp3 {

};

void sound_init();
void audio_open(const char *device);
void audio_close();

void sound_fillbuf(struct sound *s, short *buf, int n);

void mini_sound(char *path);
void mini_master(float v);
void mini_music_play(char *path);
void mini_music_pause();
void mini_music_stop();

struct wav *make_sound(const char *wav);
void free_sound(const char *wav);
void wav_norm_gain(struct wav *w, double lv);
struct sound *play_sound(struct wav *wav);
void play_oneshot(struct wav *wav);

int sound_playing(const struct sound *s);
int sound_paused(const struct sound *s);
int sound_stopped(const struct sound *s);
int sound_finished(const struct sound *s);
void sound_pause(struct sound *s);
void sound_resume(struct sound *s);
void sound_stop(struct sound *s);

struct mp3 make_mp3(const char *mp3);

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

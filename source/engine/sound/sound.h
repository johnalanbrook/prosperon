#ifndef SOUND_H
#define SOUND_H

#include "script.h"
#include "samplerate.h"
#include "pthread.h"

typedef float soundbyte;
extern pthread_mutex_t soundrun;

struct dsp_node;

/* A bookmark into a wav, actually playing the sound */
typedef struct sound {
    unsigned int frame; /* Pointing to the current frame on the wav */
    struct wav *data;
    int loop;
    float timescale;
    SRC_STATE *src;
    JSValue hook;
} sound;

/* Represents a sound file source, fulled loaded*/
typedef struct wav {
    unsigned int ch;
    unsigned int samplerate;
    unsigned long long frames;
    soundbyte *data;
} wav;

/* Represents a sound file stream */
typedef struct mp3 {
  
} mp3;

void sound_init();
void audio_open(const char *device);
void audio_close();

struct wav *make_sound(const char *wav);
void free_sound(const char *wav);
void wav_norm_gain(struct wav *w, double lv);
struct dsp_node *dsp_source(const char *path);
struct dsp_node *dsp_mod(const char *path);

int sound_finished(const struct sound *s);

struct mp3 make_mp3(const char *mp3);

float short2db(short val);
short db2short(float db);
short short_gain(short val, float db);
float fgain(float val, float db);
float float2db(float val);
float db2float(float db);

float pct2db(float pct);
float pct2mult(float pct);

void save_qoa(char *file);

#endif

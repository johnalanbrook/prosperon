#ifndef SOUND_H
#define SOUND_H

#include "script.h"
#include "pthread.h"

typedef float soundbyte;
extern pthread_mutex_t soundrun;

struct dsp_node;

/* A bookmark into a wav, actually playing the sound */
typedef struct sound {
    unsigned int frame; /* Pointing to the current frame on the wav */
    struct pcm *data;
    int loop;
} sound;

/* Represents a sound file source, fulled loaded*/
typedef struct pcm {
    unsigned int ch;
    unsigned int samplerate;
    unsigned long long frames;
    soundbyte *data;
} pcm;

void sound_init();
void audio_open(const char *device);
void audio_close();

struct pcm *make_pcm(const char *file);
void pcm_free(pcm *pcm);
void pcm_norm_gain(struct pcm *w, double lv);
void pcm_format(pcm *pcm, int samplerate, int channels);

struct dsp_node *dsp_source(pcm *pcm);
struct dsp_node *dsp_mod(const char *path);

int sound_finished(const struct sound *s);

float short2db(short val);
short db2short(float db);
short short_gain(short val, float db);
float fgain(float val, float db);
float float2db(float val);
float db2float(float db);

float pct2db(float pct);
float pct2mult(float pct);

void save_qoa(char *file, pcm *pcm);
void save_wav(char *file, pcm *pcm);

#endif

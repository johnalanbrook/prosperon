#ifndef DSP_H
#define DSP_H

#define SAMPLERATE 48000
#define BUF_FRAMES 4096
#define CHANNELS 2
#define MUSIZE 2

#include "circbuf.h"


void dsp_rectify(short *in, short *out, int n);

struct dsp_filter {
    void (*filter)(void *data, short *out, int samples);
    void *data;
};

struct dsp_iir {
    float freq;
    int poles;
    int n;  // Amount of constants
    int order;  // How many times it's applied
    float *ccof;
    float *dcof;
    float *dx;
    float *dy;

    struct dsp_filter in;
};

struct dsp_fir {
    float freq;
    int n;
    int head;
    float *cof;
    float *dx;

    struct dsp_filter in;
};

struct dsp_filter lp_fir_make(float freq);

void dsp_iir_fillbuf(struct dsp_iir *iir, short *out, int n);
struct dsp_filter hpf_make(int poles, float freq);
struct dsp_filter lpf_make(int poles, float freq);
struct dsp_filter bpf_make(int poles, float freq1, float freq2);
struct dsp_filter npf_make(int poles, float freq1, float freq2);

struct dsp_delay {
    unsigned int ms_delay;
    struct circbuf buf;
    struct dsp_filter in;
};

struct dsp_delay dsp_delay_make(unsigned int ms_delay);
void dsp_delay_filbuf(struct dsp_delay *delay, short *buf, int n);

struct dsp_ammod {
    struct dsp_filter ina;
    struct dsp_filter inb;
    short abuf[BUF_FRAMES*2];
    short bbuf[BUF_FRAMES*2];
};

struct dsp_compressor {

};

struct dsp_limiter {

};



struct phasor {
    unsigned int sr;
    float cur;
    float freq;
    unsigned int clen;
    unsigned int cstep;
    float *cache;
};

struct phasor phasor_make(unsigned int sr, float freq);

struct osc {
    float (*f)(float p);
    struct phasor p;
    unsigned int frames;
    unsigned int frame;
    short *cache;
};

struct wav;

struct wav gen_sine(float amp, float freq, int sr, int ch);
struct wav gen_square(float amp, float freq, int sr, int ch);
struct wav gen_triangle(float amp, float freq, int sr, int ch);
struct wav gen_saw(float amp, float freq, int sr, int ch);

void gen_whitenoise(void *data, short *out, int n);
void gen_pinknoise(void *data, short *out, int n);


float sin_phasor(float p);
float square_phasor(float p);
float saw_phasor(float p);
float tri_phasor(float p);

void osc_fillbuf(struct osc *osc, short *buf, int n);

void am_mod(struct dsp_ammod *mod, short *c, int n);

#endif
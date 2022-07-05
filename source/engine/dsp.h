#ifndef DSP_H
#define DSP_H

#include "circbuf.h"

void am_mod(short *a, short *b, short *c, int n);

struct dsp_filter {
    void (*filter)(short *in, short *out, int samples, void *data);
    void *data;
};

struct dsp_delay {
    unsigned int ms_delay;
    struct circbuf buf;
};

struct wav;

struct wav gen_sine(float amp, float freq, int sr, int ch);
struct wav gen_square(float amp, float freq, int sr, int ch);
struct wav gen_triangle(float amp, float freq, int sr, int ch);
struct wav gen_saw(float amp, float freq, int sr, int ch);

#endif
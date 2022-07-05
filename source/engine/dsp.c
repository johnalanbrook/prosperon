#include "dsp.h"

#include "sound.h"
#include "limits.h"
#include "math.h"

#define PI 3.14159265

void am_mod(short *a, short *b, short *c, int n)
{
    for (int i = 0; i < n; i++) {
        c[i] = (a[i]*b[i])>>15;
    }
}

static struct wav make_wav(float freq, int sr, int ch) {
    struct wav new;
    new.ch = ch;
    new.samplerate = sr;
    new.frames = sr/freq;
    new.data = calloc(new.frames*new.ch, sizeof(short));

    return new;
}

struct wav gen_sine(float amp, float freq, int sr, int ch)
{
    struct wav new = make_wav(freq, sr, ch);

    if (amp > 1) amp = 1;
    if (amp < 0) amp = 0;
    short samp = amp*SHRT_MAX;

    short *data = (short*)new.data;

    for (int i = 0; i < new.frames; i++) {
        short val = amp * sin(2*PI*((float)i / new.frames));
        for (int j = 0; j < new.ch; j++) {
            data[i+j] = val;
        }
    }

    return new;
}

struct wav gen_square(float amp, float freq, int sr, int ch)
{
    struct wav new = make_wav(freq, sr, ch);

    int crossover = new.frames/2;

    if (amp > 1) amp = 1;
    if (amp < 0) amp = 0;

    short samp = amp * SHRT_MAX;

    short *data = (short*)new.data;

    for (int i = 0; i < new.frames; i++) {
        short val = -2 * floor(2 * i / new.frames) + 1;
        for (int j = 0; j < new.ch; j++) {
            data[i+j] = val;
        }
    }

    return new;
}

struct wav gen_triangle(float amp, float freq, int sr, int ch)
{
    struct wav new = make_wav(freq, sr, ch);

    if (amp > 1) amp = 1;
    if (amp < 0) amp = 0;

    short *data = (short*)new.data;

    for (int i = 0; i < new.frames; i++) {
        short val = 2 * abs( (i/new.frames) - floor( (i/new.frames) + 0.5));
        for (int j = 0; j < new.ch; j++) {
            data[i+j] = val;
        }
    }
}

struct wav gen_saw(float amp, float freq, int sr, int ch)
{
    struct wav new = make_wav(freq, sr, ch);

    if (amp > 1) amp = 1;
    if (amp < 0) amp = 0;

    short samp = amp*SHRT_MAX;

    short *data = (short*)new.data;

    for (int i = 0; i < new.frames; i++) {
        short val = samp * 2 * i/sr - samp;
        for (int j = 0; j < new.ch; j++) {
            data[i+j] = val;
        }
    }
}

void make_dsp_filter()
{

}

void dsp_filter(short *in, short *out, int samples, struct dsp_delay *d)
{

}
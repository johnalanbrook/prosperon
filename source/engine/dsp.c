#include "dsp.h"

#include "sound.h"
#include "limits.h"
#include "math.h"
#include "stdlib.h"

#define PI 3.14159265

struct dsp_filter make_dsp(void *data, void (*in)(void *data, short *out, int n)) {
    struct dsp_filter new;
    new.data = data;
    new.filter = in;
}

void dsp_run(struct dsp_filter filter, short *out, int n) {
    filter.filter(filter.data, out, n);
}

void am_mod(struct dsp_ammod *mod, short *c, int n)
{
    dsp_run(mod->ina, mod->abuf, n);
    dsp_run(mod->inb, mod->bbuf, n);

    for (int i = 0; i < n*2; i++) {
        c[i] = (mod->abuf[i]*mod->bbuf[i])>>15;
        //c[i] = mod->abuf[i];
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
        short val = samp * sin(2*PI*((float)i / new.frames));

        for (int j = 0; j < new.ch; j++) {
            data[i*new.ch+j] = val;
        }
    }

    printf("Made sine with %i frames.\n", new.frames);

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
            data[i*new.frames+j] = val;
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

void dsp_rectify(short *in, short *out, int n)
{
    for (int i = 0; i < n; i++) {
        out[i] = abs(in[i]);
    }
}

struct phasor phasor_make(unsigned int sr, float freq)
{
    struct phasor new;
    new.sr = sr;
    new.cur = 0.f;
    new.freq = freq;

    new.cstep = 0;
    new.clen = new.sr / new.freq;
    new.cache = malloc(new.clen * sizeof(float));

    for (int i = 0; i < new.clen; i++) {
        new.cache[i] = (float)i / new.clen;
    }

    return new;
}

float phasor_step(struct phasor *p)
{
    p->cur += p->freq/p->sr;
    if (p->cur >= 1.f) p->cur = 0.f;
    return p->cur;

/*
    float ret = p->cache[p->cstep++];
    if (p->cstep ==p->clen) p->cstep = 0;

    return ret;
    */
}

float sin_phasor(float p)
{
    return sin(2*PI*p);
}

float square_phasor(float p)
{
    return lround(p);
}

float saw_phasor(float p)
{
    return 2*p-1;
}

float tri_phasor(float p)
{
    return 4*(p * 0.5f ? p : (1-p)) - 1;
}

void osc_fillbuf(struct osc *osc, short *buf, int n)
{
    for (int i = 0; i < n; i++) {
        short val = SHRT_MAX * osc->f(phasor_step(&osc->p));
        buf[i*2] = buf[i*2+1] = val;
    }
}

void gen_whitenoise(void *data, short *buf, int n)
{
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < 2; j++) {
            buf[i*2+j] = (rand()>>15) - USHRT_MAX;
        }
    }
}

struct dsp_delay dsp_delay_make(unsigned int ms_delay)
{
    struct dsp_delay new;
    new.ms_delay = ms_delay;

    /* Circular buffer size is enough to have the delay */
    unsigned int datasize = ms_delay * CHANNELS * (SAMPLERATE / 1000);
    new.buf = circbuf_init(sizeof(short), datasize);
    new.buf.write = datasize;

    printf("Buffer size is %u.\n", new.buf.len);

    return new;
}

void dsp_delay_filbuf(struct dsp_delay *delay, short *buf, int n)
{
    static short cache[BUF_FRAMES*2];
    dsp_run(delay->in, cache, n);

    for (int i = 0; i < n*2; i++) {
        cbuf_push(&delay->buf, cache[i] / 2);
        buf[i] = cache[i] + cbuf_shift(&delay->buf);
    }
}
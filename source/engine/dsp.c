#include "dsp.h"

#include "sound.h"
#include "limits.h"
#include "math.h"
#include "stdlib.h"
#include "iir.h"
#include "log.h"
#include "vec.h"

#define PI 3.14159265

struct vec filters;

struct dsp_filter make_dsp(void *data, void (*in)(void *data, short *out, int n)) {
    struct dsp_filter new;
    new.data = data;
    new.filter = in;

    return new;

    if (filters.len == 0) {

    }
}

void dsp_run(struct dsp_filter filter, short *out, int n) {
    filter.dirty = 1; // Always on for testing

    if (!filter.dirty)
        return;

    for (int i = 0; i < filter.inputs; i++)
        dsp_run(*(filter.in[i]), out, n);

    filter.filter(filter.data, out, n);
}

void dsp_filter_addin(struct dsp_filter filter, struct dsp_filter *in)
{
    if (filter.inputs > 5) {
        YughError("Too many inputs in filter.", 0);
    }

    filter.in[filter.inputs++] = in;

    return filter;
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
    for (int i = 0; i < n; i++)
        out[i] = abs(in[i]);
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
        buf[i*CHANNELS] = buf[i*CHANNELS+1] = val;
    }
}

void gen_whitenoise(void *data, short *out, int n)
{
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < CHANNELS; j++) {
            out[i*CHANNELS+j] = (rand()>>15) - USHRT_MAX;
        }
    }
}

void gen_pinknoise(void *data, short *out, int n)
{
    gen_whitenoise(NULL, out, n);

    double b[2][7] = {0};
    double ccof[6] = {0.99886, 0.99332, 0.96900, 0.8550, 0.55000, -0.76160};
    double dcof[6] = {0.0555179, 0.0750759, 0.1538520, 0.3104856, 0.5329522, 0.0168960};

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < CHANNELS; j++) {
            double pink;
            double white = (double)out[i*CHANNELS+j]/SHRT_MAX;

            for (int k = 0; k < 5; k++) {
                b[j][k] = ccof[k]*b[j][k] + white * dcof[k];
                pink += b[j][k];
            }

            pink += b[j][5] + white*0.5362;
            b[j][5] = white*0.115926;

            out[i*CHANNELS+j] = pink * SHRT_MAX;
        }
    }
    /*
      *  The above is a loopified version of this
      *  https://www.firstpr.com.au/dsp/pink-noise/
    b0 = 0.99886 * b0 + white * 0.0555179;
    b1 = 0.99332 * b1 + white * 0.0750759;
    b2 = 0.96900 * b2 + white * 0.1538520;
    b3 = 0.86650 * b3 + white * 0.3104856;
    b4 = 0.55000 * b4 + white * 0.5329522;
    b5 = -0.7616 * b5 - white * 0.0168980;
    pink = b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362;
    b6 = white * 0.115926;
    */
}

short iir_filter(struct dsp_iir *miir, short val)
{
    struct dsp_iir iir = *miir;

    float a = 0.f;

    iir.dx[0] = (float)val/SHRT_MAX;
    for (int i = 0; i < iir.n; i++)
        a += iir.ccof[i] * iir.dx[i];

    for (int i = iir.n-1; i > 0; i--)
        iir.dx[i] = iir.dx[i-1];


    for (int i =0; i < iir.n; i++)
        a -= iir.dcof[i] * iir.dy[i];

    iir.dy[0] = a;

    for (int i = iir.n-1; i > 0; i--)
        iir.dy[i] = iir.dy[i-1];



    return a * SHRT_MAX;
}

void dsp_iir_fillbuf(struct dsp_iir *iir, short *out, int n)
{
    dsp_run(iir->in, out, n);

    for (int i = 0; i < n; i++) {
        short v = iir_filter(iir, out[i*CHANNELS]);

        for (int j = 0; j < CHANNELS; j++) {
           out[i*CHANNELS+j] = v;
        }
    }
}

struct dsp_filter lpf_make(int poles, float freq)
{
    struct dsp_iir *new = malloc(sizeof(*new));
    (*new) = make_iir(3, 1);

  double fcf = new->freq*2/SAMPLERATE;

  double sf = sf_bwlp(poles, fcf);

  printf("Making LPF filter, fcf: %f, coeffs: %i, scale %1.15lf\n", fcf, new->n, sf);

  int *ccof = ccof_bwlp(new->n);
  new->dcof = dcof_bwlp(new->n, fcf);

  for (int i = 0; i < new->n; i++)
      new->ccof[i] = (float)ccof[i] * sf;

  new->dcof[0] = 0.f;

  free(ccof);

  printf("LPF coefficients are:\n");

  for (int i = 0; i < new->n; i++) {
      YughInfo("%f, %f\n", new->ccof[i], new->dcof[i]);
  }

  struct dsp_filter lpf;
  lpf.data = new;
  lpf.filter = dsp_iir_fillbuf;

  return lpf;
}

struct dsp_filter hpf_make(int poles, float freq)
{
  struct dsp_iir *new = malloc(sizeof(*new));
  *new = make_iir(3, 1);

  double fcf = new->freq*2/SAMPLERATE;
  double sf = sf_bwhp(new->n, fcf);

  int *ccof = ccof_bwhp(new->n);
  new->dcof = dcof_bwhp(new->n, fcf);

  for (int i = 0; i < new->n; i++)
      new->ccof[i] = ccof[i] * sf;

    for (int i = 0; i < new->n; i++) {
      printf("%f, %f\n", new->ccof[i], new->dcof[i]);
  }

  free(ccof);

  struct dsp_filter hpf;
  hpf.data = new;
  hpf.filter = dsp_iir_fillbuf;

  return hpf;
}

short fir_filter(struct dsp_fir *fir, short val)
{
    float ret = 0.f;
    fir->dx[fir->head] = (float)val/SHRT_MAX;

    for (int i = 0; i < fir->n; i++) {
        ret += fir->cof[i] * fir->dx[fir->head--];

        if (fir->head < 0) fir->head = fir->n-1;
    }

    return ret * SHRT_MAX;
}

void dsp_fir_fillbuf(struct dsp_fir *fir, short *out, int n)
{
    dsp_run(fir->in, out, n);

    for (int i = 0; i < n; i++) {
        short val = fir_filter(fir, out[i*CHANNELS]);
       // printf("%hd\n", val);

        for (int j = 0; j < CHANNELS; j++)
            out[i*CHANNELS + j] = val*5;
    }
}

struct dsp_filter lp_fir_make(float freq)
{
    struct dsp_fir fir;
    fir.freq = freq;
    fir.n = 9;
    fir.head = 0;
    double fcf = freq * 2 / SAMPLERATE;
    fir.dx = calloc(sizeof(float),  fir.n);
    fir.cof = fir_lp(fir.n, fcf);

    struct dsp_filter new;
    new.data = malloc(sizeof(fir));
    *(struct dsp_fir*)(new.data) = fir;
    new.filter = dsp_fir_fillbuf;

    for (int i = 0; i < fir.n; i++) {
        printf("%f\n", fir.cof[i]);
    }

    return new;
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

    for (int i = 0; i < n*CHANNELS; i++) {
        cbuf_push(&delay->buf, cache[i] / 2);
        buf[i] = cache[i] + cbuf_shift(&delay->buf);
    }
}
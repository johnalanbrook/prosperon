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
}

void am_mod(struct dsp_ammod *mod, short *c, int n)
{
    dsp_run(mod->ina, mod->abuf, n);
    dsp_run(mod->inb, mod->bbuf, n);

    for (int i = 0; i < n*CHANNELS; i++)
        c[i] = (mod->abuf[i]*mod->bbuf[i])>>15;
}

void fm_mod(float *in1, float *in2, float *out, int n)
{

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

    return new;
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

    return new;
}

struct dsp_filter dsp_filter(void *data, void (*filter)(void *data, short *out, int samples))
{
    struct dsp_filter new;
    new.data = data;
    new.filter = filter;
    return new;
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

  YughInfo("LPF coefficients are:", 0);

  for (int i = 0; i < new->n; i++)
      YughInfo("%f, %f", new->ccof[i], new->dcof[i]);

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

    for (int i = 0; i < new->n; i++)
      YughInfo("%f, %f", new->ccof[i], new->dcof[i]);

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

/* Get decay constant for a given pole */
/* Samples to decay 1 time constant is exp(-1/timeconstant) */
double tau2pole(double tau)
{
    return exp(-1/(tau*SAMPLERATE));
}

void dsp_adsr_fillbuf(struct dsp_adsr *adsr, short *out, int n)
{
    short val;

    for (int i = 0; i < n; i++) {
        if (adsr->time > adsr->rls) {
            // Totally decayed
            adsr->out = 0.f;

            goto fin;
        }

        if (adsr->time > adsr->sus) {
            // Release phase
            adsr->out = adsr->rls_t * adsr->out;

            goto fin;
        }

        if (adsr->time > adsr->dec) {
            // Sustain phase
            adsr->out = adsr->sus_pwr;

            goto fin;
        }

        if (adsr->time > adsr->atk) {
            // Decay phase
            adsr->out = (1 - adsr->dec_t) * adsr->sus_pwr + adsr->dec_t * adsr->out;

            goto fin;
        }

        // Attack phase
        adsr->out = (1-adsr->atk_t) + adsr->atk_t * adsr->out;



        fin:

        val = SHRT_MAX * adsr->out;
        out[i*CHANNELS] = out[i*CHANNELS+1] = val;
        adsr->time += (double)(1000.f / SAMPLERATE);
    }
}



struct dsp_filter make_adsr(unsigned int atk, unsigned int dec, unsigned int sus, unsigned int rls)
{
    struct dsp_adsr *adsr = calloc(sizeof(*adsr), 1);
    adsr->atk = atk;
    /* decay to 3 tau */
    adsr->atk_t = tau2pole(atk / 3000.f);

    adsr->dec = dec + adsr->atk;
    adsr->dec_t = tau2pole(dec / 3000.f);

    adsr->sus = sus + adsr->dec;
    adsr->sus_pwr = 0.8f;

    adsr->rls = rls + adsr->sus;
    adsr->rls_t = tau2pole(rls / 3000.f);

    return make_dsp(adsr, dsp_adsr_fillbuf);
}

struct dsp_filter make_reverb()
{

}

void dsp_reverb_fillbuf(struct dsp_reverb *r, short *out, int n)
{

}

struct dsp_filter dsp_make_compressor()
{
    struct dsp_filter filter;
    struct dsp_compressor new;

    new.ratio = 4000;
    new.atk = 50;
    new.rls = 250;
    new.target = 0.f;
    new.threshold = -3.f;
    new.atk_tau = tau2pole(new.atk / 3000.f);
    new.rls_tau = tau2pole(new.rls / 3000.f);

    struct dsp_compressor *c = malloc(sizeof(*c));
    *c = new;

    filter.data = c;
    filter.filter = dsp_compressor_fillbuf;

    return filter;
}

void dsp_compressor_fillbuf(struct dsp_compressor *comp, short *out, int n)
{
    float val;
    float db;
    db = comp->target * (val - comp->threshold) / comp->ratio;

    for (int i = 0; i < n; i++) {
        val = short2db(out[i*CHANNELS]);


        if (val < comp->threshold) {
            comp->target = comp->rls_tau * comp->target;

            val += db;
        } else {
            comp->target = (1 - comp->atk_tau) + comp->atk_tau * comp->target; // TODO: Bake in the 1 - atk_tau

            val -= db;
        }



        // Apply same compression to both channels
        out[i*CHANNELS] = out[i*CHANNELS+1] = db2short(val) * ( out[i*CHANNELS] > 0 ? 1 : -1);
    }
}

void dsp_pan(float *deg, short *out, int n)
{
    if (*deg < -100) *deg = -100.f;
    else if (*deg > 100) *deg = 100.f;

    if (*deg == 0.f) return;

    float db1, db2;
    float pct = *deg / 100.f;

    if (*deg > 0) {
        db1 = pct2db(1 - pct);
        db2 = pct2db(pct);
    } else {
        db1 = pct2db(1 + pct);
        db2 = pct2db(-1*pct);
    }


    for (int i = 0; i < n; i++) {
        double pct = *deg / 100.f;
        short L = out[i*CHANNELS];
        short R = out[i*CHANNELS +1];

        if (*deg > 0) {
            out[i*CHANNELS] = short_gain(L, db1);
            out[i*CHANNELS+1] = (R + short_gain(L, db2)) / 2;

            continue;
        }

        out[i*CHANNELS+1] = short_gain(R, db1);
        out[i*CHANNELS] = short_gain(L, db1) + short_gain(R, db2);
    }
}

void dsp_mono(void *p, short *out, int n)
{
    for (int i = 0; i < n; i++) {
        short val = (out[i*CHANNELS] + out[i*CHANNELS+1]) / 2;

        for (int j = 0; j < CHANNELS; j++)
            out[i*CHANNELS+j] = val;
    }
}

void dsp_bitcrush(void *p, short *out, int n)
{
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < CHANNELS; j++)
            out[i*CHANNELS+j] = (out[i*CHANNELS+j] | 0xFF); /* Mask out the lower 8 bits */
    }
}
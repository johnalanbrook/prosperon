#include "dsp.h"

#include "sound.h"
#include "limits.h"
#include "math.h"
#include "stdlib.h"
#include "iir.h"
#include "log.h"
#include "stb_ds.h"

#define PI 3.14159265

dsp_node *masterbus = NULL;

void interleave(soundbyte *a, soundbyte *b, soundbyte *stereo, int frames)
{
  for (int i = 0; i < frames; i++) {
    stereo[i*2] = a[i];
    stereo[i*2+1] = b[i];
  }
}

void mono_to_stero(soundbyte *a, soundbyte *stereo, int frames)
{
  interleave(a,a,stereo, frames);
}

void mono_expand(soundbyte *buffer, int to, int frames)
{
  soundbyte hold[frames];
  memcpy(hold, buffer, sizeof(soundbyte)*frames);

  for (int i = 0; i < frames; i++)
    for (int j = 0; j < to; j++)
      buffer[i*to+j] = hold[i];
}

dsp_node *dsp_mixer_node()
{
  return make_node(NULL, NULL);
}

void dsp_init()
{
  masterbus = dsp_limiter(1.0);
}

soundbyte *dsp_node_out(dsp_node *node)
{
  zero_soundbytes(node->cache, BUF_FRAMES*CHANNELS);
  
  if (node->off) return node->cache;

  /* Sum all inputs */
  for (int i = 0; i < arrlen(node->ins); i++) {
    soundbyte *out = dsp_node_out(node->ins[i]);
    sum_soundbytes(node->cache, out, BUF_FRAMES*CHANNELS);
  }
  
  /* If there's a filter, run it */
  if (!node->pass && node->proc)
    node->proc(node->data, node->cache, BUF_FRAMES);

  scale_soundbytes(node->cache, node->gain, BUF_FRAMES*CHANNELS);
  pan_frames(node->cache, node->pan, BUF_FRAMES);
    
  return node->cache;
}

void filter_am_mod(dsp_node *mod, soundbyte *buffer, int frames)
{
  soundbyte *m = dsp_node_out(mod);
  for (int i = 0; i < frames*CHANNELS; i++) buffer[i] *= m[i];
}

dsp_node *dsp_am_mod(dsp_node *mod)
{
  return make_node(mod, filter_am_mod);
}

/* Add b into a */
void sum_soundbytes(soundbyte *a, soundbyte *b, int samples)
{
  for (int i = 0; i < samples; i++) a[i] += b[i];
}

void norm_soundbytes(soundbyte *a, float lvl, int samples)
{
  float tar = lvl;
  float max = 0 ;
  for (int i = 0; i < samples; i++) max = (fabsf(a[i] > max) ? fabsf(a[i]) : max);
  float mult = max/tar;
  scale_soundbytes(a, mult, samples);
}

void scale_soundbytes(soundbyte *a, float scale, int samples)
{
  if (scale == 1) return;
  for (int i = 0; i < samples; i++) a[i] *= scale;
}

void zero_soundbytes(soundbyte *a, int samples)
{
  memset(a, 0, sizeof(soundbyte)*samples);
}

void set_soundbytes(soundbyte *a, soundbyte *b, int samples)
{
  zero_soundbytes(a, samples);
  sum_soundbytes(a,b,samples);
}

void dsp_node_run(dsp_node *node)
{
  zero_soundbytes(node->cache, BUF_FRAMES*CHANNELS);
  
  for (int i = 0; i < arrlen(node->ins); i++) {
    soundbyte *out = dsp_node_out(node->ins[i]);
    sum_soundbytes(node->cache, out, BUF_FRAMES);
  }
}


dsp_node *make_node(void *data, void (*proc)(void *in, soundbyte *out, int samples))
{
  dsp_node *self = malloc(sizeof(dsp_node));
  memset(self, 0, sizeof(*self));
  self->data = data;
  self->proc = proc;
  self->pass = 0;
  self->gain = 1;
  return self;
}

void node_free(dsp_node *node)
{
  unplug_node(node);
  if (node->data)
    if (node->data_free) node->data_free(node->data);
    else free(node->data);
  
  free(node);    
}

void plugin_node(dsp_node *from, dsp_node *to)
{
  if (from->out) return;
  arrput(to->ins, from);
  from->out = to;
}

/* Unplug the given node from its output */
void unplug_node(dsp_node *node) 
{
  if (!node->out) return;
  
  for (int i = 0; arrlen(node->out->ins); i++)
    if (node == node->out->ins[i]) {
      arrdelswap(node->out->ins, i);
      node->out = NULL;
      return;
    }
}

typedef struct {
  float amp;
  float freq;
  float phase; /* from 0 to 1, marking where we are */
  float (*filter)(float phase);
} phasor;

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

void filter_phasor(phasor *p, soundbyte *buffer, int frames)
{
  for (int i = 0; i < frames; i++) {
    buffer[i] = p->filter(p->phase) * p->amp;
    p->phase += p->freq/SAMPLERATE;
  }
  p->phase = p->phase - (int)p->phase;
  mono_expand(buffer, CHANNELS, frames);
}

dsp_node *dsp_phasor(float amp, float freq, float (*filter)(float))
{
  phasor *p = malloc(sizeof(*p));
  p->amp = amp;
  p->freq = freq;
  p->phase = 0;
  p->filter = filter;
  return make_node(p, filter_phasor);
}

void filter_rectify(void *data, soundbyte *out, int n)
{
  for (int i = 0; i < n; i++) out[i] = abs(out[i]);
}

dsp_node *dsp_rectify()
{
  return make_node(NULL, filter_rectify);
}

soundbyte sample_whitenoise()
{
  return ((float)rand()/(float)(RAND_MAX/2))-1;
}

void gen_whitenoise(void *data, soundbyte *out, int n)
{
  for (int i = 0; i < n; i++) out[i] = sample_whitenoise();
  mono_expand(out, CHANNELS, n);
}

dsp_node *dsp_whitenoise()
{
  return make_node(NULL, gen_whitenoise);
}

void gen_pinknoise(void *data, soundbyte *out, int n)
{
  double a[7] = {1.0, 0.0555179, 0.0750759, 0.1538520, 0.3104856, 0.5329522, 0.0168980};
  double b[7] = {0.99886, 0.99332, 0.969, 0.8665, 0.55, -0.7616, 0.115926};  

  for (int i = 0; i < n; i++) {
      double pink;
      double white = sample_whitenoise();

      for (int k = 0; k < 5; k++) {
        b[k] = a[k]*b[k] + white * b[k];
        pink += b[k];
      }

      pink += b[5] + white*0.5362;
      b[5] = white*0.115926;

      out[i] = pink;
    }
  mono_expand(out,CHANNELS,n);
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

dsp_node *dsp_pinknoise()
{
  return make_node(NULL, gen_pinknoise);
}

soundbyte iir_filter(struct dsp_iir iir, soundbyte val)
{
  iir.y[0] = 0.0;

  iir.x[0] = val;

  for (int i = 0; i < iir.n; i++)
    iir.y[0] += iir.a[i] * iir.x[i];

  for (int i = 1; i < iir.n; i++)
    iir.y[0] -= iir.b[i] * iir.y[i];

  /* Shift values in */
  for (int i = iir.n-1; i > 0; i--) {
    iir.x[i] = iir.x[i-1];
    iir.y[i] = iir.y[i-1];
  }
  
  return iir.y[0];
}

void filter_iir(struct dsp_iir *iir, soundbyte *buffer, int frames)
{
  for (int i = 0; i < frames; i++) {
    soundbyte v = iir_filter(*iir, buffer[i*CHANNELS]);
    for (int j = 0; j < CHANNELS; j++) buffer[i*CHANNELS+j] = v;
  }
}

dsp_node *dsp_lpf(float freq)
{
  struct dsp_iir *iir = malloc(sizeof(*iir));
  *iir = bqlp_dcof(2*freq/SAMPLERATE, 5);
  return make_node(iir, filter_iir);
}

dsp_node *dsp_hpf(float freq)
{
  struct dsp_iir *iir = malloc(sizeof(*iir));
  *iir = bqhp_dcof(2*freq/SAMPLERATE,5);
  return make_node(iir, filter_iir);
}

void filter_delay(delay *d, soundbyte *buf, int frames)
{
  for (int i = 0; i < frames*CHANNELS; i++) {
    buf[i] += ringshift(d->ring)*d->decay;
    ringpush(d->ring, buf[i]);
  }
}

dsp_node *dsp_delay(double sec, double decay)
{
  delay *d = malloc(sizeof(*d));
  d->ms_delay = sec;
  d->decay = decay;
  d->ring = NULL;
  d->ring = ringnew(d->ring, sec*CHANNELS*SAMPLERATE*2);    /* Circular buffer size is enough to have the delay */
  ringheader(d->ring)->write += CHANNELS*SAMPLERATE*sec;
  return make_node(d, filter_delay);
}

/* Get decay constant for a given pole */
/* Samples to decay 1 time constant is exp(-1/timeconstant) */
double tau2pole(double tau)
{
    return exp(-1/(tau*SAMPLERATE));
}

void dsp_adsr_fillbuf(struct dsp_adsr *adsr, soundbyte *out, int n)
{
    soundbyte val;

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



dsp_node *dsp_adsr(unsigned int atk, unsigned int dec, unsigned int sus, unsigned int rls)
{
    struct dsp_adsr *adsr = malloc(sizeof(*adsr));
    adsr->atk = atk;
    /* decay to 3 tau */
    adsr->atk_t = tau2pole(atk / 3000.f);

    adsr->dec = dec + adsr->atk;
    adsr->dec_t = tau2pole(dec / 3000.f);

    adsr->sus = sus + adsr->dec;
    adsr->sus_pwr = 0.8f;

    adsr->rls = rls + adsr->sus;
    adsr->rls_t = tau2pole(rls / 3000.f);

    return make_node(adsr, dsp_adsr_fillbuf);
}

void filter_noise_gate(float *floor, soundbyte *out, int frames)
{
  for (int i = 0; i < frames*CHANNELS; i++) out[i] = fabsf(out[i]) < *floor ? 0.0 : out[i];
}

dsp_node *dsp_noise_gate(float floor)
{
  float *v = malloc(sizeof(float));
  *v = floor;
  return make_node(v, filter_noise_gate);
}

void filter_limiter(float *ceil, soundbyte *out, int n)
{
  for (int i = 0; i < n*CHANNELS; i++) out[i] = fabsf(out[i]) > *ceil ?  *ceil : out[i];
}

dsp_node *dsp_limiter(float ceil)
{
  float *v = malloc(sizeof(float));
  *v = ceil;
  return make_node(v, filter_limiter);
}

void dsp_compressor_fillbuf(struct dsp_compressor *comp, soundbyte *out, int n)
{
    float val;
    float db;
    db = comp->target * (val - comp->threshold) / comp->ratio;

    for (int i = 0; i < n; i++) {
      val = float2db(out[i*CHANNELS]);

      if (val < comp->threshold) {
        comp->target = comp->rls_tau * comp->target;
        val += db;
      } else {
        comp->target = (1 - comp->atk_tau) + comp->atk_tau * comp->target; // TODO: Bake in the 1 - atk_tau
        val -= db;
      }

    // Apply same compression to both channels
    out[i*CHANNELS] = out[i*CHANNELS+1] = db2float(val) * ( out[i*CHANNELS] > 0 ? 1 : -1);
    }
}

dsp_node *dsp_compressor()
{
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
    return make_node(c, dsp_compressor_fillbuf);
}

/* Assumes 2 channels in a frame */
void pan_frames(soundbyte *out, float deg, int frames)
{
  if (deg == 0.f) return;
  if (deg < -100) deg = -100.f;
  else if (deg > 100) deg = 100.f;

  float db1, db2;
  float pct = deg / 100.f;

  if (deg > 0) {
    db1 = pct2db(1 - pct);
    db2 = pct2db(pct);
    for (int i = 0; i < frames; i++) {
      soundbyte L = out[i*2];
      soundbyte R = out[i*2+1];
      out[i*2] = fgain(L, db1);
      out[i*2+1] = (R + fgain(L, db2))/2;
    }
  } else {
    db1 = pct2db(1 + pct);
    db2 = pct2db(-1*pct);
    for (int i = 0; i < frames; i++) {
      soundbyte L = out[i*2];
      soundbyte R = out[i*2+1];
      out[i*2+1] = fgain(R,db1);
      out[i*2] = fgain(L, db1) + fgain(R, db2);
    }
  }
}

void dsp_mono(void *p, soundbyte *out, int n)
{
    for (int i = 0; i < n; i++) {
        soundbyte val = (out[i*CHANNELS] + out[i*CHANNELS+1]) / 2;

        for (int j = 0; j < CHANNELS; j++)
            out[i*CHANNELS+j] = val;
    }
}

struct bitcrush {
  float sr;
  float depth;
};

#define ROUND(f) ((float)((f>0.0)?floor(f+0.5):ceil(f-0.5)))
void filter_bitcrush(struct bitcrush *b, soundbyte *out, int frames)
{
  int max = pow(2,b->depth) - 1;
  int step = SAMPLERATE/b->sr;

  int i = 0;
  while (i < frames) {
    float left = ROUND((out[0]+1.0)*max)/(max-1.0);
    float right = ROUND((out[1]+1.0)*max)/(max-1.0);
    
    for (int j = 0; j < step && i < frames; j++) {
      out[0] = left;
      out[1] = right;
      out += CHANNELS;
      i++;
    }
  }
}

dsp_node *dsp_bitcrush(float sr, float res)
{
  struct bitcrush *b = malloc(sizeof(*b));
  b->sr = sr;
  b->depth = res;
  return make_node(b, filter_bitcrush);
}

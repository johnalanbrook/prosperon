#ifndef DSP_H
#define DSP_H

extern int SAMPLERATE;
extern int BUF_FRAMES;
extern int CHANNELS;

#include "sound.h"
#include "cbuf.h"
#include "script.h"
#include "iir.h"

/* a DSP node, when processed, sums its inputs, and stores the result of proc in its cache */
typedef struct dsp_node {
  void (*proc)(void *dsp, soundbyte *buf, int samples); /* processor */
  void *data; /* Node specific data to use in the proc function, passed in as dsp */
  void (*data_free)(void *data);
  soundbyte *cache;
  struct dsp_node **ins; /* Array of in nodes */
  struct dsp_node *out; /* node this one is connected to */
  int pass; /* True if the filter should be bypassed */
  int off; /* True if the filter shouldn't output */
  float gain; /* Between 0 and 1, to attenuate this output */
  float pan; /* Between -100 and +100, panning left to right in the speakers */
} dsp_node;

void dsp_init();

/* Get the output of a node */
soundbyte *dsp_node_out(dsp_node *node);
void dsp_node_run(dsp_node *node);
dsp_node *make_node(void *data, void (*proc)(void *data, soundbyte *out, int samples), void (*fr)(void *data));
void plugin_node(dsp_node *from, dsp_node *to);
void unplug_node(dsp_node *node);
void node_free(dsp_node *node);
void dsp_node_free(dsp_node *node);
void filter_iir(struct dsp_iir *iir, soundbyte *buffer, int frames);

void scale_soundbytes(soundbyte *a, float scale, int frames);
void sum_soundbytes(soundbyte *a, soundbyte *b, int frames);
void zero_soundbytes(soundbyte *a, int frames);
void set_soundbytes(soundbyte *a, soundbyte *b, int frames);

dsp_node *dsp_mixer_node();
dsp_node *dsp_am_mod(dsp_node *mod);
dsp_node *dsp_rectify();

extern dsp_node *masterbus;

dsp_node *dsp_hpf(float freq);
dsp_node *dsp_lpf(float freq);

/* atk, dec, sus, rls specify the time, in miliseconds, the phase begins */
typedef struct dsp_adsr {
    unsigned int atk;
    double atk_t;
    unsigned int dec;
    double dec_t;
    unsigned int sus;
    float sus_pwr; // Between 0 and 1
    unsigned int rls;
    double rls_t;

    double time; /* Current time of the filter */
    float out;
} adsr;

dsp_node *dsp_adsr(unsigned int atk, unsigned int dec, unsigned int sus, unsigned int rls);

typedef struct {
  unsigned int ms_delay;
  float decay; /* Each echo should be multiplied by this number */
  soundbyte *ring;
} delay;

dsp_node *dsp_delay(double sec, double decay);
dsp_node *dsp_fwd_delay(double sec, double decay);
dsp_node *dsp_pitchshift(float octaves);

typedef struct dsp_compressor {
    double ratio;
    double threshold;
    float target;
    unsigned int atk; /* Milliseconds */
    double atk_tau;
    unsigned int rls; /* MIlliseconds */
    double rls_tau;
} compressor;

dsp_node *dsp_compressor();

dsp_node *dsp_limiter(float ceil);
dsp_node *dsp_noise_gate(float floor);

struct phasor phasor_make(unsigned int sr, float freq);

dsp_node *dsp_whitenoise();
dsp_node *dsp_pinknoise();
dsp_node *dsp_rednoise();

float sin_phasor(float p);
float square_phasor(float p);
float saw_phasor(float p);
float tri_phasor(float p);

dsp_node *dsp_reverb();
dsp_node *dsp_sinewave(float amp, float freq);
dsp_node *dsp_square(float amp, float freq, int sr, int ch);

typedef struct {
  float amp;
  float freq;
  float phase; /* from 0 to 1, marking where we are */
  float (*filter)(float phase);
} phasor;

typedef struct bitcrush {
  float sr;
  float depth;
} bitcrush;

dsp_node *dsp_bitcrush(float sr, float res);
void dsp_mono(void *p, soundbyte *out, int n);
void pan_frames(soundbyte *out, float deg, int frames);

#endif

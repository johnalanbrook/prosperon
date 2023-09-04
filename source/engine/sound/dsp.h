#ifndef DSP_H
#define DSP_H

#define SAMPLERATE 44100
#define BUF_FRAMES 2048 /* At 48k, 128 needed for 240fps consistency */
#define CHANNELS 2

#include "sound.h"

//#include "circbuf.h"

struct dsp_iir;


void dsp_rectify(soundbyte *in, soundbyte *out, int n);

struct dsp_filter {
    void (*filter)(void *data, soundbyte *out, int samples);
    void *data;

    int inputs;
    struct dsp_filter *in[6];

    soundbyte cache[CHANNELS*BUF_FRAMES];
    int dirty;
};

struct dsp_filter dsp_filter(void *data, void (*filter)(void *data, soundbyte *out, int samples));

struct dsp_fir {
    float freq;
    int n;
    int head;
    float *cof;
    float *dx;

    struct dsp_filter in;
};

void dsp_filter_addin(struct dsp_filter filter, struct dsp_filter *in);

struct dsp_filter lp_fir_make(float freq);

void dsp_iir_fillbuf(struct dsp_iir *iir, soundbyte *out, int n);
struct dsp_filter hpf_make(int poles, float freq);
struct dsp_filter lpf_make(int poles, float freq);
struct dsp_filter bpf_make(int poles, float freq1, float freq2);
struct dsp_filter npf_make(int poles, float freq1, float freq2);

/* atk, dec, sus, rls specify the time, in miliseconds, the phase begins */
struct dsp_adsr {
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
};

void dsp_adsr_fillbuf(struct dsp_adsr *adsr, soundbyte *out, int n);
struct dsp_filter make_adsr(unsigned int atk, unsigned int dec, unsigned int sus, unsigned int rls);

struct dsp_delay {
    unsigned int ms_delay;
//    struct circbuf buf;
    struct dsp_filter in;
};

struct dsp_delay dsp_delay_make(unsigned int ms_delay);
void dsp_delay_filbuf(struct dsp_delay *delay, soundbyte *buf, int n);

struct dsp_ammod {
    struct dsp_filter ina;
    struct dsp_filter inb;
    soundbyte abuf[BUF_FRAMES*CHANNELS];
    soundbyte bbuf[BUF_FRAMES*CHANNELS];
};

struct dsp_compressor {
    double ratio;
    double threshold;
    float target;
    unsigned int atk; /* Milliseconds */
    double atk_tau;
    unsigned int rls; /* MIlliseconds */
    double rls_tau;
};

struct dsp_filter dsp_make_compressor();
void dsp_compressor_fillbuf(struct dsp_compressor *comp, soundbyte *out, int n);

struct dsp_limiter {

};

struct dsp_filter dsp_make_limiter();
void dsp_limiter_fillbuf(struct dsp_limiter *lim, soundbyte *out, int n);


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
    soundbyte *cache;
};

struct wav;

struct wav gen_sine(float amp, float freq, int sr, int ch);
struct wav gen_square(float amp, float freq, int sr, int ch);
struct wav gen_triangle(float amp, float freq, int sr, int ch);
struct wav gen_saw(float amp, float freq, int sr, int ch);

void gen_whitenoise(void *data, soundbyte *out, int n);
void gen_pinknoise(void *data, soundbyte *out, int n);


float sin_phasor(float p);
float square_phasor(float p);
float saw_phasor(float p);
float tri_phasor(float p);

void osc_fillbuf(struct osc *osc, soundbyte *buf, int n);

void am_mod(struct dsp_ammod *mod, soundbyte *c, int n);

struct dsp_reverb {
    unsigned int time; /* Time in miliseconds for the sound to decay out */
};

struct dsp_filter make_reverb();
void dsp_reverb_fillbuf(struct dsp_reverb *r, soundbyte *out, int n);

void dsp_pan(float *deg, soundbyte *out, int n);

void dsp_mono(void *p, soundbyte *out, int n);

void dsp_bitcrush(void *p, soundbyte *out, int n);

void dsp_run(struct dsp_filter filter, soundbyte *out, int n);

#endif

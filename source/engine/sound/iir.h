#ifndef IIR_H
#define IIR_H

#include "sound.h"

struct dsp_iir {
    int n;  // Amount of constants
    float *a;
    float *b;
    float *x;
    float *y;
};

struct dsp_iir make_iir(int order);

double *binomial_mult( int n, double *p );
double *trinomial_mult( int n, double *b, double *c );

double *dcof_bwlp( int n, double fcf );
double *dcof_bwhp( int n, double fcf );
double *dcof_bwbp( int n, double f1f, double f2f );
double *dcof_bwbs( int n, double f1f, double f2f );

int *ccof_bwlp( int n );
int *ccof_bwhp( int n );
int *ccof_bwbp( int n );
double *ccof_bwbs( int n, double f1f, double f2f );

double sf_bwlp( int n, double fcf );
double sf_bwhp( int n, double fcf );
double sf_bwbp( int n, double f1f, double f2f );
double sf_bwbs( int n, double f1f, double f2f );

float *fir_lp(int n, double fcf);
float *fir_hp(int n, double fcf);
float *fir_bpf(int n, double fcf1, double fcf2);

struct dsp_iir sp_lp(double fcf);
struct dsp_iir sp_hp(double fcf);

double chevy_pct_to_e(double pct);

struct dsp_iir make_iir(int order);
struct dsp_iir bqlp_dcof(double fcf, float Q);
struct dsp_iir bqhp_dcof(double fcf, float Q);
struct dsp_iir bqbpq_dcof(double fcf, float Q);
struct dsp_iir bqbp_dcof(double fcf, float Q);
struct dsp_iir bqnotch_dcof(double fcf, float Q);
struct dsp_iir bqapf_dcof(double fcf, float Q);
struct dsp_iir bqpeq_dcof(double fcf, float Q, float dbgain);
struct dsp_iir bqls_dcof(double fcf, float Q, float dbgain);
struct dsp_iir bqhs_dcof(double fcf, float Q, float dbgain);

struct dsp_iir p2_bwlp(double fcf);
struct dsp_iir p2_bwhp(double fcf);
struct dsp_iir p2_cdlp(double fcf);
struct dsp_iir p2_cdhp(double fcf);
struct dsp_iir p2_beslp(double fcf);
struct dsp_iir p2_beshp(double fcf);

struct dsp_iir che_lp(int order, double fcf, double e);
struct dsp_iir che_hp(int order, double fcf, double e);
struct dsp_iir che_bp(int order, double s, double fcf1, double fcf2, double e);
struct dsp_iir che_notch(int order, double s, double fcf1, double fcf2, double e);

#endif

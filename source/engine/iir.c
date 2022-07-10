/*
 *                            COPYRIGHT
 *
 *  liir - Recursive digital filter functions
 *  Copyright (C) 2007 Exstrom Laboratories LLC
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  A copy of the GNU General Public License is available on the internet at:
 *
 *  http://www.gnu.org/copyleft/gpl.html
 *
 *  or you can write to:
 *
 *  The Free Software Foundation, Inc.
 *  675 Mass Ave
 *  Cambridge, MA 02139, USA
 *
 *  You can contact Exstrom Laboratories LLC via Email at:
 *
 *  stefan(AT)exstrom.com
 *
 *  or you can write to:
 *
 *  Exstrom Laboratories LLC
 *  P.O. Box 7651
 *  Longmont, CO 80501, USA
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "limits.h"
#include "iir.h"
#include "dsp.h"

/**********************************************************************
  binomial_mult - multiplies a series of binomials together and returns
  the coefficients of the resulting polynomial.

  The multiplication has the following form:

  (x+p[0])*(x+p[1])*...*(x+p[n-1])

  The p[i] coefficients are assumed to be complex and are passed to the
  function as a pointer to an array of doubles of length 2n.

  The resulting polynomial has the following form:

  x^n + a[0]*x^n-1 + a[1]*x^n-2 + ... +a[n-2]*x + a[n-1]

  The a[i] coefficients can in general be complex but should in most
  cases turn out to be real. The a[i] coefficients are returned by the
  function as a pointer to an array of doubles of length 2n. Storage
  for the array is allocated by the function and should be freed by the
  calling program when no longer needed.

  Function arguments:

  n  -  The number of binomials to multiply
  p  -  Pointer to an array of doubles where p[2i] (i=0...n-1) is
        assumed to be the real part of the coefficient of the ith binomial
        and p[2i+1] is assumed to be the imaginary part. The overall size
        of the array is then 2n.
*/

double *binomial_mult( int n, double *p )
{
    int i, j;
    double *a;

    a = (double *)calloc( 2 * n, sizeof(double) );
    if( a == NULL ) return( NULL );

    for( i = 0; i < n; ++i )
    {
	for( j = i; j > 0; --j )
	{
	    a[2*j] += p[2*i] * a[2*(j-1)] - p[2*i+1] * a[2*(j-1)+1];
	    a[2*j+1] += p[2*i] * a[2*(j-1)+1] + p[2*i+1] * a[2*(j-1)];
	}
	a[0] += p[2*i];
	a[1] += p[2*i+1];
    }
    return( a );
}


/**********************************************************************
  trinomial_mult - multiplies a series of trinomials together and returns
  the coefficients of the resulting polynomial.

  The multiplication has the following form:

  (x^2 + b[0]x + c[0])*(x^2 + b[1]x + c[1])*...*(x^2 + b[n-1]x + c[n-1])

  The b[i] and c[i] coefficients are assumed to be complex and are passed
  to the function as a pointers to arrays of doubles of length 2n. The real
  part of the coefficients are stored in the even numbered elements of the
  array and the imaginary parts are stored in the odd numbered elements.

  The resulting polynomial has the following form:

  x^2n + a[0]*x^2n-1 + a[1]*x^2n-2 + ... +a[2n-2]*x + a[2n-1]

  The a[i] coefficients can in general be complex but should in most cases
  turn out to be real. The a[i] coefficients are returned by the function as
  a pointer to an array of doubles of length 4n. The real and imaginary
  parts are stored, respectively, in the even and odd elements of the array.
  Storage for the array is allocated by the function and should be freed by
  the calling program when no longer needed.

  Function arguments:

  n  -  The number of trinomials to multiply
  b  -  Pointer to an array of doubles of length 2n.
  c  -  Pointer to an array of doubles of length 2n.
*/

double *trinomial_mult( int n, double *b, double *c )
{
    int i, j;
    double *a;

    a = (double *)calloc( 4 * n, sizeof(double) );
    if( a == NULL ) return( NULL );

    a[2] = c[0];
    a[3] = c[1];
    a[0] = b[0];
    a[1] = b[1];

    for( i = 1; i < n; ++i )
    {
	a[2*(2*i+1)]   += c[2*i]*a[2*(2*i-1)]   - c[2*i+1]*a[2*(2*i-1)+1];
	a[2*(2*i+1)+1] += c[2*i]*a[2*(2*i-1)+1] + c[2*i+1]*a[2*(2*i-1)];

	for( j = 2*i; j > 1; --j )
	{
	    a[2*j]   += b[2*i] * a[2*(j-1)]   - b[2*i+1] * a[2*(j-1)+1] +
		c[2*i] * a[2*(j-2)]   - c[2*i+1] * a[2*(j-2)+1];
	    a[2*j+1] += b[2*i] * a[2*(j-1)+1] + b[2*i+1] * a[2*(j-1)] +
		c[2*i] * a[2*(j-2)+1] + c[2*i+1] * a[2*(j-2)];
	}

	a[2] += b[2*i] * a[0] - b[2*i+1] * a[1] + c[2*i];
	a[3] += b[2*i] * a[1] + b[2*i+1] * a[0] + c[2*i+1];
	a[0] += b[2*i];
	a[1] += b[2*i+1];
    }

    return( a );
}


/**********************************************************************
  dcof_bwlp - calculates the d coefficients for a butterworth lowpass
  filter. The coefficients are returned as an array of doubles.

*/

double *dcof_bwlp( int n, double fcf )
{
    int k;            // loop variables
    double theta;     // M_PI * fcf / 2.0
    double st;        // sine of theta
    double ct;        // cosine of theta
    double parg;      // pole angle
    double sparg;     // sine of the pole angle
    double cparg;     // cosine of the pole angle
    double a;         // workspace variable
    double *rcof;     // binomial coefficients
    double *dcof;     // dk coefficients

    rcof = (double *)calloc( 2 * n, sizeof(double) );
    if( rcof == NULL ) return( NULL );

    theta = M_PI * fcf;
    st = sin(theta);
    ct = cos(theta);

    for( k = 0; k < n; ++k )
    {
	parg = M_PI * (double)(2*k+1)/(double)(2*n);
	sparg = sin(parg);
	cparg = cos(parg);
	a = 1.0 + st*sparg;
	rcof[2*k] = -ct/a;
	rcof[2*k+1] = -st*cparg/a;
    }

    dcof = binomial_mult( n, rcof );
    free( rcof );

    dcof[1] = dcof[0];
    dcof[0] = 1.0;
    for( k = 3; k <= n; ++k )
        dcof[k] = dcof[2*k-2];
    return( dcof );
}

/**********************************************************************
  dcof_bwhp - calculates the d coefficients for a butterworth highpass
  filter. The coefficients are returned as an array of doubles.

*/

double *dcof_bwhp( int n, double fcf )
{
    return( dcof_bwlp( n, fcf ) );
}


/**********************************************************************
  dcof_bwbp - calculates the d coefficients for a butterworth bandpass
  filter. The coefficients are returned as an array of doubles.

*/

double *dcof_bwbp( int n, double f1f, double f2f )
{
    int k;            // loop variables
    double theta;     // M_PI * (f2f - f1f) / 2.0
    double cp;        // cosine of phi
    double st;        // sine of theta
    double ct;        // cosine of theta
    double s2t;       // sine of 2*theta
    double c2t;       // cosine 0f 2*theta
    double *rcof;     // z^-2 coefficients
    double *tcof;     // z^-1 coefficients
    double *dcof;     // dk coefficients
    double parg;      // pole angle
    double sparg;     // sine of pole angle
    double cparg;     // cosine of pole angle
    double a;         // workspace variables

    cp = cos(M_PI * (f2f + f1f) / 2.0);
    theta = M_PI * (f2f - f1f) / 2.0;
    st = sin(theta);
    ct = cos(theta);
    s2t = 2.0*st*ct;        // sine of 2*theta
    c2t = 2.0*ct*ct - 1.0;  // cosine of 2*theta

    rcof = (double *)calloc( 2 * n, sizeof(double) );
    tcof = (double *)calloc( 2 * n, sizeof(double) );

    for( k = 0; k < n; ++k )
    {
	parg = M_PI * (double)(2*k+1)/(double)(2*n);
	sparg = sin(parg);
	cparg = cos(parg);
	a = 1.0 + s2t*sparg;
	rcof[2*k] = c2t/a;
	rcof[2*k+1] = s2t*cparg/a;
	tcof[2*k] = -2.0*cp*(ct+st*sparg)/a;
	tcof[2*k+1] = -2.0*cp*st*cparg/a;
    }

    dcof = trinomial_mult( n, tcof, rcof );
    free( tcof );
    free( rcof );

    dcof[1] = dcof[0];
    dcof[0] = 1.0;
    for( k = 3; k <= 2*n; ++k )
        dcof[k] = dcof[2*k-2];
    return( dcof );
}

/**********************************************************************
  dcof_bwbs - calculates the d coefficients for a butterworth bandstop
  filter. The coefficients are returned as an array of doubles.

*/

double *dcof_bwbs( int n, double f1f, double f2f )
{
    int k;            // loop variables
    double theta;     // M_PI * (f2f - f1f) / 2.0
    double cp;        // cosine of phi
    double st;        // sine of theta
    double ct;        // cosine of theta
    double s2t;       // sine of 2*theta
    double c2t;       // cosine 0f 2*theta
    double *rcof;     // z^-2 coefficients
    double *tcof;     // z^-1 coefficients
    double *dcof;     // dk coefficients
    double parg;      // pole angle
    double sparg;     // sine of pole angle
    double cparg;     // cosine of pole angle
    double a;         // workspace variables

    cp = cos(M_PI * (f2f + f1f) / 2.0);
    theta = M_PI * (f2f - f1f) / 2.0;
    st = sin(theta);
    ct = cos(theta);
    s2t = 2.0*st*ct;        // sine of 2*theta
    c2t = 2.0*ct*ct - 1.0;  // cosine 0f 2*theta

    rcof = (double *)calloc( 2 * n, sizeof(double) );
    tcof = (double *)calloc( 2 * n, sizeof(double) );

    for( k = 0; k < n; ++k )
    {
	parg = M_PI * (double)(2*k+1)/(double)(2*n);
	sparg = sin(parg);
	cparg = cos(parg);
	a = 1.0 + s2t*sparg;
	rcof[2*k] = c2t/a;
	rcof[2*k+1] = -s2t*cparg/a;
	tcof[2*k] = -2.0*cp*(ct+st*sparg)/a;
	tcof[2*k+1] = 2.0*cp*st*cparg/a;
    }

    dcof = trinomial_mult( n, tcof, rcof );
    free( tcof );
    free( rcof );

    dcof[1] = dcof[0];
    dcof[0] = 1.0;
    for( k = 3; k <= 2*n; ++k )
        dcof[k] = dcof[2*k-2];
    return( dcof );
}

/**********************************************************************
  ccof_bwlp - calculates the c coefficients for a butterworth lowpass
  filter. The coefficients are returned as an array of integers.

*/

int *ccof_bwlp( int n )
{
    int *ccof;
    int m;
    int i;

    ccof = (int *)calloc( n+1, sizeof(int) );
    if( ccof == NULL ) return( NULL );

    ccof[0] = 1;
    ccof[1] = n;
    m = n/2;
    for( i=2; i <= m; ++i)
    {
        ccof[i] = (n-i+1)*ccof[i-1]/i;
        ccof[n-i]= ccof[i];
    }
    ccof[n-1] = n;
    ccof[n] = 1;

    return( ccof );
}

/**********************************************************************
  ccof_bwhp - calculates the c coefficients for a butterworth highpass
  filter. The coefficients are returned as an array of integers.

*/

int *ccof_bwhp( int n )
{
    int *ccof;
    int i;

    ccof = ccof_bwlp( n );
    if( ccof == NULL ) return( NULL );

    for( i = 0; i <= n; ++i)
        if( i % 2 ) ccof[i] = -ccof[i];

    return( ccof );
}

/**********************************************************************
  ccof_bwbp - calculates the c coefficients for a butterworth bandpass
  filter. The coefficients are returned as an array of integers.

*/

int *ccof_bwbp( int n )
{
    int *tcof;
    int *ccof;
    int i;

    ccof = (int *)calloc( 2*n+1, sizeof(int) );
    if( ccof == NULL ) return( NULL );

    tcof = ccof_bwhp(n);
    if( tcof == NULL ) return( NULL );

    for( i = 0; i < n; ++i)
    {
        ccof[2*i] = tcof[i];
        ccof[2*i+1] = 0.0;
    }
    ccof[2*n] = tcof[n];

    free( tcof );
    return( ccof );
}

/**********************************************************************
  ccof_bwbs - calculates the c coefficients for a butterworth bandstop
  filter. The coefficients are returned as an array of integers.

*/

double *ccof_bwbs( int n, double f1f, double f2f )
{
    double alpha;
    double *ccof;
    int i, j;

    alpha = -2.0 * cos(M_PI * (f2f + f1f) / 2.0) / cos(M_PI * (f2f - f1f) / 2.0);

    ccof = (double *)calloc( 2*n+1, sizeof(double) );

    ccof[0] = 1.0;

    ccof[2] = 1.0;
    ccof[1] = alpha;

    for( i = 1; i < n; ++i )
    {
	ccof[2*i+2] += ccof[2*i];
	for( j = 2*i; j > 1; --j )
	    ccof[j+1] += alpha * ccof[j] + ccof[j-1];

	ccof[2] += alpha * ccof[1] + 1.0;
	ccof[1] += alpha;
    }

    return( ccof );
}

/**********************************************************************
  sf_bwlp - calculates the scaling factor for a butterworth lowpass filter.
  The scaling factor is what the c coefficients must be multiplied by so
  that the filter response has a maximum value of 1.

*/

double sf_bwlp( int n, double fcf )
{
    int m, k;         // loop variables
    double omega;     // M_PI * fcf
    double fomega;    // function of omega
    double parg0;     // zeroth pole angle
    double sf;        // scaling factor

    omega = M_PI * fcf;
    fomega = sin(omega);
    parg0 = M_PI / (double)(2*n);

    m = n / 2;
    sf = 1.0;
    for( k = 0; k < n/2; ++k )
        sf *= 1.0 + fomega * sin((double)(2*k+1)*parg0);

    fomega = sin(omega / 2.0);

    if( n % 2 ) sf *= fomega + cos(omega / 2.0);
    sf = pow( fomega, n ) / sf;

    return sf;
}

/**********************************************************************
  sf_bwhp - calculates the scaling factor for a butterworth highpass filter.
  The scaling factor is what the c coefficients must be multiplied by so
  that the filter response has a maximum value of 1.

*/

double sf_bwhp( int n, double fcf )
{
    int m, k;         // loop variables
    double omega;     // M_PI * fcf
    double fomega;    // function of omega
    double parg0;     // zeroth pole angle
    double sf;        // scaling factor

    omega = M_PI * fcf;
    fomega = sin(omega);
    parg0 = M_PI / (double)(2*n);

    m = n / 2;
    sf = 1.0;
    for( k = 0; k < n/2; ++k )
        sf *= 1.0 + fomega * sin((double)(2*k+1)*parg0);

    fomega = cos(omega / 2.0);

    if( n % 2 ) sf *= fomega + sin(omega / 2.0);
    sf = pow( fomega, n ) / sf;

    return(sf);
}

/**********************************************************************
  sf_bwbp - calculates the scaling factor for a butterworth bandpass filter.
  The scaling factor is what the c coefficients must be multiplied by so
  that the filter response has a maximum value of 1.

*/

double sf_bwbp( int n, double f1f, double f2f )
{
    int k;            // loop variables
    double ctt;       // cotangent of theta
    double sfr, sfi;  // real and imaginary parts of the scaling factor
    double parg;      // pole angle
    double sparg;     // sine of pole angle
    double cparg;     // cosine of pole angle
    double a, b, c;   // workspace variables

    ctt = 1.0 / tan(M_PI * (f2f - f1f) / 2.0);
    sfr = 1.0;
    sfi = 0.0;

    for( k = 0; k < n; ++k )
    {
	parg = M_PI * (double)(2*k+1)/(double)(2*n);
	sparg = ctt + sin(parg);
	cparg = cos(parg);
	a = (sfr + sfi)*(sparg - cparg);
	b = sfr * sparg;
	c = -sfi * cparg;
	sfr = b - c;
	sfi = a - b - c;
    }

    return( 1.0 / sfr );
}

/**********************************************************************
  sf_bwbs - calculates the scaling factor for a butterworth bandstop filter.
  The scaling factor is what the c coefficients must be multiplied by so
  that the filter response has a maximum value of 1.

*/

double sf_bwbs( int n, double f1f, double f2f )
{
    int k;            // loop variables
    double tt;        // tangent of theta
    double sfr, sfi;  // real and imaginary parts of the scaling factor
    double parg;      // pole angle
    double sparg;     // sine of pole angle
    double cparg;     // cosine of pole angle
    double a, b, c;   // workspace variables

    tt = tan(M_PI * (f2f - f1f) / 2.0);
    sfr = 1.0;
    sfi = 0.0;

    for( k = 0; k < n; ++k )
    {
	parg = M_PI * (double)(2*k+1)/(double)(2*n);
	sparg = tt + sin(parg);
	cparg = cos(parg);
	a = (sfr + sfi)*(sparg - cparg);
	b = sfr * sparg;
	c = -sfi * cparg;
	sfr = b - c;
	sfi = a - b - c;
    }

    return( 1.0 / sfr );
}




float *fir_lp(int n, double fcf)
{
    float *ret = malloc(n * sizeof(*ret));

    double d1 = ((double)n - 1.f) / 2.f;
    double d2, fc, h;

    fc = fcf * M_PI;

    for (int i = 0; i < n; i++) {
        d2 = (double)i - d1;
        h = d2 == 0.f ? fc / M_PI : sin(fc * d2) / (M_PI * d2);
        ret[i] = h;
    }

    return ret;
}

float *fir_hp(int n, double fcf)
{
    float *ret = malloc(n * sizeof(*ret));

    double d1 = ((double)n - 1.f) / 2.f;
    double d2, fc, h;

    fc = fcf * M_PI;

    for (int i = 0; i < n; i++) {
        d2 = (double)i - d1;
        h = d2 == 0.f ? 1.f - fc / M_PI : (sin(M_PI * d2) - sin(fc * d2)) / (M_PI * d2);
        ret[i] = h;
    }

    return ret;
}

float *fir_bpf(int n, double fcf1, double fcf2)
{
    float *ret = malloc(n * sizeof(*ret));

    double d1 = ((double)n - 1.f) / 2.f;
    double d2, fc1, fc2, h;

    fc1 = fcf1 * M_PI;
    fc2 = fcf2 * M_PI;

    for (int i = 0; i < n; i++) {
        d2 = (double)i - d1;
        h = d2 == 0.f ? (fc2 - fc1) / M_PI : (sin(fc2 * d2) - sin(fc1 * d2)) / (M_PI * d2);
        ret[i] = h;
    }

    return ret;
}









/* Biquad filters */

struct dsp_iir biquad_iir()
{
    struct dsp_iir new;
    new.dcof = malloc(sizeof(float) * 3);
    new.ccof = malloc(sizeof(float) * 3);
    new.dx = malloc(sizeof(float) * 3);
    new.dy = malloc(sizeof(float) * 3);
    new.n = 3;
    return new;
}

void biquad_iir_fill(struct dsp_iir bq, double *a, double *b)
{
    bq.ccof[0] = (b[0] / a[0]);
    bq.ccof[1] = (b[1] / a[0]);
    bq.ccof[2] = (b[2] / a[0]);
    bq.dcof[0] = 0.f;
    bq.dcof[1] = (a[1] / a[0]);
    bq.dcof[2] = (a[2] / a[0]);
}

struct dsp_iir bqlp_dcof(double fcf, float Q)
{
    double w0 = M_PI * fcf;

    double a[3];
    double b[3];
    double az = sin(w0) / (2 * Q);

    b[0] = (1 - cos(w0)) / 2;
    b[1] = 1 - cos(w0);
    b[2] = b[0];

    a[0] = 1 + az;
    a[1] = -2 * cos(w0);
    a[2] = 1 - az;

    struct dsp_iir new = biquad_iir();
    biquad_iir_fill(new, a, b);
    return new;
}

struct dsp_iir bqhp_dcof(double fcf, float Q)
{
    double w0 = M_PI * fcf;
    double a[3];
    double b[3];
    double az = sin(w0) / (2 * Q);

    b[0] = (1 + cos(w0)) / 2;
    b[1] = -(1 + cos(w0));
    b[2] = b[0];

    a[0] = 1 + az;
    a[1] = -2 * cos(w0);
    a[2] = 1 - az;

    struct dsp_iir new = biquad_iir();
    biquad_iir_fill(new, a, b);
    return new;
}

struct dsp_iir bqbp_dcof(double fcf, float Q)
{
    double w0 = M_PI * fcf;
    double a[3];
    double b[3];
    double az = sin(w0) / (2 * Q);

    b[0] = az;
    b[1] = 0;
    b[2] = -b[0];

    a[0] = 1 + az;
    a[1] = -2 * cos(w0);
    a[2] = 1 - az;

    struct dsp_iir new = biquad_iir();
    biquad_iir_fill(new, a, b);
    return new;}

struct dsp_iir bqnotch_dcof(double fcf, float Q)
{
    double w0 = M_PI * fcf;
    double a[3];
    double b[3];
    double az = sin(w0) / (2 * Q);

    b[0] = 1;
    b[1] = -2 * cos(w0);
    b[2] = 1;

    a[0] = 1 + az;
    a[1] = -2 * cos(w0);
    a[2] = 1 - az;

    struct dsp_iir new = biquad_iir();
    biquad_iir_fill(new, a, b);
    return new;
}

struct dsp_iir bqapf_dcof(double fcf, float Q)
{
    double w0 = M_PI * fcf;
    double a[3];
    double b[3];
    double az = sin(w0) / (2 * Q);

    b[0] = 1 - az;
    b[1] = -2 * cos(w0);
    b[2] = 1 + az;

    a[0] = 1 + az;
    a[1] = -2 * cos(w0);
    a[2] = 1 - az;

    struct dsp_iir new = biquad_iir();
    biquad_iir_fill(new, a, b);
    return new;
}

struct dsp_iir bqpeq_dcof(double fcf, float Q, float dbgain)
{
    double w0 = M_PI * fcf;
    double a[3];
    double b[3];
    double az = sin(w0) / (2 * Q);
    double A = dbgain * 10 / 40;

    b[0] = 1+ az * A;
    b[1] = -2 * cos(w0);
    b[2] = 1 - az * A;

    a[0] = 1 + az /A;
    a[1] = -2 * cos(w0);
    a[2] = 1 - az / A;

    struct dsp_iir new = biquad_iir();
    biquad_iir_fill(new, a, b);
    return new;}

struct dsp_iir bqls_dcof(double fcf, float Q, float dbgain)
{
    double w0 = M_PI * fcf;
    double a[3];
    double b[3];
    double az = sin(w0) / (2 * Q);
    double A = dbgain * 10 / 40;

    b[0] = A * ((A + 1) - (A - 1) * cos(w0) + 2 * sqrt(A) * az);
    b[1] = 2 * A * ((A - 1) - (A + 1) * cos(w0));
    b[2] = A * ((A + 1) - (A - 1) * cos(w0) - 2 * sqrt(A) * az);

    a[0] = (A + 1) + (A - 1) * cos(w0) + 2 * sqrt(A) * az;
    a[1] = -2 * ((A - 1) + (A + 1) * cos(w0));
    a[2] = (A + 1) + (A - 1) * cos(w0) - 2 * sqrt(A) * az;

    struct dsp_iir new = biquad_iir();
    biquad_iir_fill(new, a, b);
    return new;
}

struct dsp_iir bqhs_dcof(double fcf, float Q, float dbgain)
{
    double w0 = M_PI * fcf;
    double a[3];
    double b[3];
    double az = sin(w0) / (2 * Q);
    double A = dbgain * 10 / 40;

    b[0] = A * ((A + 1) - (A - 1) * cos(w0) + 2 * sqrt(A) * az);
    b[1] = -2 * A * ((A - 1) - (A + 1) * cos(w0));
    b[2] = A * ((A + 1) - (A - 1) * cos(w0) - 2 * sqrt(A) * az);

    a[0] = (A + 1) - (A - 1) * cos(w0) + 2 * sqrt(A) * az;
    a[1] = -2 * ((A - 1) + (A + 1) * cos(w0));
    a[2] = (A + 1) - (A - 1) * cos(w0) - 2 * sqrt(A) * az;

    struct dsp_iir new = biquad_iir();
    biquad_iir_fill(new, a, b);
    return new;
}



/* Bipole Butterworth, Critically damped, and Bessel */
/* https://unicorn.us.com/trading/allpolefilters.html */

void p2_ccalc(double fcf, double p, double g, double *a, double *b)
{
    double w0 = tan(M_PI * fcf);
    double k[2];
    k[0] = p * w0;
    k[1] = g * pow2(w0);

    a[0] = k[1] / (1 + k[0] + k[1]);
    a[1] = 2 * a[0];
    a[2] = a[0];
    b[0] = 0.f;
    b[1] = 2 * a[0] * (1/k[1] - 1);
    b[2] = 1 - (a[0] + a[1] + a[2] + b[1]);
}

struct dsp_iir p2_bwlp(double fcf)
{
    double p = sqrt(2);
    double g = 1;

    struct dsp_iir new = biquad_iir();
    p2_ccalc(fcf, p, g, new.ccof, new.dcof);

    return new;
}

struct dsp_iir p2_bwhp(double fcf)
{
    struct dsp_iir new = p2_bwlp(fcf);
    new.ccof[1] *= -1;
    new.dcof[1] *= -1;

    return new;
}

struct dsp_iir p2_cdlp(double fcf)
{
    double g = 1;
    double p = 2;

    struct dsp_iir new = biquad_iir();
    p2_ccalc(fcf, p, g, new.ccof, new.dcof);

    return new;
}

struct dsp_iir p2_cdhp(double fcf)
{
    struct dsp_iir new = p2_cdlp(fcf);
    new.ccof[1] *= -1;
    new.dcof[1] *= -1;

    return new;
}

struct dsp_iir p2_beslp(double fcf)
{
    double g = 3;
    double p = 3;

    struct dsp_iir new = biquad_iir();
    p2_ccalc(fcf, p, g, new.ccof, new.dcof);

    return new;
}

struct dsp_iir p2_beshp(double fcf)
{
    struct dsp_iir new = p2_beslp(fcf);
    new.ccof[1] *= -1;
    new.dcof[1] *= -1;

    return new;
}

struct dsp_iir p2_iir_order(int order)
{
    struct dsp_iir new;
    new.n = 3;
    new.order = order;

    new.ccof = calloc(sizeof(float), 3 * order);
    new.dcof = calloc(sizeof(float), 3 * order);
    new.dx = calloc(sizeof(float), 3 * order);
    new.dy = calloc(sizeof(float), 3 * order);

    return new;
}

short p2_filter(struct dsp_iir iir, short val)
{
    float a = (float)val/SHRT_MAX;

    for (int i = 0; i < iir.order; i++) {
        int indx = i * iir.n;

        iir.dx[indx] = a;

        a = 0.f;

        for (int j = 0; j < iir.n; j++)
            a += iir.ccof[indx + j] * iir.dx[indx];

        for (int j = iir.n-1; j > 0; j--)
            iir.dx[indx] = iir.dx[indx-1];

        for (int j = 0; j < iir.n; j++)
            a -= iir.dcof[indx+j] * iir.dy[indx];

        iir.dy[indx] = a;

        for (int j = iir.n-1; j > 0; j--)
            iir.dy[indx] = iir.dy[indx-1];
    }

    return a * SHRT_MAX;
}


struct dsp_iir che_lp(int order, double fcf, double e)
{
    struct dsp_iir new = p2_iir_order(order);


    double a = tan(M_PI * fcf);
    double a2 = pow2(a);
    double u = log((1.f + sqrt(1.f + pow2(e)))/e);
    double su = sinh(u/new.order);
    double cu = cosh(u/new.order);
    double b, c, s;
    double ep = 2.f/e;

    for (int i = 0; i < new.order; ++i)
    {
        b = sin(M_PI * (2.f*i + 1.f)/(2.f*new.order)) * su;
        c = cos(M_PI * (2.f*i + 1.f)/(2.f*new.order)) * cu;
        c = pow2(b) + pow2(c);
        s = a2*c + 2.f*a*b + 1.f;
        double A = a2/(4.f);

        new.ccof[0*i] = ep * 1.f/A;
        new.ccof[1*i] = ep * -2.f/A;
        new.ccof[2*i] = ep * 1.f/A;

        new.dcof[0*i] = ep * 0.f;
        new.dcof[1*i] = ep * 2.f*(1-a2*c);
        new.dcof[2*i] = ep * -(a2*c - 2.f*a*b + 1.f);
    }

    return new;
}

struct dsp_iir che_hp(int order, double fcf, double e)
{
    struct dsp_iir new = che_lp(order, fcf, e);

    double a = tan(M_PI * fcf);
    double a2 = pow2(a);
    double u = log((1.f + sqrt(1.f + pow2(e)))/e);
    double su = sinh(u/new.order);
    double cu = cosh(u/new.order);
    double b, c, s;
    double ep = 2.f/e;

    for (int i = 0; i < new.order; ++i)
    {
        b = sin(M_PI * (2.f*i + 1.f)/(2.f*new.order)) * su;
        c = cos(M_PI * (2.f*i + 1.f)/(2.f*new.order)) * cu;
        c = pow2(b) + pow2(c);
        s = a2*c + 2.f*a*b + 1.f;
        double A = 1.f/(4.f);

        new.ccof[0*i] = ep * 1.f/A;
        new.ccof[1*i] = ep * -2.f/A;
        new.ccof[2*i] = ep * 1.f/A;


    }

    return new;
}

struct dsp_iir che_bp(int order, double s, double fcf1, double fcf2, double e)
{
    if (order %4 != 0) {
        YughWarn("Tried to make a filter with wrong order. Given order was %d, but order should be 4, 8, 12, ...", order);
    }

    double ep = 2.f/e;

    int n = order / 4;
        struct dsp_iir new = p2_iir_order(order);

        double a = cos(M_PI*(fcf1+fcf2)/2) / cos(M_PI*(fcf2-fcf1)/s);
        double a2 = pow2(a);
        double b = tan(M_PI*(fcf2-fcf1)/s);
        double b2 = pow2(b);
        double u = log((1.f+sqrt(1.f+pow2(e)))/e);
        double su = sinh(2.f*u/new.order);
        double cu = cosh(2.f*u/new.order);
        double A = b2/(4.f);
        double r, c;

        for (int i = 0; i < new.order; ++i) {
            r = sin(M_PI*(2.f*i+1.f)/new.order)*su;
            c = cos(M_PI*(2.f*i+1.f)/new.order)*su;
            c = pow2(r) + pow2(c);
            s = b2*c + 2.f*b*r + 1.f;

            new.ccof[0*i] = ep * 1.f/A;
            new.ccof[1*i] = ep * -2.f/A;
            new.ccof[2*i] = ep * 1.f/A;

            new.dcof[0*i] = 0.f;
            new.dcof[1*i] = ep * 4.f*a*(1.f+b*r)/s;
            new.dcof[2*i] = ep * 2.f*(b2*c-2.f*a2-1.f)/s;
            new.dcof[3*i] = ep * 4.f*a*(1.f-b*r)/s;
            new.dcof[4*i] = ep * -(b2*c - 2.f*b*r + 1.f) / s;
        }

        return new;
}

struct dsp_iir che_notch(int order, double fcf1, double fcf2, double e)
{
        if (order %4 != 0) {
        YughWarn("Tried to make a filter with wrong order. Given order was %d, but order should be 4, 8, 12, ...", order);
    }

    int n = order / 4;

    struct dsp_iir new = p2_iir_order(order);

    double a = cos(M_PI*(fcf1+fcf2)/2) / cos(M_PI*(fcf2-fcf1)/s);
    double a2 = pow2(a);
    double b = tan(M_PI*(fcf2-fcf1)/s);
    double b2 = pow2(b);
    double u = log((1.f+sqrt(1.f+pow2(e)))/e);
    double su = sinh(2.f*u/n);
    double cu = cosh(2.f*u/n);
    double A = b2/(4.f*s);
    double r, c, s;

    for (int i = 0; i < new.order; ++i) {
        r = sin(M_PI*(2.f*i+1.f)/new.order)*su;
        c = cos(M_PI*(2.f*i+1.f)/ew.order)*su;
        c = pow2(r) + pow2(c);
        s = b2*c + 2.f*b*r + 1.f;

        new.ccof[0*i] = ep * 1.f/A;
        new.ccof[1*i] = ep * -2.f/A;
        new.ccof[2*i] = ep * 1.f/A;

        new.ddof[0*i] = 0.f;
        new.dcof[1*i] = ep * 4.f*a*(c+b*r)/s;
        new.dcof[2*i] = ep * 2.f*(b2-2.f*a2*c-c)/s;
        new.dcof[3*i] = ep * 4.f*a*(c-b*r)/s;
        new.dcof[4*i] = ep * -(b2 - 2.f*b*r + c) / s;
    }

    return new;
}

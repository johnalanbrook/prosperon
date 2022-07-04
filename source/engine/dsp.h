#ifndef DSP_H
#define DSP_H

void am_mod(short *a, short *b, short *c, int n);

struct wav;

struct wav gen_sine(float amp, float freq, int sr, int ch);
struct wav gen_square(float amp, float freq, int sr, int ch);
struct wav gen_triangle(float amp, float freq, int sr, int ch);

#endif
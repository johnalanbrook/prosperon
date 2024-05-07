//externs made available by simplex.c

#ifndef SIMPLEX_H_INCLUDED
#define SIMPLEX_H_INCLUDED

#define SIMPLEX_X 0
#define SIMPLEX_Y 1
#define SIMPLEX_Z 2
#define SIMPLEX_W 3

typedef double (*noise2Dptr)(double, double);
typedef double (*noise3Dptr)(double, double, double);
typedef double (*noise4Dptr)(double, double, double, double);

typedef double (*noiseNDptr)(int, double[]);

typedef union NoiseUnion {
    noise2Dptr p2;
    noise3Dptr p3;
    noise4Dptr p4;
    noiseNDptr pn;
} genericNoise;

double Noise2D(double x, double y);
double Noise3D(double x, double y, double z);
double Noise4D(double x, double y, double z, double w);

double GBlur1D(double stdDev, double x);
double GBlur2D(double stdDev, double x, double y);

double Noise(genericNoise func, int len, double args[]);

double TurbulentNoise(genericNoise func, int direction, int iterations, int len, double args[]);
double FractalSumNoise(genericNoise func, int iterations, int len, double args[]);
double FractalSumAbsNoise(genericNoise func, int iterations, int len, double args[]);

double octave_3d(double x, double y, double z, int octaves, double persistence);

#endif

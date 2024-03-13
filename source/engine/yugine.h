#ifndef YUGINE_H
#define YUGINE_H

#include "script.h"

int sim_playing();
int sim_paused();
void sim_start();
void sim_pause();
void sim_stop();
void sim_step();
int phys_stepping();
void set_timescale(float val);
void print_stacktrace();
void engine_start(JSValue fn); /* fn runs after the engine starts */

int frame_fps();
double get_timescale();
void quit();

double apptime();
extern double renderMS;
extern double physMS;
extern double updateMS;
#endif

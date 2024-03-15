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
void print_stacktrace();
void engine_start(JSValue fn, JSValue proc_fn); /* fn runs after the engine starts */

int frame_fps();
void quit();

double apptime();
extern double timescale;
extern double renderMS;
extern double physMS;
extern double updateMS;
#endif

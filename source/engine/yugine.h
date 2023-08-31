#ifndef YUGINE_H
#define YUGINE_H

int sim_playing();
int sim_paused();
void sim_start();
void sim_pause();
void sim_stop();
void sim_step();
int phys_stepping();
void set_timescale(float val);
void print_stacktrace();

const char *engine_info();

int frame_fps();

extern double appTime;
extern double renderMS;
extern double physMS;
extern double updateMS;

#endif

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
void yugine_init();

const char *engine_info();
void app_name(char *name);

int frame_fps();
double get_timescale();

extern double appTime;
extern double renderMS;
extern double physMS;
extern double updateMS;
extern int editor_mode;
#endif

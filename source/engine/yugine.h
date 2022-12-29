#ifndef YUGINE_H
#define YUGINE_H

int sim_playing();
int sim_paused();
void sim_start();
void sim_pause();
void sim_stop();
void sim_step();
void set_timescale(float val);

int frame_fps();


#endif
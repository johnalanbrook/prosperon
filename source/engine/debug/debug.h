#ifndef DEBUG_GUI_H
#define DEBUG_GUI_H

#include <stdint.h>

struct d_prof {
  char *name;
  float *ms;
  uint64_t lap;
  int laps;
};

extern unsigned long long triCount;
void resetTriangles();
void prof_start(struct d_prof *prof);
void prof_reset(struct d_prof *prof);
void prof_lap(struct d_prof *prof);
float prof_lap_avg(struct d_prof *prof);

#endif

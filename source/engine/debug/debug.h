#ifndef DEBUG_GUI_H
#define DEBUG_GUI_H

#ifndef static_assert
#define static_assert(pred) switch(0){case 0:case pred:;}
#endif

#include <stdint.h>

struct d_prof {
  char *name;
  float *ms;
  uint64_t lap;
};

extern unsigned long long triCount;
void resetTriangles();
void prof_start(struct d_prof *prof);
void prof(struct d_prof *prof);

#endif

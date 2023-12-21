#include "debug.h"

#include "stb_ds.h"
#include "log.h"
#include "sokol/sokol_time.h"

unsigned long long triCount = 0;

void prof_start(struct d_prof *prof)
{
  arrsetlen(prof->ms, prof->laps);
}

void prof_reset(struct d_prof *prof)
{
  prof->lap = stm_now();
}

float prof_lap_avg(struct d_prof *prof)
{
  float avg;
  for (int i = 0; i < arrlen(prof->ms); i++)
    avg += prof->ms[i];

  avg /= arrlen(prof->ms);
  return avg;
}

void prof_lap(struct d_prof *prof)
{
  if (prof->ms == NULL)
    prof_start(prof);
    
  if (arrlen(prof->ms) >= prof->laps) {
    YughWarn("Profiled time of %s is avg %g", prof->name, prof_lap_avg(prof));
    arrsetlen(prof->ms, 0);
  }
  uint64_t t = stm_since(prof->lap);
  arrput(prof->ms, stm_sec(t));
}

void resetTriangles()
{
  triCount = 0;
}

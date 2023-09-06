#include "debug.h"

#include "stb_ds.h"
#include "log.h"
#include "sokol/sokol_time.h"

unsigned long long triCount = 0;

void prof_start(struct d_prof *prof)
{
#ifndef NDEBUG
  prof->lap = stm_now();
#endif
}

void prof(struct d_prof *prof)
{
#ifndef NDEBUG
  uint64_t t = stm_since(prof->lap);
  arrput(prof->ms, stm_sec(t));
#endif
}

void resetTriangles()
{
  triCount = 0;
}

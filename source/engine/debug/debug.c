#include "debug.h"

#include "stb_ds.h"
#include "log.h"
#include "sokol/sokol_time.h"

unsigned long long triCount = 0;

void prof_start(struct d_prof *prof)
{
  prof->lap = stm_now();
}

void prof(struct d_prof *prof)
{
  uint64_t t = stm_since(prof->lap);
  arrput(prof->ms, stm_sec(t));
}

void resetTriangles()
{
  triCount = 0;
}

#include "mix.h"
#include "stddef.h"
#include "time.h"
#include "sound.h"
#include "dsp.h"
#include <string.h>
#include "log.h"

#include <assert.h>

static struct bus bus[256];
static int first = 0; /* First bus available */

static int first_on = -1; /* First bus to fill buffer with */
short mastermix[BUF_FRAMES*CHANNELS];

static int initted = 0;

static float master_volume = 1.f;

void mix_master_vol(float v) {
    if (v < 0.f) v = 0.f;
    if (v > 100.f) v = 100.f;
    master_volume = v / 100.f;
}

void mixer_init() {
    for (int i = 0; i < 256; i++) {
        bus[i].next = i+1;
        bus[i].on = 0;
        bus[i].id = i;
    }

    bus[255].next = -1;

    initted = 1;
}

struct bus *first_free_bus(struct dsp_filter in) {
//    assert(initted);

    for (int i = 0; i < 255; i++)
      if (!bus[i].on) {
        bus[i].on = 1;
	bus[i].in = in;
        return &bus[i];
      }
	
    return NULL;
    
    if (first == -1) return NULL;
    int ret = first;
    first = bus[ret].next;

    bus[ret].on = 1;
    bus[ret].in = in;

    if (first_on != -1)  bus[first_on].prev = ret;

    bus[ret].next = first_on;
    bus[ret].prev = -1;
    first_on = ret;

    return &bus[ret];
}

void bus_free(struct bus *b)
{
  if (!b) return;
    b->on = 0;
    return;
    
    if (first_on == b->id) first_on = b->next;
    if (b->next != -1) bus[b->next].prev = b->prev;
    if (b->prev != -1) bus[b->prev].next = b->next;

    b->next = first;
    first = b->id;
    b->on = 0;
}

void bus_fill_buffers(short *master, int n) {
    int curbus = first_on;
//    if (curbus == -1) return;
    memset(master, 0, BUF_FRAMES*CHANNELS*sizeof(short));

    for (int i = 0; i < 255; i++) {
      if (!bus[i].on) continue;
      dsp_run(bus[i].in, bus[i].buf, BUF_FRAMES);
      for (int j = 0; j < BUF_FRAMES*CHANNELS; j++)
        master[j] += bus[i].buf[j] * master_volume;
    }

    return;

    while (curbus != -1) {
        int nextbus = bus[curbus].next; /* Save this in case busses get changed during fill */
        dsp_run(bus[curbus].in, bus[curbus].buf, BUF_FRAMES);
        for (int i = 0; i < BUF_FRAMES*CHANNELS; i++)
            master[i] += bus[curbus].buf[i] * master_volume;

        curbus = nextbus;
    }
}

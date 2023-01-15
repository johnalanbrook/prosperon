#include "mix.h"
#include "stddef.h"
#include "time.h"
#include "sound.h"
#include "dsp.h"
#include <string.h>

static struct bus bus[256];
static int first = 0; /* First bus available */
static int first_on = -1;
short mastermix[BUF_FRAMES*CHANNELS];

void mixer_init() {
    for (int i = 0; i < 256; i++) {
        bus[i].next = i+1;
        bus[i].on = 0;
        bus[i].id = i;
    }

    bus[255].next = -1;
}

struct bus *first_free_bus(struct dsp_filter in) {
    if (first == -1) return NULL;
    struct bus *ret = bus[first];
    first = ret->next;
    ret->on = 1;
    ret->in = in;

    ret->next = first_on;
    first_on = ret->id;

    return ret;
}

void bus_free(struct bus *bus)
{
    bus->next = first;
    bus->on = 0;
    first = bus->id;
}

void bus_fill_buffers(short *master, int n) {
    int curbus = first_one
    memset(master, 0, BUF_FRAMES*CHANNELS*sizeof(short));
    while (bus[curbus].next != -1) {
        dsp_run(bus[curbus].in, bus[curbus].buf, BUF_FRAMES);
        for (int i = 0; i < BUF_FRAMES*CHANNELS; i++)
            master[i] += bus[curbus].buf[i];

        curbus = bus[curbus].next;
    }

    /*
    for (int i = 0; i < 256; i++) {
        if (bus[i].on != 1) continue;
        dsp_run(bus[i].in, bus[i].buf, BUF_FRAMES);
    }

        for (int j = 0; j < 256; j++) {
            if (!bus[j].on) continue;
            for (int i = 0; i < BUF_FRAMES*CHANNELS; i++) {
                master[i] += bus[j].buf[i];
            }
        }
            */
}

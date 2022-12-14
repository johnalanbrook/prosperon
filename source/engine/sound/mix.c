#include "mix.h"
#include "stddef.h"
#include "time.h"
#include "sound.h"
#include "dsp.h"
#include <string.h>

static struct bus bus[256];
short mastermix[BUF_FRAMES*CHANNELS];

struct bus *first_free_bus(struct dsp_filter in) {
    for (int i = 0; i < 256; i++) {
        if (!bus[i].on) {
            bus[i].on = 1;
            bus[i].in = in;
            return &bus[i];
        }
    }

    return NULL;
}

void bus_free(struct bus *bus)
{
    bus->on = 0;
}

void bus_fill_buffers(short *master, int n) {
    for (int i = 0; i < 256; i++) {
        if (bus[i].on != 1) continue;
        dsp_run(bus[i].in, bus[i].buf, BUF_FRAMES);
    }

    memset(master, 0, BUF_FRAMES*CHANNELS*sizeof(short));

        for (int j = 0; j < 256; j++) {
            if (!bus[j].on) continue;
            for (int i = 0; i < BUF_FRAMES*CHANNELS; i++) {
                master[i] += bus[j].buf[i];
            }
        }
}

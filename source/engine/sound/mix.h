#ifndef MIX_H
#define MIX_H

#include "dsp.h"

struct sound;


struct bus {
    struct dsp_filter in;
    short buf[BUF_FRAMES*CHANNELS];
    float gain;
    int on;
    int next; /* Next available bus */
    int prev;
    int id;
};

extern short mastermix[BUF_FRAMES*CHANNELS];

void mixer_init();

struct bus *first_free_bus(struct dsp_filter in);
void bus_fill_buffers(short *master, int n);


void bus_free(struct bus *bus);


#endif

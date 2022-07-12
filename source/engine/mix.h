#ifndef MIX_H
#define MIX_H

#include "dsp.h"

struct sound;


struct bus {
    int on;
    struct dsp_filter in;
    short buf[BUF_FRAMES*CHANNELS];
    float gain;
};

struct listener {
    float x;
    float y;
    float z;
};

extern short mastermix[BUF_FRAMES*CHANNELS];

struct bus *first_free_bus(struct dsp_filter in);
void bus_fill_buffers(short *master, int n);
void bus_free(struct bus *bus);


#endif
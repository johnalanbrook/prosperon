#ifndef MIX_H
#define MIX_H

#include "dsp.h"
#include "sound.h"

struct bus {
    struct dsp_filter in;
    soundbyte buf[BUF_FRAMES*CHANNELS];
    float gain;
    int on;
    int next; /* Next available bus */
    int prev;
    int id;
};

extern soundbyte mastermix[BUF_FRAMES*CHANNELS];

void mixer_init();

struct bus *first_free_bus(struct dsp_filter in);
void bus_fill_buffers(soundbyte *master, int n);

/* Set volume between 0 and 100% */
void mix_master_vol(float v);
void bus_free(struct bus *bus);


#endif

#ifndef MIX_H
#define MIX_H

#define BUF_FRAMES 4096
#define CHANNELS 2
#define MUSIZE 2

struct sound;


struct bus {
    int on;
    struct sound *sound;
    short buf[BUF_FRAMES*CHANNELS];
};

extern short mastermix[BUF_FRAMES*CHANNELS];

struct bus *first_free_bus();
void bus_fill_buffers();


#endif
#include "mix.h"
#include "stddef.h"

#include "sound.h"

static struct bus bus[256];
short mastermix[BUF_FRAMES*CHANNELS];

struct bus *first_free_bus() {
    for (int i = 0; i < 256; i++) {
        if (!bus[i].on) return &bus[i];
    }

    return NULL;
}

void bus_fill_buffers() {
    for (int i = 0; i < 256; i++) {
        if (!bus[i].on) continue;

        short *s = (short*)bus[i].sound->data->data;

        for (int k = 0; k < BUF_FRAMES; k++) {
            for (int j = 0; j < CHANNELS; j++) {
                bus[i].buf[k*CHANNELS + j] = s[bus[i].sound->frame++];
                if (bus[i].sound->frame == bus[i].sound->data->frames) {
                    bus[i].sound->frame = 0;
                    if (!bus[i].sound->loop) bus[i].on = 0;
                }
            }
        }
    }

    for (int i = 0; i < BUF_FRAMES*CHANNELS; i++) {
        mastermix[i] = 0;

        for (int j = 0; j < 256; j++) {
            if (!bus[j].on) continue;
            mastermix[i] += bus[j].buf[i];
        }
    }
}
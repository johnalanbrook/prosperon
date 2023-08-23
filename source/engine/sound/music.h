#ifndef MUSIC_H
#define MUSIC_H

#include "tsf.h"
#include "tml.h"

struct dsp_midi_song {
    float bpm;
    double time;
    tsf *sf;
    tml_message *midi;
};

extern float music_pan;

void play_song(const char *midi, const char *sf);
void music_stop();
void dsp_midi_fillbuf(struct dsp_midi_song *song, void *out, int n);

#endif

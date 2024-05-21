#ifndef MUSIC_H
#define MUSIC_H

#include "tsf.h"
#include "tml.h"
#include "dsp.h"

typedef struct dsp_midi_song {
  float bpm;
  double time;
  tml_message *midi;
  tsf *sf;
} midi;

dsp_node *dsp_midi(const char *midi, tsf *sf);
tsf *make_soundfont(const char *sf);
void play_song(const char *midi, const char *sf);
void dsp_midi_fillbuf(struct dsp_midi_song *song, void *out, int n);

#endif

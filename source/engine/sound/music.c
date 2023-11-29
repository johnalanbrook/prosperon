#include "music.h"

#include "dsp.h"
#include "tsf.h"
#include "tml.h"
#include "sound.h"
#include "log.h"
#include "resources.h"
#include <stdlib.h>
#include "stb_ds.h"

#define TSF_BLOCK 32

static struct {
  char *key;
  tsf **value;
} *sf_hash = NULL;

void dsp_midi_fillbuf(struct dsp_midi_song *song, void *out, int n)
{
  soundbyte *o = out;
  tml_message *midi = song->midi;

  for (int i = 0; i < n; i += TSF_BLOCK) {
    while (midi && song->time >= midi->time) {
      switch (midi->type) {
	 case TML_PROGRAM_CHANGE:
	   tsf_channel_set_presetnumber(song->sf, midi->channel, midi->program, (midi->channel == 9));
	   break;

	 case TML_NOTE_ON:
	   tsf_channel_note_on(song->sf, midi->channel, midi->key, midi->velocity / 127.f);
	   break;

	 case TML_NOTE_OFF:
	   tsf_channel_note_off(song->sf, midi->channel, midi->key);
	   break;

	 case TML_PITCH_BEND:
	   tsf_channel_set_pitchwheel(song->sf, midi->channel, midi->pitch_bend);
	   break;

	 case TML_CONTROL_CHANGE:
	   tsf_channel_midi_control(song->sf, midi->channel, midi->control, midi->control_value);
	   break;
       }

      midi = midi->next;
    }
    tsf_render_float(song->sf, o, TSF_BLOCK, 0);
    o += TSF_BLOCK*CHANNELS;
    song->time += TSF_BLOCK * (1000.f/SAMPLERATE);
  }

  song->midi = midi;
}

tsf *make_soundfont(const char *path)
{
  int idx = shgeti(sf_hash, path);
  if (idx != -1) return sf_hash[idx].value;
  
  long rawlen;
  void *raw = slurp_file(path, &rawlen);
  tsf *sf = tsf_load_memory(raw,rawlen);
  free(raw);
  
  if (!sf) { YughWarn("Soundfont %s not found.", sf); return NULL; }
  tsf_set_output(sf, TSF_STEREO_INTERLEAVED, SAMPLERATE, 0.f);
  // Preset on 10th MIDI channel to use percussion sound bank if possible  
  tsf_channel_set_bank_preset(sf, 0, 128, 0);

  shput(sf_hash, path, sf);
  return sf;
}

void dsp_midi_free(struct dsp_midi_song *ms)
{
  free(ms->midi);
  tsf_close(ms->sf);
  free(ms);
}

dsp_node *dsp_midi(const char *midi, tsf *sf)
{
  long rawlen;
  void *raw = slurp_file(midi, &rawlen);
  struct dsp_midi_song *ms = malloc(sizeof(*ms));
  ms->time = 0.0;
  ms->midi = tml_load_memory(raw, rawlen);
  ms->sf = tsf_copy(sf);
  free(midi);
  return make_node(ms, dsp_midi_fillbuf, dsp_midi_free);
}

void play_song(const char *midi, const char *sf)
{
  plugin_node(dsp_midi(midi, make_soundfont(sf)), masterbus);
}

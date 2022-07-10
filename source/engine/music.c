#include "music.h"

#include "dsp.h"
#include "tsf.h"
#include "tml.h"
#include "mix.h"
#include "sound.h"

#define TSF_BLOCK 64

struct dsp_filter cursong;
struct dsp_midi_song gsong;

void dsp_midi_fillbuf(struct dsp_midi_song *song, void *out, int n)
{
    short *o = (short*)out;
    tml_message *midi = song->midi;

    for (int i = TSF_BLOCK; n; n -= i, o += n*CHANNELS) {
        if (i > n) i = n;

        for (song->time += i * (1000.f / SAMPLERATE); midi && song->time >= midi->time; midi = midi->next) {
            switch (midi->type)
            {
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
        }

        tsf_render_short(song->sf, o, i, 0);
    }

    song->midi = midi;
}

void play_song(const char *midi, const char *sf)
{
    gsong.midi = tml_load_filename("sounds/one-winged-angel.mid");
    gsong.sf = tsf_load_filename("sounds/mario.sf2");
    gsong.time = 0.f;

    tsf_set_output(gsong.sf, TSF_STEREO_INTERLEAVED, SAMPLERATE, 0.f);

    // Preset on 10th MIDI channel to use percussion sound bank if possible
    tsf_channel_set_bank_preset(gsong.sf, 9, 128, 0);

    cursong.data = &gsong;
    cursong.filter = dsp_midi_fillbuf;
    first_free_bus(cursong);
}

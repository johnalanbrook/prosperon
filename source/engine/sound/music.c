#include "music.h"

#include "dsp.h"
#include "tsf.h"
#include "tml.h"
#include "mix.h"
#include "sound.h"
#include "log.h"

#define TSF_BLOCK 32

struct dsp_filter cursong;
struct dsp_midi_song gsong;

float music_pan = 0.f;

void dsp_midi_fillbuf(struct dsp_midi_song *song, void *out, int n)
{
    short *o = (short*)out;
    tml_message *midi = song->midi;

    for (int i = 0; i < n; i += TSF_BLOCK) {

        while (midi && song->time >= midi->time) {

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


             midi = midi->next;
        }


        tsf_render_short(song->sf, o, TSF_BLOCK, 0);
        o += TSF_BLOCK*CHANNELS;
       song->time += TSF_BLOCK * (1000.f/SAMPLERATE);
    }

    song->midi = midi;

    dsp_pan(&music_pan, out, n);
}

struct bus *musicbus;

void play_song(const char *midi, const char *sf)
{
    gsong.midi = tml_load_filename(midi);
    if (gsong.midi == NULL) {
        YughWarn("Midi %s not found.", midi);
        return;
    }

    gsong.sf = tsf_load_filename(sf);

    if (gsong.sf == NULL) {
        YughWarn("SF2 %s not found.", sf);
        return;
    }

    gsong.time = 0.f;

    tsf_set_output(gsong.sf, TSF_STEREO_INTERLEAVED, SAMPLERATE, 0.f);

    // Preset on 10th MIDI channel to use percussion sound bank if possible
    tsf_channel_set_bank_preset(gsong.sf, 9, 128, 0);

    cursong.data = &gsong;
    cursong.filter = dsp_midi_fillbuf;
    musicbus = first_free_bus(cursong);
}


void music_play()
{

}

void music_stop()
{
    bus_free(musicbus);
}

void music_volume()
{

}

void music_resume()
{

}

void music_paused()
{

}

void music_pause()
{

}

void sound_play()
{

}
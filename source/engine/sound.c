#include "sound.h"
#include "limits.h"
#include "log.h"
#include "math.h"
#include "music.h"
#include "resources.h"
#include "stb_vorbis.h"
#include "string.h"
#include "time.h"
#include <stdlib.h>

#include "samplerate.h"

#include "stb_ds.h"

#include "dsp.h"
#include "mix.h"

#include "sokol/sokol_audio.h"

#define TSF_NO_STDIO
#define TSF_IMPLEMENTATION
#include "tsf.h"

#define TML_NO_STDIO
#define TML_IMPLEMENTATION
#include "tml.h"

#define DR_WAV_NO_STDIO
#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

#ifndef NFLAC

#define DR_FLAC_IMPLEMENTATION
#define DR_FLAC_NO_STDIO
#include "dr_flac.h"
#endif

#ifndef NMP3
#define DR_MP3_NO_STDIO
#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"
#endif

#define QOA_NO_STDIO
#define QOA_IMPLEMENTATION
#include "qoa.h"

static struct {
  char *key;
  struct wav *value;
} *wavhash = NULL;

static struct wav change_channels(struct wav w, int ch) {
  soundbyte *data = w.data;
  int samples = ch * w.frames;
  soundbyte *new = malloc(sizeof(soundbyte) * samples);

  if (ch > w.ch) {
    /* Sets all new channels equal to the first one */
    for (int i = 0; i < w.frames; i++) {
      for (int j = 0; j < ch; j++)
        new[i * ch + j] = data[i];
    }
  } else {
    /* Simple method; just use first N channels present in wav */
    for (int i = 0; i < w.frames; i++)
      for (int j = 0; j < ch; j++)
        new[i * ch + j] = data[i * ch + j];
  }

  free(w.data);
  w.data = new;
  return w;
}

static struct wav change_samplerate(struct wav w, int rate) {
  float ratio = (float)rate / w.samplerate;
  int outframes = w.frames * ratio;
  SRC_DATA ssrc;
  soundbyte *resampled = calloc(w.ch*outframes,sizeof(soundbyte));

  ssrc.data_in = w.data;
  ssrc.data_out = resampled;
  ssrc.input_frames = w.frames;
  ssrc.output_frames = outframes;
  ssrc.src_ratio = ratio;

  int err = src_simple(&ssrc, SRC_LINEAR, w.ch);
  if (err) {
    YughError("Resampling error code %d: %s", err, src_strerror(err));
    free(resampled);
    return w;
  }
  
  free(w.data);
  w.data = resampled;
  w.frames = outframes;
  w.samplerate = rate;

  return w;
}

void wav_norm_gain(struct wav *w, double lv) {
  short tarmax = db2short(lv);
  short max = 0;
  short *s = w->data;
  for (int i = 0; i < w->frames; i++) {
    for (int j = 0; j < w->ch; j++) {
      max = (abs(s[i * w->ch + j]) > max) ? abs(s[i * w->ch + j]) : max;
    }
  }

  float mult = (float)max / tarmax;

  for (int i = 0; i < w->frames; i++) {
    for (int j = 0; j < w->ch; j++) {
      s[i * w->ch + j] *= mult;
    }
  }
}

void push_sound(soundbyte *buffer, int frames, int chan)
{
  bus_fill_buffers(buffer, frames*chan);
}

void sound_init() {
  saudio_setup(&(saudio_desc){
    .stream_cb = push_sound,
    .sample_rate = SAMPLERATE,
    .num_channels = CHANNELS,
    .buffer_frames = BUF_FRAMES,
    .logger.func = sg_logging,
  });
  mixer_init();
}

struct wav *make_sound(const char *wav) {
  int index = shgeti(wavhash, wav);
  if (index != -1) return wavhash[index].value;
  
  char *ext = strrchr(wav, '.')+1;
  
  if(!ext) {
    YughWarn("No extension detected for %s.", wav);
    return NULL;
  }

  struct wav mwav;
  long rawlen;
  void *raw = slurp_file(wav, &rawlen);
  if (!raw) {
    YughError("Could not find file %s.", wav);
    return;
  }

  if (!strcmp(ext, "wav"))
    mwav.data = drwav_open_memory_and_read_pcm_frames_f32(raw, rawlen, &mwav.ch, &mwav.samplerate, &mwav.frames, NULL);
  #ifndef NFLAC
  else if (!strcmp(ext, "flac"))
    mwav.data = drflac_open_memory_and_read_pcm_frames_f32(raw, rawlen, &mwav.ch, &mwav.samplerate, &mwav.frames, NULL);
  #endif
  #ifndef NMP3
  else if (!strcmp(ext, "mp3")) {
    drmp3_config cnf;
    mwav.data = drmp3_open_memory_and_read_pcm_frames_f32(raw, rawlen, &cnf, &mwav.frames, NULL);
    mwav.ch = cnf.channels;
    mwav.samplerate = cnf.sampleRate;
  }
  #endif
  else if (!strcmp(ext, "qoa")) {
    qoa_desc qoa;
    short *qoa_data = qoa_decode(raw, rawlen, &qoa);
    mwav.ch = qoa.channels;
    mwav.samplerate = qoa.samplerate;
    mwav.frames = qoa.samples;
    mwav.data = malloc(sizeof(soundbyte) * mwav.frames * mwav.ch);
    src_short_to_float_array(qoa_data, mwav.data, mwav.frames*mwav.ch);
    free(qoa_data);
  } else {
    YughWarn("Cannot process file type '%s'.", ext);
    free (raw);
    return NULL;
  }
  free(raw);

  if (mwav.samplerate != SAMPLERATE)
    mwav = change_samplerate(mwav, SAMPLERATE);

  if (mwav.ch != CHANNELS)
    mwav = change_channels(mwav, CHANNELS);

  mwav.gain = 1.f;
  struct wav *newwav = malloc(sizeof(*newwav));
  *newwav = mwav;
  if (shlen(wavhash) == 0) sh_new_arena(wavhash);
  shput(wavhash, wav, newwav);

  return newwav;
}

void free_sound(const char *wav) {
  struct wav *w = shget(wavhash, wav);
  if (w == NULL) return;

  free(w->data);
  free(w);
  shdel(wavhash, wav);
}

struct soundstream *soundstream_make() {
  struct soundstream *new = malloc(sizeof(*new));
  new->buf = circbuf_make(sizeof(short), BUF_FRAMES * CHANNELS * 2);
  return new;
}

void kill_oneshot(struct sound *s) {
  free(s);
}

void play_oneshot(struct wav *wav) {
  struct sound *new = malloc(sizeof(*new));
  new->data = wav;
  new->bus = first_free_bus(dsp_filter(new, sound_fillbuf));
  new->playing = 1;
  new->loop = 0;
  new->frame = 0;
  new->endcb = kill_oneshot;
}

struct sound *play_sound(struct wav *wav) {
  struct sound *new = calloc(1, sizeof(*new));
  new->data = wav;

  new->bus = first_free_bus(dsp_filter(new, sound_fillbuf));
  new->playing = 1;

  return new;
}

int sound_playing(const struct sound *s) {
  return s->playing;
}

int sound_paused(const struct sound *s) {
  return (!s->playing && s->frame < s->data->frames);
}
void sound_pause(struct sound *s) {
  s->playing = 0;
  bus_free(s->bus);
}

void sound_resume(struct sound *s) {
  s->playing = 1;
  s->bus = first_free_bus(dsp_filter(s, sound_fillbuf));
}

void sound_stop(struct sound *s) {
  s->playing = 0;
  s->frame = 0;
  bus_free(s->bus);
}

int sound_finished(const struct sound *s) {
  return !s->playing && s->frame == s->data->frames;
}

int sound_stopped(const struct sound *s) {
  return !s->playing && s->frame == 0;
}

struct mp3 make_music(const char *mp3) {
  //    drmp3 new;
  //    if (!drmp3_init_file(&new, mp3, NULL)) {
  //        YughError("Could not open mp3 file %s.", mp3);
  //    }

  struct mp3 newmp3 = {};
  return newmp3;
}

void close_audio_device(int device) {
}

int open_device(const char *adriver) {
  return 0;
}

void sound_fillbuf(struct sound *s, soundbyte *buf, int n) {
  float gainmult = pct2mult(s->data->gain);

  soundbyte *in = s->data->data;
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < CHANNELS; j++)
      buf[i * CHANNELS + j] = in[s->frame*CHANNELS + j] * gainmult;
    s->frame++;
    
    if (s->frame == s->data->frames) {
      bus_free(s->bus);
      s->bus = NULL;
      s->endcb(s);
      return;
    }
  }
}

void mp3_fillbuf(struct sound *s, soundbyte *buf, int n) {
}

void soundstream_fillbuf(struct soundstream *s, soundbyte *buf, int n) {
  int max = 1;//s->buf->write - s->buf->read;
  int lim = (max < n * CHANNELS) ? max : n * CHANNELS;
  for (int i = 0; i < lim; i++) {
//    buf[i] = cbuf_shift(s->buf);
  }
}

float short2db(short val) {
  return 20 * log10(abs(val) / SHRT_MAX);
}

short db2short(float db) {
  return pow(10, db / 20.f) * SHRT_MAX;
}

short short_gain(short val, float db) {
  return (short)(pow(10, db / 20.f) * val);
}

float pct2db(float pct) {
  if (pct <= 0) return -72.f;

  return 10 * log2(pct);
}

float pct2mult(float pct) {
  if (pct <= 0) return 0.f;

  return pow(10, 0.5 * log2(pct));
}

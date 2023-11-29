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

#define POCKETMOD_IMPLEMENTATION
#include "pocketmod.h"

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

#ifndef NQOA
#define QOA_NO_STDIO
#define QOA_IMPLEMENTATION
#include "qoa.h"
#endif

static struct {
  char *key;
  struct wav *value;
} *wavhash = NULL;

void change_channels(struct wav *w, int ch) {
  if (w->ch == ch) return;
  soundbyte *data = w->data;
  int samples = ch * w->frames;
  soundbyte *new = malloc(sizeof(soundbyte) * samples);

  if (ch > w->ch) {
    /* Sets all new channels equal to the first one */
    for (int i = 0; i < w->frames; i++) {
      for (int j = 0; j < ch; j++)
        new[i * ch + j] = data[i];
    }
  } else {
    /* Simple method; just use first N channels present in wav */
    for (int i = 0; i < w->frames; i++)
      for (int j = 0; j < ch; j++)
        new[i * ch + j] = data[i * ch + j];
  }

  free(w->data);
  w->data = new;
}

void resample(soundbyte *in, soundbyte *out, int in_frames, int out_frames, int channels)
{
  float ratio = (float)in_frames/out_frames;
  SRC_DATA ssrc;
  ssrc.data_in = in;
  ssrc.data_out = out;
  ssrc.input_frames = in_frames;
  ssrc.output_frames = out_frames;
  ssrc.src_ratio = ratio;
  int err = src_simple(&ssrc, SRC_LINEAR, channels);
  if (err)
    YughError("Resampling error code %d: %s", err, src_strerror(err));
}

void change_samplerate(struct wav *w, int rate) {
  if (rate == w->samplerate) return;
  float ratio = (float)rate / w->samplerate;
  int outframes = w->frames * ratio;
  soundbyte *resampled = malloc(w->ch*outframes*sizeof(soundbyte));
  resample(w->data, resampled, w->frames, outframes, w->ch);
  free(w->data);
  
  w->data = resampled;
  w->frames = outframes;
  w->samplerate = rate;
}

void push_sound(soundbyte *buffer, int frames, int chan)
{
  set_soundbytes(buffer, dsp_node_out(masterbus), frames*chan);
}

void filter_mod(pocketmod_context *mod, soundbyte *buffer, int frames)
{
  pocketmod_render(mod, buffer, frames*CHANNELS*sizeof(soundbyte));
}

dsp_node *dsp_mod(const char *path)
{
  long modsize;
  void *data = slurp_file(path, &modsize);
  pocketmod_context *mod = malloc(sizeof(*mod));
  pocketmod_init(mod, data, modsize, SAMPLERATE);
  return make_node(mod, filter_mod, NULL);
}

void sound_init() {
  dsp_init();
  saudio_setup(&(saudio_desc){
    .stream_cb = push_sound,
    .sample_rate = SAMPLERATE,
    .num_channels = CHANNELS,
    .buffer_frames = BUF_FRAMES,
    .logger.func = sg_logging,
  });
}

typedef struct {
  int channels;
  int samplerate;
  void *f;
} stream;

void mp3_filter(stream *mp3, soundbyte *buffer, int frames)
{
  if (mp3->samplerate == SAMPLERATE) {
    drmp3_read_pcm_frames_f32(mp3->f, frames, buffer);
    return;
  }
  
  int in_frames = (float)mp3->samplerate/SAMPLERATE;
  soundbyte *decode = malloc(sizeof(*decode)*in_frames*mp3->channels);
  drmp3_read_pcm_frames_f32(mp3->f, in_frames, decode);
  resample(decode, buffer, in_frames, frames, CHANNELS);
}

struct wav *make_sound(const char *wav) {
  int index = shgeti(wavhash, wav);
  if (index != -1)
    return wavhash[index].value;
  
  char *ext = strrchr(wav, '.')+1;
  
  if(!ext) {
    YughWarn("No extension detected for %s.", wav);
    return NULL;
  }

  struct wav *mwav = malloc(sizeof(*mwav));
  long rawlen;
  void *raw = slurp_file(wav, &rawlen);
  if (!raw) {
    YughError("Could not find file %s.", wav);
    return NULL;
  }

  if (!strcmp(ext, "wav"))
    mwav->data = drwav_open_memory_and_read_pcm_frames_f32(raw, rawlen, &mwav->ch, &mwav->samplerate, &mwav->frames, NULL);

  else if (!strcmp(ext, "flac")) {
  #ifndef NFLAC  
    mwav->data = drflac_open_memory_and_read_pcm_frames_f32(raw, rawlen, &mwav->ch, &mwav->samplerate, &mwav->frames, NULL);
  #else
    YughWarn("Could not load %s because Primum was built without FLAC support.", wav);
  #endif
  }
  else if (!strcmp(ext, "mp3")) {
  #ifndef NMP3  
    drmp3_config cnf;
    mwav->data = drmp3_open_memory_and_read_pcm_frames_f32(raw, rawlen, &cnf, &mwav->frames, NULL);
    mwav->ch = cnf.channels;
    mwav->samplerate = cnf.sampleRate;
  #else
    YughWarn("Could not load %s because Primum was built without MP3 support.", wav);
  #endif
  }
  else if (!strcmp(ext, "qoa")) {
  #ifndef NQOA
    qoa_desc qoa;
    short *qoa_data = qoa_decode(raw, rawlen, &qoa);
    mwav->ch = qoa.channels;
    mwav->samplerate = qoa.samplerate;
    mwav->frames = qoa.samples;
    mwav->data = malloc(sizeof(soundbyte) * mwav->frames * mwav->ch);
    src_short_to_float_array(qoa_data, mwav->data, mwav->frames*mwav->ch);
    free(qoa_data);
  #else
    YughWarn("Could not load %s because Primum was built without QOA support.", wav);
  #endif
  } else {
    YughWarn("File with unknown type '%s'.", wav);
    free (raw);
    free(mwav);
    return NULL;
  }
  free(raw);

  change_samplerate(mwav, SAMPLERATE);
  change_channels(mwav, CHANNELS);

  if (shlen(wavhash) == 0) sh_new_arena(wavhash);
  shput(wavhash, wav, mwav);

  return mwav;
}

void free_sound(const char *wav) {
  struct wav *w = shget(wavhash, wav);
  if (w == NULL) return;

  free(w->data);
  free(w);
  shdel(wavhash, wav);
}

void sound_fillbuf(struct sound *s, soundbyte *buf, int n) {
  int frames = s->data->frames - s->frame;
  if (frames == 0) return;
  int end = 0;
  if (frames > n)
    frames = n;
  else
    end = 1;

  if (s->timescale != 1) {
    src_callback_read(s->src, s->timescale, frames, buf);
    return;
  }
  
  soundbyte *in = s->data->data;
  
  for (int i = 0; i < frames; i++) {
    for (int j = 0; j < CHANNELS; j++)
      buf[i * CHANNELS + j] = in[s->frame*CHANNELS + j];

    s->frame++;
  }

  if(end) {
    if (s->loop)
      s->frame = 0;
    call_env(s->hook, "this.end();");
  }
}

void free_source(struct sound *s)
{
  JS_FreeValue(js, s->hook);
  src_delete(s->src);
  free(s);
}

static long *src_cb(struct sound *s, float **data)
{
  long needed = BUF_FRAMES/s->timescale;
  *data = s->data->data+s->frame;
  s->frame += needed;
  return needed;
}

struct dsp_node *dsp_source(char *path)
{
  struct sound *self = malloc(sizeof(*self));
  self->frame = 0;
  self->data = make_sound(path);
  self->loop = false;
  self->src = src_callback_new(src_cb, SRC_SINC_MEDIUM_QUALITY, 2, NULL, self);
  self->timescale = 1;
  self->hook = JS_UNDEFINED;
  dsp_node *n = make_node(self, sound_fillbuf, free_source);
  return n;
}

int sound_finished(const struct sound *s) {
  return s->frame == s->data->frames;
}

struct mp3 make_music(const char *mp3) {
  //    drmp3 new;
  //    if (!drmp3_init_file(&new, mp3, NULL)) {
  //        YughError("Could not open mp3 file %s.", mp3);
  //    }

  struct mp3 newmp3 = {};
  return newmp3;
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

float float2db(float val) { return 20 * log10(fabsf(val)); }
float db2float(float db) { return pow(10, db/20); }

float fgain(float val, float db) {
  return pow(10,db/20.f)*val;
}

float pct2db(float pct) {
  if (pct <= 0) return -72.f;

  return 10 * log2(pct);
}

float pct2mult(float pct) {
  if (pct <= 0) return 0.f;

  return pow(10, 0.5 * log2(pct));
}

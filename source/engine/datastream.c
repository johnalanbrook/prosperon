#include "datastream.h"

#include "config.h"
#include "dsp.h"
#include "iir.h"
#include "limits.h"
#include "log.h"
#include "resources.h"
#include "sound.h"
#include "texture.h"
#include <stdbool.h>
#include <stdlib.h>
#include "font.h"
#include "render.h"

#include "cbuf.h"

#include "sokol/sokol_gfx.h"

sg_shader vid_shader;
sg_pipeline vid_pipeline;
sg_bindings vid_bind;

void soundstream_fillbuf(struct datastream *ds, soundbyte *buf, int frames) {
  for (int i = 0; i < frames*CHANNELS; i++)
    buf[i] = ringshift(ds->ring);
}

static void render_frame(plm_t *mpeg, plm_frame_t *frame, struct datastream *ds) {
  return;
  uint8_t rgb[frame->height*frame->width*4];
  plm_frame_to_rgba(frame, rgb, frame->width*4);
  sg_image_data imgd;
  sg_range ir = {
    .ptr = rgb,
    .size = frame->height*frame->width*4*sizeof(uint8_t)
  };

  imgd.subimage[0][0] = ir;  
  sg_update_image(ds->img, &imgd);
}

static void render_audio(plm_t *mpeg, plm_samples_t *samples, struct datastream *ds) {
  for (int i = 0; i < samples->count * CHANNELS; i++)
    ringpush(ds->ring, samples->interleaved[i]);
}

struct datastream *ds_openvideo(const char *path)
{
  struct datastream *ds = malloc(sizeof(*ds));
  size_t rawlen;
  void *raw;
  raw = slurp_file(path, &rawlen);
  ds->plm = plm_create_with_memory(raw, rawlen, 0);
  free(raw);

  if (!ds->plm) {
    YughError("Couldn't open %s", path);
  }

  YughWarn("Opened %s - framerate: %f, samplerate: %d,audio streams: %d, duration: %g",
          path,
          plm_get_framerate(ds->plm),
          plm_get_samplerate(ds->plm),
	  plm_get_num_audio_streams(ds->plm),
          plm_get_duration(ds->plm));


  ds->img = sg_make_image(&(sg_image_desc){
    .width = plm_get_width(ds->plm),
    .height = plm_get_height(ds->plm)
  });  

  ds->ring = ringnew(ds->ring, 8192);
  plugin_node(make_node(ds, soundstream_fillbuf, NULL), masterbus);

  plm_set_video_decode_callback(ds->plm, render_frame, ds);
  plm_set_audio_decode_callback(ds->plm, render_audio, ds);
  plm_set_loop(ds->plm, false);

  plm_set_audio_enabled(ds->plm, true);
  plm_set_audio_stream(ds->plm, 0);

  // Adjust the audio lead time according to the audio_spec buffer size
  plm_set_audio_lead_time(ds->plm, BUF_FRAMES / SAMPLERATE);

  ds->playing = true;

  return ds;
}

void ds_advance(struct datastream *ds, double s) {
  if (ds->playing) plm_decode(ds->plm, s);
}

void ds_seek(struct datastream *ds, double time) {
  plm_seek(ds->plm, time, false);
}

void ds_advanceframes(struct datastream *ds, int frames) {
  for (int i = 0; i < frames; i++) {
    plm_frame_t *frame = plm_decode_video(ds->plm);
    render_frame(ds->plm, frame, ds);
  }
}

void ds_pause(struct datastream *ds) {
  ds->playing = false;
}

void ds_stop(struct datastream *ds) {
  if (ds->plm != NULL) {
    plm_destroy(ds->plm);
    ds->plm = NULL;
  }

  ds->playing = false;
}

// TODO: Must be a better way
int ds_videodone(struct datastream *ds) {
  return (ds->plm == NULL) || plm_get_time(ds->plm) >= plm_get_duration(ds->plm);
}

double ds_remainingtime(struct datastream *ds) {
  if (ds->plm != NULL)
    return plm_get_duration(ds->plm) - plm_get_time(ds->plm);
  else
    return 0.f;
}

double ds_length(struct datastream *ds) {
  return plm_get_duration(ds->plm);
}

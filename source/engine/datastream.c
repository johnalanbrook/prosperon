#include "datastream.h"

#include "config.h"
#include "dsp.h"
#include "iir.h"
#include "limits.h"
#include "log.h"
#include "mix.h"
#include "resources.h"
#include "shader.h"
#include "sound.h"
#include "texture.h"
#include <stdbool.h>
#include <stdlib.h>
#include "font.h"
#include "render.h"

#include "mpeg2.sglsl.h"

#define CBUF_IMPLEMENT
#include "cbuf.h"

#include "sokol/sokol_gfx.h"

sg_shader vid_shader;
sg_pipeline vid_pipeline;
sg_bindings vid_bind;

static void render_frame(plm_t *mpeg, plm_frame_t *frame, void *user) {
  struct datastream *ds = user;
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

static void render_audio(plm_t *mpeg, plm_samples_t *samples, void *user) {
  struct datastream *ds = user;
  short t;

  for (int i = 0; i < samples->count * CHANNELS; i++) {
    t = (short)(samples->interleaved[i] * SHRT_MAX);
//    cbuf_push(ds->astream->buf, t * 5);
  }
}

void ds_openvideo(struct datastream *ds, const char *video, const char *adriver) {
  size_t rawlen;
  void *raw;
  raw = slurp_file(video, &rawlen);
  ds->plm = plm_create_with_memory(raw, rawlen, 0);
  free(raw);

  if (!ds->plm) {
    YughError("Couldn't open %s", video);
  }

  YughWarn("Opened %s - framerate: %f, samplerate: %d, audio streams: %i, duration: %f",
          video,
          plm_get_framerate(ds->plm),
          plm_get_samplerate(ds->plm),

          plm_get_duration(ds->plm));


  ds->img = sg_make_image(&(sg_image_desc){
    .width = plm_get_width(ds->plm),
    .height = plm_get_height(ds->plm)
  });  


  ds->astream = soundstream_make();
  struct dsp_filter astream_filter;
  astream_filter.data = &ds->astream;
  astream_filter.filter = soundstream_fillbuf;

  // struct dsp_filter lpf = lpf_make(8, 10000);
  struct dsp_filter lpf = lpf_make(1, 200);
  struct dsp_iir *iir = lpf.data;
  iir->in = astream_filter;

  struct dsp_filter hpf = hpf_make(1, 2000);
  struct dsp_iir *hiir = hpf.data;
  hiir->in = astream_filter;

  /*
      struct dsp_filter llpf = lp_fir_make(20);
      struct dsp_fir *fir = llpf.data;
      fir->in = astream_filter;
  */

  // first_free_bus(astream_filter);

  plm_set_video_decode_callback(ds->plm, render_frame, ds);
  plm_set_audio_decode_callback(ds->plm, render_audio, ds);
  plm_set_loop(ds->plm, false);

  plm_set_audio_enabled(ds->plm, true);
  plm_set_audio_stream(ds->plm, 0);

  // Adjust the audio lead time according to the audio_spec buffer size
  plm_set_audio_lead_time(ds->plm, BUF_FRAMES / SAMPLERATE);

  ds->playing = true;
}

void MakeDatastream() {
  vid_shader = sg_make_shader(mpeg2_shader_desc(sg_query_backend()));}

void ds_advance(struct datastream *ds, double s) {
  if (ds->playing) {
    plm_decode(ds->plm, s);
  }
}

void ds_seek(struct datastream *ds, double time) {
  // clear_raw(ds->audio_device);
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
  if (ds->audio_device)
    close_audio_device(ds->audio_device);
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

#include "datastream.h"

#include "limits.h"
#include "texture.h"
#include <stdbool.h>
#include <stdlib.h>
#include "font.h"
#include "render.h"
#include <string.h>

#include "cbuf.h"

void datastream_free(JSRuntime *rt,datastream *ds)
{
//  sg_destroy_image(ds->img);
  plm_destroy(ds->plm);
  free(ds);
}

//void soundstream_fillbuf(struct datastream *ds, soundbyte *buf, int frames) {
//  for (int i = 0; i < frames*CHANNELS; i++)
//    buf[i] = ringshift(ds->ring);
//}

static void render_frame(plm_t *mpeg, plm_frame_t *frame, struct datastream *ds) {
  if (ds->dirty) return;
  uint8_t rgb[frame->height*frame->width*4];
  memset(rgb,255,frame->height*frame->width*4);
  plm_frame_to_rgba(frame, rgb, frame->width*4);
//  sg_image_data imgd = {0};
//  imgd.subimage[0][0] = SG_RANGE(rgb);
//  sg_update_image(ds->img, &imgd);
  ds->dirty = true;
}

static void render_audio(plm_t *mpeg, plm_samples_t *samples, struct datastream *ds) {
//  for (int i = 0; i < samples->count * CHANNELS; i++)
//    ringpush(ds->ring, samples->interleaved[i]);
}

struct datastream *ds_openvideo(void *raw, size_t rawlen)
{
  struct datastream *ds = malloc(sizeof(*ds));
  ds->plm = plm_create_with_memory(raw, rawlen, 0);
  free(raw);

  if (!ds->plm)
    return NULL;

/*  ds->img = sg_make_image(&(sg_image_desc){
    .width = plm_get_width(ds->plm),
    .height = plm_get_height(ds->plm),
    .usage = SG_USAGE_STREAM,
    .type = SG_IMAGETYPE_2D,
    .pixel_format = SG_PIXELFORMAT_RGBA8,
  });  
*/  
  plm_set_video_decode_callback(ds->plm, render_frame, ds);
  
  return ds;

//  ds->ring = ringnew(ds->ring, 8192);
//  plugin_node(make_node(ds, soundstream_fillbuf, NULL), masterbus);

  plm_set_audio_decode_callback(ds->plm, render_audio, ds);
  plm_set_loop(ds->plm, false);

  plm_set_audio_enabled(ds->plm, true);
  plm_set_audio_stream(ds->plm, 0);

  // Adjust the audio lead time according to the audio_spec buffer size
//  plm_set_audio_lead_time(ds->plm, BUF_FRAMES / SAMPLERATE);

  return ds;
}

void ds_advance(struct datastream *ds, double s) {
  ds->dirty = false;
  plm_decode(ds->plm, s);
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

double ds_length(struct datastream *ds) {
  return plm_get_duration(ds->plm);
}

#include "datastream.h"

#include "limits.h"
#include <stdbool.h>
#include <stdlib.h>
#include "font.h"
#include "render.h"
#include <string.h>

#include "cbuf.h"

void datastream_free(JSRuntime *rt,datastream *ds)
{
  plm_destroy(ds->plm);
  free(ds);
}

static void render_audio(plm_t *mpeg, plm_samples_t *samples, struct datastream *ds) {
//  for (int i = 0; i < samples->count * CHANNELS; i++)
//    ringpush(ds->ring, samples->interleaved[i]);
}

struct datastream *ds_openvideo(void *raw, size_t rawlen)
{
  struct datastream *ds = malloc(sizeof(*ds));
  void *newraw = malloc(rawlen);
  memcpy(newraw,raw,rawlen);
  ds->plm = plm_create_with_memory(newraw, rawlen, 1);

  if (!ds->plm) {
    free(ds);
    free(newraw);
    return NULL;
  }
    
  return ds;
}

void ds_advance(struct datastream *ds, double s) {
  plm_decode(ds->plm, s);
}

void ds_seek(struct datastream *ds, double time) {
  plm_seek(ds->plm, time, false);
}

double ds_length(struct datastream *ds) {
  return plm_get_duration(ds->plm);
}

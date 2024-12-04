#ifndef DATASTREAM_H
#define DATASTREAM_H

#include <pl_mpeg.h>
#include <stdint.h>
#include <quickjs.h>

#include <SDL3/SDL.h>

struct datastream {
  plm_t *plm;
  JSValue callback;
  JSContext *js;
  int width;
  int height;
};

typedef struct datastream datastream;

void datastream_free(JSRuntime *rt,datastream *ds);

struct datastream *ds_openvideo(void *raw, size_t rawlen);
void ds_advance(struct datastream *ds, double); // advance time in seconds
void ds_seek(struct datastream *ds, double);
void ds_pause(struct datastream *ds);
void ds_stop(struct datastream *ds);
int ds_videodone(struct datastream *ds);
double ds_remainingtime(struct datastream *ds);
double ds_length(struct datastream *ds);

#endif

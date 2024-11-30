#ifndef DATASTREAM_H
#define DATASTREAM_H

#include <pl_mpeg.h>
#include <stdint.h>
#include <quickjs.h>

struct soundstream;

struct datastream {
  plm_t *plm;
  int width;
  int height;
  int dirty;
};

typedef struct datastream datastream;

struct texture;

void datastream_free(JSRuntime *rt,datastream *ds);

struct datastream *ds_openvideo(void *raw, size_t rawlen);
struct texture *ds_maketexture(struct datastream *);
void ds_advance(struct datastream *ds, double);
void ds_seek(struct datastream *ds, double);
void ds_advanceframes(struct datastream *ds, int frames);
void ds_pause(struct datastream *ds);
void ds_stop(struct datastream *ds);
int ds_videodone(struct datastream *ds);
double ds_remainingtime(struct datastream *ds);
double ds_length(struct datastream *ds);

#endif

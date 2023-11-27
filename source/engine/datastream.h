#ifndef DATASTREAM_H
#define DATASTREAM_H

#include <pl_mpeg.h>
#include <stdint.h>
#include "dsp.h"

#include "sokol/sokol_gfx.h"

struct soundstream;

struct datastream {
  plm_t *plm;
  double last_time;
  int playing;
  sg_image img;
  int width;
  int height;
  soundbyte *ring;
};

struct Texture;

void MakeDatastream();
struct datastream *ds_openvideo(const char *path);
struct Texture *ds_maketexture(struct datastream *);
void ds_advance(struct datastream *ds, double);
void ds_seek(struct datastream *ds, double);
void ds_advanceframes(struct datastream *ds, int frames);
void ds_pause(struct datastream *ds);
void ds_stop(struct datastream *ds);
int ds_videodone(struct datastream *ds);
double ds_remainingtime(struct datastream *ds);
double ds_length(struct datastream *ds);

#endif

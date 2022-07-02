#ifndef DATASTREAM_H
#define DATASTREAM_H

#include <stdint.h>
#include <pl_mpeg.h>

struct datastream {
    plm_t *plm;
    struct mShader *shader;
    double last_time;
    int playing;
    int audio_device;
    uint32_t texture_y;
    uint32_t texture_cb;
    uint32_t texture_cr;
};

struct Texture;

extern struct mShader *vid_shader;

struct datastream *MakeDatastream();
void ds_openvideo(struct datastream *ds, const char *path, const char *adriver);
struct Texture *ds_maketexture(struct datastream*);
void ds_advance(struct datastream *ds, double);
void ds_seek(struct datastream *ds, double);
void ds_advanceframes(struct datastream *ds, int frames);
void ds_pause(struct datastream *ds);
void ds_stop(struct datastream *ds);
int ds_videodone(struct datastream *ds);
double ds_remainingtime(struct datastream *ds);
double ds_length(struct datastream *ds);

#endif

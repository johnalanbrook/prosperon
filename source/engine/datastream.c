#include "datastream.h"

#include "render.h"
#include "config.h"
#include "shader.h"
#include "resources.h"
#include "sound.h"
#include <stdbool.h>
#include "log.h"
#include "texture.h"
#include <stdlib.h>
#include "mix.h"
#include "limits.h"
#include "iir.h"
#include "dsp.h"

struct shader *vid_shader;

static void ds_update_texture(uint32_t unit, uint32_t texture, plm_plane_t * plane)
{
    glActiveTexture(unit);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, plane->width, plane->height, 0, GL_RED, GL_UNSIGNED_BYTE, plane->data);
}

static void render_frame(plm_t * mpeg, plm_frame_t * frame, void *user)
{
    struct datastream *ds = user;
    shader_use(ds->shader);
    ds_update_texture(GL_TEXTURE0, ds->texture_y, &frame->y);
    ds_update_texture(GL_TEXTURE1, ds->texture_cb, &frame->cb);
    ds_update_texture(GL_TEXTURE2, ds->texture_cr, &frame->cr);
}

static void render_audio(plm_t * mpeg, plm_samples_t * samples, void *user)
{
    struct datastream *ds = user;
    short t;

    for (int i = 0; i < samples->count * CHANNELS; i++) {
        t = (short)(samples->interleaved[i] * SHRT_MAX);
        cbuf_push(ds->astream->buf, t*5);
    }
}

struct Texture *ds_maketexture(struct datastream *ds)
{
    struct Texture *new = malloc(sizeof(*new));
    new->id = ds->texture_cb;
    new->width = 500;
    new->height = 500;
    return new;
}

void ds_openvideo(struct datastream *ds, const char *video, const char *adriver)
{
   // ds_stop(ds);
    char buf[MAXPATH] = {'\0'};
    sprintf(buf, "%s%s", "video/", video);
    ds->plm = plm_create_with_filename(buf);

    if (!ds->plm) {
        YughLog(0, 0, "Couldn't open %s", video);
    }

    YughLog(0, 0, "Opened %s - framerate: %f, samplerate: %d, audio streams: %i, duration: %f",
	    video,
	    plm_get_framerate(ds->plm),
	    plm_get_samplerate(ds->plm),
	    plm_get_num_audio_streams(ds->plm),
	    plm_get_duration(ds->plm)
	);

    ds->astream = soundstream_make();
    struct dsp_filter astream_filter;
    astream_filter.data = &ds->astream;
    astream_filter.filter = soundstream_fillbuf;

    //struct dsp_filter lpf = lpf_make(8, 10000);
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

    //first_free_bus(astream_filter);

    plm_set_video_decode_callback(ds->plm, render_frame, ds);
    plm_set_audio_decode_callback(ds->plm, render_audio, ds);
    plm_set_loop(ds->plm, false);

    plm_set_audio_enabled(ds->plm, true);
    plm_set_audio_stream(ds->plm, 0);

    // Adjust the audio lead time according to the audio_spec buffer size
    plm_set_audio_lead_time(ds->plm, BUF_FRAMES/SAMPLERATE);

    ds->playing = true;
}

struct datastream *MakeDatastream()
{
    struct datastream *newds = malloc(sizeof(*newds));
    if (!vid_shader) vid_shader = MakeShader("videovert.glsl", "videofrag.glsl");

    newds->shader = vid_shader;
    shader_use(newds->shader);
    glGenTextures(1, &newds->texture_y);
    glBindTexture(GL_TEXTURE_2D, newds->texture_y);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    shader_setint(newds->shader, "texture_y", 0);

    glGenTextures(1, &newds->texture_cb);
    glBindTexture(GL_TEXTURE_2D, newds->texture_cb);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    shader_setint(newds->shader, "texture_cb", 1);

    glGenTextures(1, &newds->texture_cr);
    glBindTexture(GL_TEXTURE_2D, newds->texture_cr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    shader_setint(newds->shader, "texture_cr", 2);

    return newds;
}

void ds_advance(struct datastream *ds, double s)
{
    if (ds->playing) {
	plm_decode(ds->plm, s);
    }
}

void ds_seek(struct datastream *ds, double time)
{
    //clear_raw(ds->audio_device);
    plm_seek(ds->plm, time, false);
}

void ds_advanceframes(struct datastream *ds, int frames)
{
    for (int i = 0; i < frames; i++) {
	plm_frame_t *frame = plm_decode_video(ds->plm);
	render_frame(ds->plm, frame, ds);
    }
}

void ds_pause(struct datastream *ds)
{
    ds->playing = false;
}

void ds_stop(struct datastream *ds)
{
    if (ds->plm != NULL) {
	plm_destroy(ds->plm);
	ds->plm = NULL;
    }
    if (ds->audio_device)
	close_audio_device(ds->audio_device);
    ds->playing = false;
}

// TODO: Must be a better way
int ds_videodone(struct datastream *ds)
{
    return (ds->plm == NULL)
	|| plm_get_time(ds->plm) >= plm_get_duration(ds->plm);
}

double ds_remainingtime(struct datastream *ds)
{
    if (ds->plm != NULL)
	return plm_get_duration(ds->plm) - plm_get_time(ds->plm);
    else
	return 0.f;
}

double ds_length(struct datastream *ds)
{
    return plm_get_duration(ds->plm);
}

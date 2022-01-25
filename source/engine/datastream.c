#include "datastream.h"

#include <pl_mpeg.h>

#include "config.h"
#include "shader.h"
#include "resources.h"
#include <GL/glew.h>
#include <stdbool.h>

static void ds_update_texture(uint32_t unit, uint32_t texture,
			      plm_plane_t * plane)
{
    glActiveTexture(unit);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, plane->width, plane->height, 0,
		 GL_RED, GL_UNSIGNED_BYTE, plane->data);
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
    struct datastream *ds = (struct datastream *) user;
    int size = sizeof(float) * samples->count * 2;
    SDL_QueueAudio(ds->audio_device, samples->interleaved, size);
}

void ds_openvideo(struct datastream *ds, const char *video,
		  const char *adriver)
{
    ds_stop(ds);
    char buf[MAXPATH] = {'\0'};
    sprintf(buf, "%s%s", DATA_PATH, video);
    ds->plm = plm_create_with_filename(buf);

    if (!ds->plm) {
	SDL_Log("Couldn't open %s", video);
    }

    int samplerate = plm_get_samplerate(ds->plm);

    SDL_Log("Opened %s - framerate: %f, samplerate: %d, duration: %f",
	    video,
	    plm_get_framerate(ds->plm),
	    plm_get_samplerate(ds->plm), plm_get_duration(ds->plm)
	);

    plm_set_video_decode_callback(ds->plm, render_frame, ds);
    plm_set_audio_decode_callback(ds->plm, render_audio, ds);
    plm_set_loop(ds->plm, false);

    plm_set_audio_enabled(ds->plm, true);
    plm_set_audio_stream(ds->plm, 0);
    SDL_AudioSpec audio_spec;
    SDL_memset(&audio_spec, 0, sizeof(audio_spec));
    audio_spec.freq = samplerate;
    audio_spec.format = AUDIO_F32;
    audio_spec.channels = 2;
    audio_spec.samples = 4096;

    ds->audio_device =
	SDL_OpenAudioDevice(adriver, 0, &audio_spec, NULL, 0);
    printf("Opened audio device %d on driver %s\n", ds->audio_device,
	   adriver);
    // if (audio_device == 0) {
    //     SDL_Log("Failed to open audio device: %s", SDL_GetError());
    // }
    SDL_PauseAudioDevice(ds->audio_device, 0);




    // Adjust the audio lead time according to the audio_spec buffer size
    //plm_set_audio_lead_time(plm, (double)audio_spec.samples / (double)samplerate);

    ds->playing = true;
}

struct datastream *MakeDatastream(struct mShader *shader)
{
    struct datastream *newds =
	(struct datastream *) malloc(sizeof(struct datastream));
    newds->shader = shader;
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

void ds_advance(struct datastream *ds, uint32_t ms)
{
    if (ds->playing) {
	double advanceTime = ms / 1000.f;
	plm_decode(ds->plm, advanceTime);
    }
}

void ds_seek(struct datastream *ds, uint32_t time)
{
    SDL_ClearQueuedAudio(0);
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
	SDL_CloseAudioDevice(ds->audio_device);
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
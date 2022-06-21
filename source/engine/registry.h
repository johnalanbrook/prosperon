#ifndef REGISTRY_H
#define REGISTRY_H

#include <stddef.h>
#include "config.h"

struct mGameObject;

struct component {
    const char *name;
    void *(*make)(struct mGameObject * go);
    void *data;
    struct mGameObject *go;
    void (*draw_debug)(void *data);
    void (*draw_gui)(void *data);
    void (*update)(void *data, struct mGameObject * go);
    int id;
    int datasize;
    void (*init)(void *data, struct mGameObject * go);
};

extern struct component components[MAXNAME];
extern int ncomponent;

void comp_draw_debug(struct component *c);
void comp_draw_gui(struct component *c);
void comp_update(struct component *c, struct mGameObject *go);


void registry_init();
void register_component(const char *name, size_t size,
			void (*make)(struct mGameObject * go,
				     struct component * c),
			void(*draw_debug)(void *data),
			void(*draw_gui)(void *data),
			void(*init)(void *data, struct mGameObject * go),
			void(*update)(void *data,
				      struct mGameObject * go));

#endif

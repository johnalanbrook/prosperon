#ifndef REGISTRY_H
#define REGISTRY_H

#include <stddef.h>
#include <stdio.h>

#include "config.h"

struct gameobject;

struct component_interface {
    const char *name;
    void *(*make)(struct gameobject * go);		/* Called to create the component */
    void (*io)(void *data, FILE *f, int read);
    void (*draw_debug)(void *data);				/* Draw debugging info in editor */
    void (*draw_gui)(void *data);
    void (*init)(void *data, struct gameobject * go);
    void (*delete)(void *data);
};

struct component {
    void *data;
    struct gameobject *go;
    struct component_interface *ref;
};

struct component comp_make(struct component_interface *interface);
void comp_draw_debug(struct component *c);
void comp_draw_gui(struct component *c);
void comp_delete(struct component *c);
void comp_init(struct component *c, struct gameobject *go);
void comp_io(struct component *c, FILE *f, int read);

void comp_update(struct component *c, struct gameobject *go);


void registry_init();
void register_component(const char *name, size_t size,
			void (*make)(struct gameobject * go),
			void (*delete)(void *data),
			void (*io)(void *data, FILE *f, int read),
			void(*draw_debug)(void *data),
			void(*draw_gui)(void *data),
			void(*init)(void *data, struct gameobject * go));

#endif

#ifndef REGISTRY_H
#define REGISTRY_H

#include <stddef.h>
#include <stdio.h>

#include "config.h"

struct gameobject;

struct component {
    const char *name;
    void *(*make)(struct gameobject * go);		/* Called to create the component */
    void (*io)(void *data, FILE *f, int read);			/* Pulls data from a component file into the component */
    void *data;
    struct gameobject *go;
    void (*draw_debug)(void *data);				/* Draw debugging info in editor */
    void (*draw_gui)(void *data);				/* Use to draw GUI for editing the component in editor */
    void (*delete)(void *data);					/* Deletes and cleans up component */
    int id;
    int datasize;
    void (*init)(void *data, struct gameobject * go);	/* Inits the component */
};

extern struct component components[MAXNAME];
extern int ncomponent;

void comp_draw_debug(struct component *c);
void comp_draw_gui(struct component *c);
void comp_update(struct component *c, struct gameobject *go);


void registry_init();
void register_component(const char *name, size_t size,
			void (*make)(struct gameobject * go, struct component * c),
			void (*delete)(void *data),
			void (*io)(void *data, FILE *f, int read),
			void(*draw_debug)(void *data),
			void(*draw_gui)(void *data),
			void(*init)(void *data, struct gameobject * go));

#endif

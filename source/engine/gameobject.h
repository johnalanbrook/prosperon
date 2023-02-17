#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include <stdio.h>
#include "mathc.h"
#include "config.h"
#include <stdbool.h>
#include <chipmunk/chipmunk.h>
#include "2dphysics.h"

struct shader;
struct sprite;
struct component;

struct gameobject {
    cpBodyType bodytype;
    int next;
    float scale;
    float mass;
    float f;			/* friction */
    float e;			/* elasticity */
    int flipx; /* 1 or -1 */
    int flipy;
    cpShapeFilter filter;
    cpBody *body; /* NULL if this object is dead */
    int id;
    struct phys_cbs cbs;
    struct shape_cb *shape_cbs;
};

extern struct gameobject *gameobjects;

int MakeGameobject();
void gameobject_apply(struct gameobject *go);
void gameobject_delete(int id);
void gameobjects_cleanup();
void toggleprefab(struct gameobject *go);

struct gameobject *get_gameobject_from_id(int id);
struct gameobject *id2go(int id);
int id_from_gameobject(struct gameobject *go);
int body2id(cpBody *body);
cpBody *id2body(int id);

void go_shape_apply(cpBody *body, cpShape *shape, struct gameobject *go);

void gameobject_save(struct gameobject *go, FILE * file);

void gameobject_saveprefab(struct gameobject *go);
void gameobject_syncprefabs(char *revertPath);
void gameobject_revertprefab(struct gameobject *go);

/* Tries a few methods to select a gameobject; if none is selected returns -1 */
int pos2gameobject(cpVect pos);

void gameobject_init(struct gameobject *go, FILE * fprefab);

void gameobject_move(struct gameobject *go, cpVect vec);
void gameobject_rotate(struct gameobject *go, float as);
void gameobject_setangle(struct gameobject *go, float angle);
void gameobject_setpos(struct gameobject *go, cpVect vec);

void gameobject_draw_debugs();

void object_gui(struct gameobject *go);

void gameobject_saveall();
void gameobject_loadall();
int gameobjects_saved();

#endif

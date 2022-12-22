#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include <stdio.h>
#include "mathc.h"
#include "transform.h"
#include "config.h"
#include <stdbool.h>
#include <chipmunk/chipmunk.h>
#include "2dphysics.h"

struct shader;
struct sprite;
struct component;

struct editor {
    mfloat_t color[3];
    int id;
    bool active;
    bool prefabSync;
    char mname[MAXNAME];
    char *curPrefabPath;
    char prefabName[MAXNAME];
    char rootPrefabName[MAXNAME];
};

struct go_temp {
    struct phys_cbs phys_cbs;
};

struct gameobject {
    struct mTransform transform;
    struct editor editor;
    cpBodyType bodytype;
    float scale;
    float mass;
    cpBody *body;
    float f;			/* friction */
    float e;			/* elasticity */
    struct component *components;
    struct phys_cbs *cbs;
};

extern struct gameobject *gameobjects;

struct gameobject *MakeGameobject();
void init_gameobjects();
void gameobject_delete(int id);
void clear_gameobjects();
int number_of_gameobjects();
void set_n_gameobjects(int n);
void setup_model_transform(struct mTransform *t, struct shader *s, float scale);
void toggleprefab(struct gameobject *go);

struct gameobject *get_gameobject_from_id(int id);
int id_from_gameobject(struct gameobject *go);

void gameobject_save(struct gameobject *go, FILE * file);
void gameobject_addcomponent(struct gameobject *go, struct component *c);
void gameobject_delcomponent(struct gameobject *go, int n);
void gameobject_loadcomponent(struct gameobject *go, int id);

void gameobject_saveprefab(struct gameobject *go);
int gameobject_makefromprefab(char *path);
void gameobject_syncprefabs(char *revertPath);
void gameobject_revertprefab(struct gameobject *go);

void gameobject_init(struct gameobject *go, FILE * fprefab);

void gameobject_move(struct gameobject *go, float xs, float ys);
void gameobject_rotate(struct gameobject *go, float as);
void gameobject_setangle(struct gameobject *go, float angle);
void gameobject_setpos(struct gameobject *go, float x, float y);

void gameobject_draw_debugs();

void object_gui(struct gameobject *go);

#endif

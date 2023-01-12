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

struct gameobject {
      union {
          cpBodyType bodytype;
          struct gameobject *next;
     };
    float scale;
    float mass;
    float f;			/* friction */
    float e;			/* elasticity */
    int flipx; /* 1 or -1 */
    int flipy;
    cpBody *body;
    int id;
    struct phys_cbs cbs;
};

extern struct gameobject *gameobjects;

int MakeGameobject();
void gameobject_apply(struct gameobject *go);
void gameobject_delete(int id);
void toggleprefab(struct gameobject *go);

struct gameobject *get_gameobject_from_id(int id);
struct gameobject *id2go(int id);
int id_from_gameobject(struct gameobject *go);

void gameobject_save(struct gameobject *go, FILE * file);

void gameobject_saveprefab(struct gameobject *go);
int gameobject_makefromprefab(char *path);
void gameobject_syncprefabs(char *revertPath);
void gameobject_revertprefab(struct gameobject *go);

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

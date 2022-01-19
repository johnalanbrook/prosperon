#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include <stdio.h>
#include "mathc.h"
#include "transform.h"
#include "config.h"
#include <stdbool.h>
#include <chipmunk.h>

struct mShader;
struct mSprite;
struct component;
struct vec;

extern struct mGameObject *updateGO;
extern struct vec *gameobjects;

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

struct mGameObject {
    struct mTransform transform;
    struct editor editor;
    cpBodyType bodytype;
    float scale;
    float mass;
    cpBody *body;
    float f;			/* friction */
    float e;			/* elasticity */
    struct vec *components;
    char *start;
    char *update;
    char *fixedupdate;
    char *stop;
};

struct mGameObject *MakeGameobject();
void init_gameobjects();
void gameobject_delete(int id);
void clear_gameobjects();
int number_of_gameobjects();
void set_n_gameobjects(int n);
void setup_model_transform(struct mTransform *t, struct mShader *s,
			   float scale);
void toggleprefab(struct mGameObject *go);
struct mGameObject *get_gameobject_from_id(int id);
void gameobject_save(struct mGameObject *go, FILE * file);
void gameobject_addcomponent(struct mGameObject *go, struct component *c);
void gameobject_delcomponent(struct mGameObject *go, int n);
void gameobject_loadcomponent(struct mGameObject *go, int id);

void gameobject_saveprefab(struct mGameObject *go);
void gameobject_makefromprefab(char *path);
void gameobject_syncprefabs(char *revertPath);
void gameobject_revertprefab(struct mGameObject *go);

void gameobject_init(struct mGameObject *go, FILE * fprefab);

void gameobject_update(struct mGameObject *go);
void update_gameobjects();

void gameobject_move(struct mGameObject *go, float xs, float ys);
void gameobject_rotate(struct mGameObject *go, float as);

#endif

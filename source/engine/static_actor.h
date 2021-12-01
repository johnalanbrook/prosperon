#ifndef STATIC_ACTOR_H
#define STATIC_ACTOR_H

#include "gameobject.h"
#include "model.h"
#include "shader.h"

struct mStaticActor {
    struct mGameObject obj;
    struct mModel *model;
    char *modelPath;
    char currentModelPath[MAXPATH];
    bool castShadows;
};

void staticactor_draw_dbg_color_pick(struct mShader *s);
void staticactor_draw_models(struct mShader *s);
void staticactor_draw_shadowcasters(struct mShader *s);
struct mStaticActor *MakeStaticActor();


extern struct mStaticActor *curActor;

#endif

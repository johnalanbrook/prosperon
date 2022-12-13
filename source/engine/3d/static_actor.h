#ifndef STATIC_ACTOR_H
#define STATIC_ACTOR_H

#include "gameobject.h"
#include "model.h"
#include "shader.h"

struct mStaticActor {
    struct gameobject obj;
    struct model *model;
    char *modelPath;
    char currentModelPath[MAXPATH];
    bool castShadows;
};

void staticactor_draw_dbg_color_pick(struct shader *s);
void staticactor_draw_models(struct shader *s);
void staticactor_draw_shadowcasters(struct shader *s);
struct mStaticActor *MakeStaticActor();
void staticactor_gui(struct mStaticActor *sa);

extern struct mStaticActor *curActor;

#endif

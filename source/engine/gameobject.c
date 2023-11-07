#include "gameobject.h"

#include "2dphysics.h"
#include "debugdraw.h"
#include "input.h"
#include "log.h"
#include "resources.h"
#include "script.h"
#include "shader.h"
#include "sprite.h"
#include <chipmunk/chipmunk.h>
#include <string.h>
#include "debugdraw.h"

#include "stb_ds.h"

struct gameobject *gameobjects = NULL;
static int first = -1;

const int nameBuf[MAXNAME] = {0};
const int prefabNameBuf[MAXNAME] = {0};

struct gameobject *get_gameobject_from_id(int id) {
  if (id < 0) return NULL;

  return &gameobjects[id];
}

struct gameobject *id2go(int id) {
  if (id < 0) return NULL;

  return &gameobjects[id];
}

int body2id(cpBody *body) {
  return (int)cpBodyGetUserData(body);
}

cpBody *id2body(int id) {
  struct gameobject *go = id2go(id);

  if (go)
    return go->body;

  return NULL;
}

int shape2gameobject(cpShape *shape) {
  struct phys2d_shape *s = cpShapeGetUserData(shape);
  return s->go;
}

struct gameobject *shape2go(cpShape *shape)
{
  return id2go(shape2gameobject(shape));
}

int pos2gameobject(cpVect pos) {
  cpShape *hit = phys2d_query_pos(pos);

  if (hit) {
    return shape2gameobject(hit);
  }

  for (int i = 0; i < arrlen(gameobjects); i++) {
    if (!gameobjects[i].body) continue;
    cpVect gpos = cpBodyGetPosition(gameobjects[i].body);
    float dist = cpvlength(cpvsub(gpos, pos));

    if (dist <= 25) return i;
  }
  return -1;
}

int id_from_gameobject(struct gameobject *go) {
  for (int i = 0; i < arrlen(gameobjects); i++) {
    if (&gameobjects[i] == go) return i;
  }

  return -1;
}

void gameobject_set_sensor(int id, int sensor) {
  id2go(id)->sensor = sensor;
  gameobject_apply(id2go(id));
}

int go2id(struct gameobject *go) {
  return id_from_gameobject(go);
}

uint32_t go2category(struct gameobject *go)
{
  return 0;
}

uint32_t go2mask(struct gameobject *go)
{
  return 0;
}

void go_shape_apply(cpBody *body, cpShape *shape, struct gameobject *go) {
  cpShapeSetFriction(shape, go->f);
  cpShapeSetElasticity(shape, go->e);
  cpShapeSetCollisionType(shape, go2id(go));

  cpShapeFilter filter;
  filter.group = go2id(go);
  filter.categories = 1<<go->layer;
  filter.mask = category_masks[go->layer];
  cpShapeSetFilter(shape, filter);

  struct phys2d_shape *ape = cpShapeGetUserData(shape);
  if (ape && ape->apply)
    ape->apply(ape->data);
}

void go_shape_moi(cpBody *body, cpShape *shape, struct gameobject *go) {
  float moment = cpBodyGetMoment(go->body);
  struct phys2d_shape *s = cpShapeGetUserData(shape);
  if (!s) {
    cpBodySetMoment(go->body, moment + 1);
    return;
  }

  moment += s->moi(s->data, go->mass);
  if (moment < 0) moment = 1;
  cpBodySetMoment(go->body, 1);
}

void gameobject_apply(struct gameobject *go) {
  cpBodySetType(go->body, go->bodytype);
  cpBodyEachShape(go->body, go_shape_apply, go);

  if (go->bodytype == CP_BODY_TYPE_DYNAMIC) {
    cpBodySetMass(go->body, go->mass);
    cpBodySetMoment(go->body, 0.f);
    cpBodyEachShape(go->body, go_shape_moi, go);

    if (cpBodyGetMoment(go->body) <= 0.f)
      cpBodySetMoment(go->body, 1.f);

    return;
  }
}

static void gameobject_setpickcolor(struct gameobject *go) {
  /*
      float r = ((go->editor.id & 0x000000FF) >> 0) / 255.f;
      float g = ((go->editor.id & 0x0000FF00) >> 8) / 255.f;
      float b = ((go->editor.id & 0x00FF0000) >> 16) / 255.f;

      go->editor.color[0] = r;
      go->editor.color[1] = g;
      go->editor.color[2] = b;
      */
}

static void velocityFn(cpBody *body, cpVect gravity, cpFloat damping, cpFloat dt)
{
  struct gameobject *go = id2go(cpBodyGetUserData(body));
  if (!go)
    cpBodyUpdateVelocity(body,gravity,damping,dt);

//  cpFloat d = isnan(go->damping) ? damping : d;
  cpVect g = go->gravity ? gravity : cpvzero;

  cpBodyUpdateVelocity(body,g,damping,dt);
  if (!isinf(go->maxvelocity))
    cpBodySetVelocity(body, cpvclamp(cpBodyGetVelocity(body), go->maxvelocity));
    
  if (!isinf(go->maxangularvelocity)) {
    float av = cpBodyGetAngularVelocity(body);
    if (fabs(av) > go->maxangularvelocity)
      cpBodySetAngularVelocity(body, copysignf(go->maxangularvelocity, av));
  }
}

int MakeGameobject() {
  struct gameobject go = {
      .scale = 1.f,
      .scale3 = (HMM_Vec3){1.f,1.f,1.f},
      .bodytype = CP_BODY_TYPE_STATIC,
      .maxvelocity = INFINITY,
      .maxangularvelocity = INFINITY,
      .mass = 1.f,
      .next = -1,
      .sensor = 0,
      .shape_cbs = NULL,
      .ref = JS_NULL,
  };

  go.cbs.begin.obj = JS_NULL;
  go.cbs.separate.obj = JS_NULL;

  go.body = cpSpaceAddBody(space, cpBodyNew(go.mass, 1.f));
  cpBodySetVelocityUpdateFunc(go.body, velocityFn);

  int retid;

  if (first < 0) {
    arrput(gameobjects, go);
    retid = arrlast(gameobjects).id = arrlen(gameobjects) - 1;
  } else {
    retid = first;
    first = id2go(first)->next;
    *id2go(retid) = go;
  }

  cpBodySetUserData(go.body, (void *)retid);
  phys2d_setup_handlers(retid);
  return retid;
}

void rm_body_shapes(cpBody *body, cpShape *shape, void *data) {
  struct phys2d_shape *s = cpShapeGetUserData(shape);
  if (s->data) {
    free(s->data);
    s->data = NULL;
  }
  cpSpaceRemoveShape(space, shape);
  cpShapeFree(shape);
}

int *go_toclean = NULL;

/* Free this gameobject */
void gameobject_clean(int id) {
  struct gameobject *go = id2go(id);
  arrfree(go->shape_cbs);
  cpBodyEachShape(go->body, rm_body_shapes, NULL);
  cpSpaceRemoveBody(space, go->body);
  cpBodyFree(go->body);
  go->body = NULL;
}

/* Really more of a "mark for deletion" ... */
void gameobject_delete(int id) {
  id2go(id)->next = first;
  JS_FreeValue(js, id2go(id)->ref);
  first = id;

  if (cpSpaceIsLocked(space))
    arrpush(go_toclean, id);
  else
    gameobject_clean(id);
}

void gameobjects_cleanup() {
  for (int i = 0; i < arrlen(go_toclean); i++)
    gameobject_clean(go_toclean[i]);

  arrsetlen(go_toclean, 0);

  return;

  int clean = first;

  while (clean >= 0 && id2go(clean)->body) {
    gameobject_clean(clean);
    clean = id2go(clean)->next;
  }
}

void gameobject_move(struct gameobject *go, cpVect vec) {
  cpVect p = cpBodyGetPosition(go->body);
  p.x += vec.x;
  p.y += vec.y;
  cpBodySetPosition(go->body, p);

  phys2d_reindex_body(go->body);
}

void gameobject_rotate(struct gameobject *go, float as) {
  cpFloat a = cpBodyGetAngle(go->body);
  a += as * deltaT;
  cpBodySetAngle(go->body, a);

  phys2d_reindex_body(go->body);
}

void gameobject_setangle(struct gameobject *go, float angle) {
  cpBodySetAngle(go->body, angle);
  phys2d_reindex_body(go->body);
}

void gameobject_setpos(struct gameobject *go, cpVect vec) {
  if (!go || !go->body) return;
  cpBodySetPosition(go->body, vec);

  phys2d_reindex_body(go->body);
}

void body_draw_shapes_dbg(cpBody *body, cpShape *shape, void *data) {
  struct phys2d_shape *s = cpShapeGetUserData(shape);
  s->debugdraw(s->data);
}

void gameobject_draw_debug(int go) {
  struct gameobject *g = id2go(go);
  if (!g || !g->body) return;

  cpVect pos = cpBodyGetPosition(g->body);
  struct rgba color = {
    .r = 0.76*255,
    .b = 0.38*255,
    .g = 255,
    .a = 255
  };

  cpBodyEachShape(g->body, body_draw_shapes_dbg, NULL);
}

void gameobject_draw_debugs() {
  for (int i = 0; i < arrlen(gameobjects); i++)
    gameobject_draw_debug(i);
}

static struct {
  struct gameobject go;
  cpVect pos;
  float angle;
} *saveobjects = NULL;

void gameobject_saveall() {
  arrfree(saveobjects);
  arrsetlen(saveobjects, arrlen(gameobjects));

  for (int i = 0; i < arrlen(gameobjects); i++) {
    saveobjects[i].go = gameobjects[i];
    saveobjects[i].pos = cpBodyGetPosition(gameobjects[i].body);
    saveobjects[i].angle = cpBodyGetAngle(gameobjects[i].body);
  }
}

void gameobject_loadall() {
  YughInfo("N gameobjects: %d, N saved: %d", arrlen(gameobjects), arrlen(saveobjects));
  for (int i = 0; i < arrlen(saveobjects); i++) {
    gameobjects[i] = saveobjects[i].go;
    cpBodySetPosition(gameobjects[i].body, saveobjects[i].pos);
    cpBodySetAngle(gameobjects[i].body, saveobjects[i].angle);
    cpBodySetVelocity(gameobjects[i].body, cpvzero);
    cpBodySetAngularVelocity(gameobjects[i].body, 0.f);
  }

  arrfree(saveobjects);
}

int gameobjects_saved() {
  return arrlen(saveobjects);
}

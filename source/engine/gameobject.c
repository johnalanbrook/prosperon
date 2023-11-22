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

HMM_Vec2 go2pos(struct gameobject *go)
{
  cpVect p = cpBodyGetPosition(go->body);
  return (HMM_Vec2){p.x, p.y};
}

float go2angle(struct gameobject *go)
{
  return cpBodyGetAngle(go->body);
}

transform2d mat2transform2d(HMM_Mat3 mat)
{
  
}

HMM_Mat3 mt_t(transform2d t)
{
  HMM_Mat3 p = HMM_M3D(1);
  p.Columns[2].X = t.pos.X;
  p.Columns[2].Y = t.pos.Y;
  return p;
}

HMM_Mat3 mt_s(transform2d t)
{
  HMM_Mat3 s = HMM_M3D(1);
  s.Columns[0].X = t.scale.X;
  s.Columns[1].Y = t.scale.Y;
  return s;
}

HMM_Mat3 mt_r(transform2d t)
{
  HMM_Mat3 r = HMM_M3D(1);
  r.Columns[0] = (HMM_Vec3){cos(t.angle), sin(t.angle), 0};
  r.Columns[1] = (HMM_Vec3){-sin(t.angle), cos(t.angle), 0};
  return r;
}

HMM_Mat3 transform2d2mat(transform2d t)
{
  return HMM_MulM3(mt_t(t), HMM_MulM3(mt_r(t), mt_s(t)));
}

transform3d mat2transform3d(HMM_Mat4 mat)
{
  
}

transform3d go2t3(gameobject *go)
{
  transform3d t;
  HMM_Vec2 p = go2pos(go);
  t.pos.Y = p.Y;
  t.pos.X = p.X;
  t.scale = go->scale;
  t.scale.Z = go->scale.X;
  t.rotation = HMM_QFromAxisAngle_RH(vFWD, go2angle(go));
  t.rotation = HMM_MulQ(HMM_QFromAxisAngle_RH(vRIGHT, -t.pos.Y/100), t.rotation);
  t.rotation = HMM_MulQ(HMM_QFromAxisAngle_RH(vUP, t.pos.X/100), t.rotation);
  return t;
}

HMM_Mat4 m4_t(transform3d t)
{
  return HMM_Translate(t.pos);
}

HMM_Mat4 m4_s(transform3d t)
{
  return HMM_Scale(t.scale);
}

HMM_Mat4 m4_r(transform3d t)
{
  return HMM_QToM4(t.rotation);
}

HMM_Mat4 m4_st(transform3d t)
{
  return HMM_MulM4(m4_t(t), m4_s(t));
}

HMM_Mat4 m4_rt(transform3d t)
{
  return HMM_MulM4(m4_t(t), m4_r(t));
}

HMM_Mat4 m4_rst(transform3d t)
{
  return HMM_MulM4(m4_st(t), m4_r(t));
}

HMM_Mat4 transform3d2mat(transform3d t)
{
  return m4_rst(t);
}

HMM_Mat3 mt_rst(transform2d t)
{
  return transform2d2mat(t);
}

HMM_Mat3 mt_st(transform2d t)
{
  return HMM_MulM3(mt_t(t), mt_s(t));
}

HMM_Mat3 mt_rt(transform2d t)
{
  return HMM_MulM3(mt_t(t), mt_r(t));
}

HMM_Vec2 go2world(struct gameobject *go, HMM_Vec2 pos)
{
  HMM_Vec2 v = HMM_MulM3V3(t_go2world(go), (HMM_Vec3){pos.X, pos.Y, 1.0}).XY;
  return v;
}

HMM_Vec2 world2go(struct gameobject *go, HMM_Vec2 pos)
{
  return HMM_MulM3V3(t_world2go(go), (HMM_Vec3){pos.X, pos.Y, 1.0}).XY;
}

HMM_Vec2 mat_t_pos(HMM_Mat3 m, HMM_Vec2 pos)
{
  return HMM_MulM3V3(m, (HMM_Vec3){pos.x, pos.y, 1}).XY;
}

HMM_Vec2 mat_t_dir(HMM_Mat3 m, HMM_Vec2 dir)
{
  m.Columns[2] = (HMM_Vec3){0,0,1};
  return HMM_MulM3V3(m, (HMM_Vec3){dir.x, dir.y, 1}).XY;
}

HMM_Vec3 mat3_t_pos(HMM_Mat4 m, HMM_Vec3 pos)
{
  return HMM_MulM4V4(m, (HMM_Vec4){pos.X, pos.Y, pos.Z, 1}).XYZ;
}

HMM_Vec3 mat3_t_dir(HMM_Mat4 m, HMM_Vec3 dir)
{
  m.Columns[4] = (HMM_Vec4){0,0,0,1};
  return mat3_t_pos(m, dir);
}


HMM_Vec2 goscale(struct gameobject *go, HMM_Vec2 pos)
{
  return HMM_MulV2(go->scale.XY, pos);
}

HMM_Mat3 t_go2world(struct gameobject *go)
{
  return transform2d2mat(go2t(go));
}

HMM_Mat3 t_world2go(struct gameobject *go)
{
  return HMM_InvGeneralM3(transform2d2mat(go2t(go)));
}


int pos2gameobject(HMM_Vec2 pos) {
  cpShape *hit = phys2d_query_pos(pos.cp);

  if (hit) {
    return shape2gameobject(hit);
  }

  for (int i = 0; i < arrlen(gameobjects); i++) {
    if (!gameobjects[i].body) continue;
    cpVect gpos = cpBodyGetPosition(gameobjects[i].body);
    float dist = cpvlength(cpvsub(gpos, pos.cp));

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

transform2d go2t(gameobject *go)
{
  transform2d t;
  t.pos.cp = cpBodyGetPosition(go->body);
  t.angle = cpBodyGetAngle(go->body);
  t.scale = go->scale.XY;
  if (isnan(t.scale.X)) t.scale.X = 1;
  if (isnan(t.scale.Y)) t.scale.Y = 1;
  return t;
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

unsigned int editor_cat = 1<<31;

void go_shape_apply(cpBody *body, cpShape *shape, struct gameobject *go) {
  cpShapeSetFriction(shape, go->f);
  cpShapeSetElasticity(shape, go->e);
  cpShapeSetCollisionType(shape, go2id(go));

  cpShapeFilter filter;
  filter.group = go2id(go);
  filter.categories = 1<<go->layer | editor_cat;
//  filter.mask = CP_ALL_CATEGORIES;
  filter.mask = category_masks[go->layer] | editor_cat;
//  filter.mask = CP_ALL_CATEGORIES;
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
  struct gameobject *go = id2go((int)cpBodyGetUserData(body));
  if (!go) {
    cpBodyUpdateVelocity(body,gravity,damping,dt);
    return;
  }

  cpFloat d = isnan(go->damping) ? damping : d;
  cpVect g = go->gravity ? gravity : go->cgravity.cp;
  
  cpBodyUpdateVelocity(body,g,d,dt*go->timescale);

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
      .scale = (HMM_Vec3){1.f,1.f,1.f},
      .bodytype = CP_BODY_TYPE_STATIC,
      .maxvelocity = INFINITY,
      .maxangularvelocity = INFINITY,
      .mass = 1.f,
      .next = -1,
      .sensor = 0,
      .drawlayer = 0,
      .shape_cbs = NULL,
      .gravity = 1,
      .cgravity = (HMM_Vec2){0,0},
      .damping = NAN,
      .timescale = 1.0,
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

void gameobject_move(struct gameobject *go, HMM_Vec2 vec) {
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

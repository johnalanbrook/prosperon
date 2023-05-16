#include "gameobject.h"

#include "2dphysics.h"
#include "debugdraw.h"
#include "input.h"
#include "log.h"
#include "nuke.h"
#include "resources.h"
#include "script.h"
#include "shader.h"
#include "sprite.h"
#include <chipmunk/chipmunk.h>
#include <string.h>

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

void go_shape_apply(cpBody *body, cpShape *shape, struct gameobject *go) {
  cpShapeSetFriction(shape, go->f);
  cpShapeSetElasticity(shape, go->e);
  cpShapeSetCollisionType(shape, go2id(go));

  /*    cpShapeFilter filter;
      filter.group = go2id(go);
      filter.categories = 1<<go->layer;
      filter.mask = category_masks[go->layer];
      cpShapeSetFilter(shape, filter);
  */
}

void go_shape_moi(cpBody *body, cpShape *shape, struct gameobject *go) {
  float moment = cpBodyGetMoment(go->body);
  struct phys2d_shape *s = cpShapeGetUserData(shape);
  if (!s) {
    cpBodySetMoment(go->body, moment + 1);
    return;
  }

  moment += s->moi(s->data, go->mass);
  cpBodySetMoment(go->body, moment);
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

int MakeGameobject() {
  struct gameobject go = {
      .scale = 1.f,
      .bodytype = CP_BODY_TYPE_STATIC,
      .mass = 1.f,
      .next = -1,
      .sensor = 0,
      .shape_cbs = NULL,
  };

  go.cbs.begin.obj = JS_NULL;
  go.cbs.separate.obj = JS_NULL;

  go.body = cpSpaceAddBody(space, cpBodyNew(go.mass, 1.f));

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

void object_gui(struct gameobject *go) {
  /*
      float temp_pos[2];
      draw_cppoint(cpBodyGetPosition(go->body), 3);

      nuke_property_float2("Position", -1000000.f, temp_pos, 1000000.f, 1.f, 0.5f);

      cpVect tvect = { temp_pos[0], temp_pos[1] };
      cpBodySetPosition(go->body, tvect);

      float mtry = cpBodyGetAngle(go->body);
      float modtry = fmodf(mtry * RAD2DEGS, 360.f);
      if (modtry < 0.f)
        modtry += 360.f;

      float modtry2 = modtry;
      nuke_property_float("Angle", -1000.f, &modtry, 1000.f, 0.5f, 0.5f);
      modtry -= modtry2;
      cpBodySetAngle(go->body, mtry + (modtry * DEG2RADS));

      nuke_property_float("Scale", 0.f, &go->scale, 1000.f, 0.01f, go->scale * 0.01f);

      nuke_nel(3);
      nuke_radio_btn("Static", &go->bodytype, CP_BODY_TYPE_STATIC);
      nuke_radio_btn("Dynamic", &go->bodytype, CP_BODY_TYPE_DYNAMIC);
      nuke_radio_btn("Kinematic", &go->bodytype, CP_BODY_TYPE_KINEMATIC);

      cpBodySetType(go->body, go->bodytype);

      if (go->bodytype == CP_BODY_TYPE_DYNAMIC) {
           nuke_property_float("Mass", 0.01f, &go->mass, 1000.f, 0.01f, 0.01f);
          cpBodySetMass(go->body, go->mass);
      }

      nuke_property_float("Friction", 0.f, &go->f, 10.f, 0.01f, 0.01f);
      nuke_property_float("Elasticity", 0.f, &go->e, 2.f, 0.01f, 0.01f);

      int n = -1;



      for (int i = 0; i < arrlen(go->components); i++) {
          struct component *c = &go->components[i];

           comp_draw_debug(c);

       nuke_nel(5);
       if (nuke_btn("Del")) n = i;

       if (nuke_push_tree_id(c->ref->name, i)) {
              comp_draw_gui(c);
              nuke_tree_pop();
          }


      }

      if (n >= 0)
          gameobject_delcomponent(go, n);
  */
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
  draw_cppoint(pos, 3.f, color);
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

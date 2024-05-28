#include "2dphysics.h"

#include "gameobject.h"
#include "stb_ds.h"
#include "jsffi.h"

cpSpace *space = NULL;

static JSValue *fns = NULL;
static JSValue *hits = NULL;

void phys2d_init()
{
  space = cpSpaceNew();
}

void phys2d_update(float deltaT) {
  cpSpaceStep(space, deltaT);
  arrsetlen(fns,0);
  arrsetlen(hits,0);
}

JSValue arb2js(cpArbiter *arb)
{
  cpBody *body1;
  cpBody *body2;
  cpArbiterGetBodies(arb, &body1, &body2);
  
  cpShape *shape1;
  cpShape *shape2;
  cpArbiterGetShapes(arb, &shape1, &shape2);

  JSValue j = *(JSValue*)cpShapeGetUserData(shape2);

  JSValue jg = body2go(body2)->ref;

  HMM_Vec2 srfv;
  srfv.cp = cpArbiterGetSurfaceVelocity(arb);
  
  JSValue obj = JS_NewObject(js);
  JS_SetPropertyStr(js, obj, "normal", vec22js((HMM_Vec2)cpArbiterGetNormal(arb)));
  JS_SetPropertyStr(js, obj, "obj", JS_DupValue(js,jg));
  JS_SetPropertyStr(js, obj, "shape", JS_DupValue(js, j));
  JS_SetPropertyStr(js, obj, "point", vec22js((HMM_Vec2)cpArbiterGetPointA(arb, 0)));
  JS_SetPropertyStr(js, obj, "velocity", vec22js(srfv));
  JS_SetPropertyStr(js, obj, "impulse", vec22js((HMM_Vec2)cpArbiterTotalImpulse(arb)));
  JS_SetPropertyStr(js, obj, "ke", number2js(cpArbiterTotalKE(arb)));

  return obj;
}

void phys_run_post(cpSpace *space, JSValue *fn, JSValue *hit)
{
  script_call_sym(*fn, 1, hit);
  JS_FreeValue(js, *hit);
  JS_FreeValue(js, *fn);
}

void register_hit(cpArbiter *arb, gameobject *go, const char *name)
{
  if (JS_IsUndefined(go->ref)) return;
  JSValue cb = JS_GetPropertyStr(js, go->ref, name);
  if (!JS_IsUndefined(cb)) {
    JSValue jarb = arb2js(arb);
    arrput(fns, JS_DupValue(js,cb));
    arrput(hits, jarb);
    cpSpaceAddPostStepCallback(space, phys_run_post, fns+arrlen(fns)-1, hits+arrlen(hits)-1);
  }
  
  cpShape *s1, *s2;
  cpArbiterGetShapes(arb, &s1, &s2);
  JSValue j1 = *(JSValue*)cpShapeGetUserData(s1);
  JSValue j2 = *(JSValue*)cpShapeGetUserData(s2);
  cb = JS_GetPropertyStr(js, j1, name);
  if (!JS_IsUndefined(cb)) {
    JSValue jarb = arb2js(arb);
    arrput(fns, JS_DupValue(js,cb));
    arrput(hits, jarb);
    cpSpaceAddPostStepCallback(space, phys_run_post, fns+arrlen(fns)-1, hits+arrlen(hits)-1);
  }
}

int script_phys_cb_begin(cpArbiter *arb, cpSpace *space, gameobject *go) { register_hit(arb, go, "collide"); return 1; }
void script_phys_cb_separate(cpArbiter *arb, cpSpace *space, gameobject *go) { register_hit(arb, go, "separate"); }

void phys2d_setup_handlers(gameobject *go) {
  cpCollisionHandler *handler = cpSpaceAddWildcardHandler(space, (cpCollisionType)go);
  handler->userData = go;
  handler->beginFunc = script_phys_cb_begin;
  handler->separateFunc = script_phys_cb_separate;
}

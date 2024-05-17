#include "2dphysics.h"

#include "gameobject.h"
#include "stb_ds.h"
#include "jsffi.h"

cpSpace *space = NULL;

static JSValue fns[100];
static JSValue hits[100];
static int cb_idx = 0;

void phys2d_init()
{
  space = cpSpaceNew();
}

constraint *constraint_make(cpConstraint *c)
{
  constraint *cp = malloc(sizeof(*cp));
  cp->c = c;
  cp->break_cb = JS_UNDEFINED;
  cp->remove_cb = JS_UNDEFINED;
  cpSpaceAddConstraint(space,c);
  cpConstraintSetUserData(c, cp);
  return cp;
}

void constraint_break(constraint *constraint)
{
  if (!constraint->c) return;
  cpSpaceRemoveConstraint(space, constraint->c);
  cpConstraintFree(constraint->c);
  constraint->c = NULL;
  script_call_sym(constraint->break_cb,0,NULL);
}

void constraint_free(constraint *constraint)
{
  constraint_break(constraint);
  free(constraint);
}

void constraint_test(cpConstraint *constraint, float *dt)
{
  float max = cpConstraintGetMaxForce(constraint);
  if (!isfinite(max)) return;
  float force = cpConstraintGetImpulse(constraint)/ *dt;
  if (force > max)
    constraint_break(cpConstraintGetUserData(constraint));
}

void phys2d_update(float deltaT) {
  cpSpaceStep(space, deltaT);
  cpSpaceEachConstraint(space, constraint_test, &deltaT);
  cb_idx = 0;
}

JSValue arb2js(cpArbiter *arb)
{
  cpBody *body1;
  cpBody *body2;
  cpArbiterGetBodies(arb, &body1, &body2);
  
  cpShape *shape1;
  cpShape *shape2;
  cpArbiterGetShapes(arb, &shape1, &shape2);

  JSValue *j = cpShapeGetUserData(shape2);

  JSValue jg = body2go(body2)->ref;

  HMM_Vec2 srfv;
  srfv.cp = cpArbiterGetSurfaceVelocity(arb);
  
  JSValue obj = JS_NewObject(js);
  JS_SetPropertyStr(js, obj, "normal", vec22js((HMM_Vec2)cpArbiterGetNormal(arb)));
  JS_SetPropertyStr(js, obj, "obj", JS_DupValue(js,jg));
  JS_SetPropertyStr(js, obj, "shape", JS_DupValue(js, *j));
  JS_SetPropertyStr(js, obj, "point", vec22js((HMM_Vec2)cpArbiterGetPointA(arb, 0)));
  JS_SetPropertyStr(js, obj, "velocity", vec22js(srfv));

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
    fns[cb_idx] = JS_DupValue(js, cb);
    hits[cb_idx] = jarb;
    cpSpaceAddPostStepCallback(space, phys_run_post, &fns[cb_idx], &hits[cb_idx]);
    cb_idx++;
  }
  
  cpShape *s1, *s2;
  cpArbiterGetShapes(arb, &s1, &s2);
  JSValue *j1 = cpShapeGetUserData(s1);
  JSValue *j2 = cpShapeGetUserData(s2);
  cb = JS_GetPropertyStr(js, *j1, name);
  if (!JS_IsUndefined(cb)) {
    JSValue jarb = arb2js(arb);
    fns[cb_idx] = JS_DupValue(js,cb);
    hits[cb_idx] = jarb;
    cpSpaceAddPostStepCallback(space, phys_run_post, &fns[cb_idx], &hits[cb_idx]);
    cb_idx++;
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

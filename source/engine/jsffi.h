#ifndef FFI_H
#define FFI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "quickjs/quickjs.h"
#include "HandmadeMath.h"
#include <stdarg.h>
#include <chipmunk/chipmunk.h>

extern JSValue cpShape2js(cpShape *s);

#define MIST_CFUNC_DEF(name, length, func1) { name, JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE, JS_DEF_CFUNC, 0, .u = { .func = { length, JS_CFUNC_generic, { .generic = func1 } } } }

#define MIST_FUNC_DEF(TYPE, FN, LEN) MIST_CFUNC_DEF(#FN, LEN, js_##TYPE##_##FN)

#define JS_SETSIG JSContext *js, JSValue self, JSValue val

#define JSC_SCALL(NAME, FN) JSC_CCALL(NAME, \
  const char *str = js2str(argv[0]); \
  FN ; \
  JS_FreeCString(js,str); \
) \

#define JSC_SSCALL(NAME, FN) JSC_CCALL(NAME, \
  const char *str = js2str(argv[0]); \
  const char *str2 = js2str(argv[1]); \
  FN ; \
  JS_FreeCString(js,str2); \
  JS_FreeCString(js,str); \
) \

#define MIST_CGETSET_BASE(name, fgetter, fsetter, props) { name, props, JS_DEF_CGETSET, 0, .u = { .getset = { .get = { .getter = fgetter }, .set = { .setter = fsetter } } } }
#define MIST_CGETSET_DEF(name, fgetter, fsetter) MIST_CGETSET_BASE(name, fgetter, fsetter, JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE)
#define MIST_CGETET_HID(name, fgetter, fsetter) MIST_CGETSET_BASE(name, fgetter, fsetter, JS_PROP_CONFIGURABLE)
#define MIST_GET(name, fgetter) { #fgetter , JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE, JS_DEF_CGETSET, 0, .u = { .getset = { .get = { .getter = js_##name##_get_##fgetter } } } }

#define CGETSET_ADD_NAME(ID, ENTRY, NAME) MIST_CGETSET_DEF(#NAME, js_##ID##_get_##ENTRY, js_##ID##_set_##ENTRY)
#define CGETSET_ADD(ID, ENTRY) MIST_CGETSET_DEF(#ENTRY, js_##ID##_get_##ENTRY, js_##ID##_set_##ENTRY)
#define CGETSET_ADD_HID(ID, ENTRY) MIST_CGETSET_BASE(#ENTRY, js_##ID##_get_##ENTRY, js_##ID##_set_##ENTRY, JS_PROP_CONFIGURABLE)

#define JSC_CCALL(NAME, FN) JSValue js_##NAME (JSContext *js, JSValue self, int argc, JSValue *argv) { \
  JSValue ret = JS_UNDEFINED; \
  {FN;} \
  return ret; \
} \

#define GETSETPAIR(ID, ENTRY, TYPE, FN) \
JSValue js_##ID##_set_##ENTRY (JS_SETSIG) { \
  js2##ID (self)->ENTRY = js2##TYPE (val); \
  {FN;} \
  return JS_UNDEFINED; \
} \
\
JSValue js_##ID##_get_##ENTRY (JSContext *js, JSValue self) { \
  return TYPE##2js(js2##ID (self)->ENTRY); \
} \

#define JSC_GETSET(ID, ENTRY, TYPE) GETSETPAIR( ID , ENTRY , TYPE , ; )
#define JSC_GETSET_APPLY(ID, ENTRY, TYPE) GETSETPAIR(ID, ENTRY, TYPE, ID##_apply(js2##ID (self));)
#define JSC_GETSET_HOOK(ID, ENTRY) GETSETPAIR(ID, ENTRY, JSValue, ;)

#define JSC_GETSET_GLOBAL(ENTRY, TYPE) \
JSValue js_global_set_##ENTRY (JS_SETSIG) { \
  ENTRY = js2##TYPE (val); \
  return JS_UNDEFINED; \
} \
\
JSValue js_global_get_##ENTRY (JSContext *js, JSValue self) { \
  return TYPE##2js(ENTRY); \
} \

#define JSC_GETSET_BODY(ENTRY, CPENTRY, TYPE) \
JSValue js_gameobject_set_##ENTRY (JS_SETSIG) { \
  cpBody *b = js2gameobject(self)->body; \
  cpBodySet##CPENTRY (b, js2##TYPE (val)); \
  return JS_UNDEFINED; \
} \
\
JSValue js_gameobject_get_##ENTRY (JSContext *js, JSValue self) { \
  cpBody *b = js2gameobject(self)->body; \
  return TYPE##2js (cpBodyGet##CPENTRY (b)); \
} \

#define JSC_GET(ID, ENTRY, TYPE) \
JSValue js_##ID##_get_##ENTRY (JSContext *js, JSValue self) { \
  return TYPE##2js(js2##ID (self)->ENTRY); } \

#define QJSCLASS(TYPE)\
static JSClassID js_##TYPE##_id;\
static void js_##TYPE##_finalizer(JSRuntime *rt, JSValue val){\
TYPE *n = JS_GetOpaque(val, js_##TYPE##_id);\
YughSpam("Freeing " #TYPE " at %p", n); \
TYPE##_free(n);}\
static JSClassDef js_##TYPE##_class = {\
  #TYPE,\
  .finalizer = js_##TYPE##_finalizer,\
};\
TYPE *js2##TYPE (JSValue val) { return JS_GetOpaque(val,js_##TYPE##_id); }\
JSValue TYPE##2js(TYPE *n) { \
  JSValue j = JS_NewObjectClass(js,js_##TYPE##_id);\
  YughSpam("Created " #TYPE " at %p", n); \
  JS_SetOpaque(j,n);\
  return j; }\
\
static JSValue js_##TYPE##_memid (JSContext *js, JSValue self) { return str2js("%p", js2##TYPE(self)); } \

#define QJSGLOBALCLASS(NAME) \
JSValue NAME = JS_NewObject(js); \
JS_SetPropertyFunctionList(js, NAME, js_##NAME##_funcs, countof(js_##NAME##_funcs)); \
JS_SetPropertyStr(js, globalThis, #NAME, NAME); \

#define QJSCLASSPREP(TYPE) \
JS_NewClassID(&js_##TYPE##_id);\
JS_NewClass(JS_GetRuntime(js), js_##TYPE##_id, &js_##TYPE##_class);\

/* Defines a class and uses its function list as its prototype */
#define QJSCLASSPREP_FUNCS(TYPE) \
QJSCLASSPREP(TYPE); \
JSValue TYPE##_proto = JS_NewObject(js); \
JS_SetPropertyFunctionList(js, TYPE##_proto, js_##TYPE##_funcs, countof(js_##TYPE##_funcs)); \
JS_SetPropertyStr(js, TYPE##_proto, "memid", JS_NewCFunction(js, &js_##TYPE##_memid, "memid", 0)); \
JS_SetClassProto(js, js_##TYPE##_id, TYPE##_proto); \

#define countof(x) (sizeof(x)/sizeof((x)[0]))

void ffi_load();

JSValue vec22js(HMM_Vec2 v);
HMM_Vec2 js2vec2(JSValue v);

const char *js2str(JSValue v);
char *js2strdup(JSValue v);

JSValue bitmask2js(cpBitmask mask);
cpBitmask js2bitmask(JSValue v);
int js_print_exception(JSValue v);

struct rgba js2color(JSValue v);
double js2number(JSValue v);
JSValue number2js(double g);

uint64_t js2uint64(JSValue v);

JSValue str2js(const char *c, ...);

void nota_int(char *blob);

JSValue js_getpropidx(JSValue v, uint32_t i);
JSValue js_getpropstr(JSValue v, const char *str);
void jsfreestr(const char *str);
int js_arrlen(JSValue v);
void js_setpropstr(JSValue v, const char *str, JSValue p);

int js2boolean(JSValue v);
JSValue boolean2js(int b);

#define PREP_PARENT(TYPE, PARENT) \
TYPE##_proto = JS_NewObject(js); \
JS_SetPropertyFunctionList(js, TYPE##_proto, js_##TYPE##_funcs, countof(js_##TYPE##_funcs)); \
JS_SetPrototype(js, TYPE##_proto, PARENT##_proto); \

#ifdef __cplusplus
}
#endif

#endif

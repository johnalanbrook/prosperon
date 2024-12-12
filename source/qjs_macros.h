#include <tracy/TracyC.h>

#define MIST_CFUNC_DEF(name, length, func1, props) { name, props, JS_DEF_CFUNC, 0, .u = { .func = { length, JS_CFUNC_generic, { .generic = func1 } } } }

#define MIST_FUNC_DEF(TYPE, FN, LEN) MIST_CFUNC_DEF(#FN, LEN, js_##TYPE##_##FN, JS_PROP_C_W_E)
#define PROTO_FUNC_DEF(TYPE, FN, LEN) MIST_CFUNC_DEF(#FN, LEN, js_##TYPE##_##FN, 0)

#define JS_SETSIG JSContext *js, JSValue self, JSValue val
#define JSC_CCALL(NAME, ...) static JSValue js_##NAME (JSContext *js, JSValue self, int argc, JSValue *argv) { \
  JSValue ret = JS_UNDEFINED; \
  __VA_ARGS__  ;\
  return ret; \
}

#define JSC_SCALL(NAME, ...) JSC_CCALL(NAME, \
  const char *str = JS_ToCString(js,argv[0]); \
  __VA_ARGS__  ;\
  JS_FreeCString(js,str); \
)

#define JSC_SSCALL(NAME, ...) JSC_CCALL(NAME, \
  const char *str = JS_ToCString(js,argv[0]); \
  const char *str2 = JS_ToCString(js,argv[1]); \
  __VA_ARGS__ ; \
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

#define JSC_DCALL(FN) JSValue js_##FN (JSContext *js, JSValue self, int argc, JSValue *argv) { FN(); return JS_UNDEFINED; }

#define JSC_1CALL(FN) JSValue js_##FN (JSContext *js, JSValue self, int argc, JSValue *argv) { FN(js2number(js,argv[0])); return JS_UNDEFINED; }
#define JSC_2CALL(FN) JSValue js_##FN (JSContext *js, JSValue self, int argc, JSValue *argv) { FN(js2number(js,argv[0]), js2number(js,argv[1])); return JS_UNDEFINED; }
#define JSC_3CALL(FN) JSValue js_##FN (JSContext *js, JSValue self, int argc, JSValue *argv) { FN(js2number(js,argv[0]), js2number(js,argv[1]), js2number(js,argv[2])); return JS_UNDEFINED; }
#define JSC_4CALL(FN) JSValue js_##FN (JSContext *js, JSValue self, int argc, JSValue *argv) { FN(js2number(js,argv[0]), js2number(js,argv[1]), js2number(js,argv[2]), js2number(js,argv[3])); return JS_UNDEFINED; }
#define JSC_5CALL(FN) JSValue js_##FN (JSContext *js, JSValue self, int argc, JSValue *argv) { FN(js2number(js,argv[0]), js2number(js,argv[1]), js2number(js,argv[2]), js2number(js,argv[3]), js2number(js,argv[4])); return JS_UNDEFINED; }
#define JSC_6CALL(FN) JSValue js_##FN (JSContext *js, JSValue self, int argc, JSValue *argv) { FN(js2number(js,argv[0]), js2number(js,argv[1]), js2number(js,argv[2]), js2number(js,argv[3]), js2number(js,argv[4]), js2number(js,argv[5])); return JS_UNDEFINED; }
#define JSC_7CALL(FN) JSValue js_##FN (JSContext *js, JSValue self, int argc, JSValue *argv) { FN(js2number(js,argv[0]), js2number(js,argv[1]), js2number(js,argv[2]), js2number(js,argv[3]), js2number(js,argv[4]), js2number(js,argv[5]), js2number(js,argv[6])); return JS_UNDEFINED; }

#define GETSETPAIR(ID, ENTRY, TYPE, FN) \
JSValue js_##ID##_set_##ENTRY (JS_SETSIG) { \
  js2##ID (js, self)->ENTRY = js2##TYPE (js,val); \
  {FN;} \
  return JS_UNDEFINED; \
} \
\
JSValue js_##ID##_get_##ENTRY (JSContext *js, JSValue self) { \
  return TYPE##2js(js,js2##ID (js, self)->ENTRY); \
} \

#define JSC_GETSET(ID, ENTRY, TYPE) GETSETPAIR( ID , ENTRY , TYPE , ; )
#define JSC_GETSET_APPLY(ID, ENTRY, TYPE) GETSETPAIR(ID, ENTRY, TYPE, ID##_apply(js2##ID (js, self));)
#define JSC_GETSET_CALLBACK(ID, ENTRY) \
JSValue js_##ID##_set_##ENTRY (JS_SETSIG) { \
  JSValue fn = js2##ID (js, self)->ENTRY; \
  if (!JS_IsUndefined(fn)) JS_FreeValue(js, fn); \
  js2##ID (js, self)->ENTRY = JS_DupValue(js, val); \
  return JS_UNDEFINED; \
}\
JSValue js_##ID##_get_##ENTRY (JSContext *js, JSValue self) { return JS_DupValue(js, js2##ID (js, self)->ENTRY); } \

#define JSC_GETSET_GLOBAL(ENTRY, TYPE) \
JSValue js_global_set_##ENTRY (JS_SETSIG) { \
  ENTRY = js2##TYPE (val); \
  return JS_UNDEFINED; \
} \
\
JSValue js_global_get_##ENTRY (JSContext *js, JSValue self) { \
  return TYPE##2js(ENTRY); \
} \

#define JSC_GET(ID, ENTRY, TYPE) \
JSValue js_##ID##_get_##ENTRY (JSContext *js, JSValue self) { \
  return TYPE##2js(js,js2##ID (js, self)->ENTRY); } \

#define QJSCLASS(TYPE, ...)\
static JSClassID js_##TYPE##_id;\
static void js_##TYPE##_finalizer(JSRuntime *rt, JSValue val){\
TYPE *n = JS_GetOpaque(val, js_##TYPE##_id);\
TYPE##_free(rt,n);}\
static JSClassDef js_##TYPE##_class = {\
  #TYPE,\
  .finalizer = js_##TYPE##_finalizer,\
};\
TYPE *js2##TYPE (JSContext *js, JSValue val) { \
  if (JS_IsUndefined(val)) return NULL; \
  if (JS_GetClassID(val) != js_##TYPE##_id) return NULL; \
  return JS_GetOpaque(val,js_##TYPE##_id); \
}\
JSValue TYPE##2js(JSContext *js, TYPE *n) { \
  JSValue j = JS_NewObjectClass(js,js_##TYPE##_id);\
  JS_SetOpaque(j,n);\
  __VA_ARGS__ \
  return j; }\
\

#define QJSGLOBALCLASS(NAME) \
JSValue NAME = JS_NewObject(js); \
JS_SetPropertyFunctionList(js, NAME, js_##NAME##_funcs, countof(js_##NAME##_funcs)); \
JS_SetPropertyStr(js, globalThis, #NAME, NAME); \

/* Defines a class and uses its function list as its prototype */
#define QJSCLASSPREP_FUNCS(TYPE) \
JS_NewClassID(&js_##TYPE##_id);\
JS_NewClass(JS_GetRuntime(js), js_##TYPE##_id, &js_##TYPE##_class);\
JSValue TYPE##_proto = JS_NewObject(js); \
JS_SetPropertyFunctionList(js, TYPE##_proto, js_##TYPE##_funcs, countof(js_##TYPE##_funcs)); \
JS_SetClassProto(js, js_##TYPE##_id, TYPE##_proto); \

#define countof(x) (sizeof(x)/sizeof((x)[0]))

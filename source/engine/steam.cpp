#include "steam.h"
#include <steam/steam_api.h>

#define countof(x) (sizeof(x) / sizeof((x)[0]))

static JSValue js_steam_start(JSContext *js, JSValue this_v, int argc, JSValue *argv)
{
  SteamAPI_Init();
  return JS_UNDEFINED;
}

static const JSCFunctionListEntry js_steam_funcs[] = {
  JS_CFUNC_DEF("init", 0, js_steam_start ),
};

int js_steam_init(JSContext *js)
{
  JSValue proto = JS_NewObject(js);
  JS_SetPropertyFunctionList(js, proto, js_steam_funcs, countof(js_steam_funcs));
  JS_SetPropertyStr(js, JS_GetGlobalObject(js), "steam", proto);
  return 0;
}
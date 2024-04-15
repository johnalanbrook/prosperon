#include "steam.h"
/*
#include <steam/steam_api.h>
#include <steam/steam_api_flat.h>

static JSValue js_steam_init(JSContext *js, JSValue this_v, int argc, JSValue *argv)
{
  SteamAPI_Init();
  return JS_UNDEFINED;
}

static const JSCFunctionListEntry js_steam_funcs[] = {
  MIST_FUNC_DEF(steam, init, 1),
};

JSValue js_init_steam(JSContext *js)
{
  JSValue steam = JS_NewObject(js);
  JS_SetPropertyFunctionList(js, steam, js_steam_funcs, countof(js_steam_funcs));
  return steam;
}
*/
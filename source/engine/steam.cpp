#include "steam.h"
#ifndef NSTEAM
#include <steam/steam_api.h>
#include <steam/steam_api_flat.h>

#include "jsffi.h"

ISteamUserStats *stats = NULL;
ISteamApps *app = NULL;
ISteamRemoteStorage *remote = NULL;
ISteamUGC *ugc = NULL;

static JSValue js_steam_init(JSContext *js, JSValue this_v, int argc, JSValue *argv)
{
  SteamErrMsg err;
  SteamAPI_InitEx(&err);
  JSValue str = str2js(err);
  stats = SteamAPI_SteamUserStats();
  app = SteamAPI_SteamApps();
  remote = SteamAPI_SteamRemoteStorage();
  ugc = SteamAPI_SteamUGC();
  return str;
}

static const JSCFunctionListEntry js_steam_funcs[] = {
  MIST_FUNC_DEF(steam, init, 1),
};

JSC_SCALL(achievement_get32,
  int32 data;
  SteamAPI_ISteamUserStats_GetStatInt32(stats, str, &data);
  return number2js(data);
)

JSC_SCALL(achievement_get,
  bool data;
  SteamAPI_ISteamUserStats_GetAchievement(stats, str, &data);
  return boolean2js(data);
)

JSC_SCALL(achievement_set,
  return boolean2js(SteamAPI_ISteamUserStats_SetAchievement(stats, str));
)

JSC_CCALL(achievement_num,
  return number2js(SteamAPI_ISteamUserStats_GetNumAchievements(stats));
)

JSC_SCALL(achievement_clear, SteamAPI_ISteamUserStats_ClearAchievement(stats, str))

JSC_SCALL(achievement_user_get,
  bool a;
  boolean2js(SteamAPI_ISteamUserStats_GetUserAchievement(stats, js2uint64(argv[1]), str, &a));
  ret = boolean2js(a);
)

static const JSCFunctionListEntry js_achievement_funcs[] = {
  MIST_FUNC_DEF(achievement, clear, 1),
  MIST_FUNC_DEF(achievement, get32, 2),
  MIST_FUNC_DEF(achievement, get, 2),
  MIST_FUNC_DEF(achievement, set, 1),
  MIST_FUNC_DEF(achievement, num, 0),
  MIST_FUNC_DEF(achievement, user_get, 2),
};

JSC_CCALL(cloud_app_enabled,
  return boolean2js(SteamAPI_ISteamRemoteStorage_IsCloudEnabledForApp(remote))
)
JSC_CCALL(cloud_enable, SteamAPI_ISteamRemoteStorage_SetCloudEnabledForApp(remote, js2boolean(self)))
JSC_CCALL(cloud_account_enabled, return boolean2js(SteamAPI_ISteamRemoteStorage_IsCloudEnabledForAccount(remote)))

static const JSCFunctionListEntry js_cloud_funcs[] = {
  MIST_FUNC_DEF(cloud, app_enabled, 0),
  MIST_FUNC_DEF(cloud, account_enabled, 0),
  MIST_FUNC_DEF(cloud, enable, 1)
};

JSC_CCALL(app_owner,
  uint64_t own = SteamAPI_ISteamApps_GetAppOwner(app);
  return JS_NewBigUint64(js, own);
)
static const JSCFunctionListEntry js_app_funcs[] = {
  MIST_FUNC_DEF(app, owner, 0),
};

JSValue js_init_steam(JSContext *js)
{
  JSValue steam = JS_NewObject(js);
  JS_SetPropertyFunctionList(js, steam, js_steam_funcs, countof(js_steam_funcs));
  JSValue achievements = JS_NewObject(js);
  JS_SetPropertyFunctionList(js, achievements, js_achievement_funcs, countof(js_achievement_funcs));
  js_setpropstr(steam, "achievements", achievements);

  JSValue app = JS_NewObject(js);
  JS_SetPropertyFunctionList(js, app, js_app_funcs, countof(js_app_funcs));
  js_setpropstr(steam, "app", app);

  JSValue cloud = JS_NewObject(js);
  JS_SetPropertyFunctionList(js, cloud, js_cloud_funcs, countof(js_cloud_funcs));
  js_setpropstr(steam, "cloud", cloud);
  return steam;
}
#endif

#ifndef QJS_STEAM_H
#define QJS_STEAM_H

#include "jsffi.h"

#ifdef __cplusplus
extern "C" int js_steam_init(JSContext *js);
#else
int js_steam_init(JSContext *js);
#endif

#endif
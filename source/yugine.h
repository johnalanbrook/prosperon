#ifndef YUGINE_H
#define YUGINE_H

#include "script.h"

void engine_start(JSContext *js, JSValue start_fn, JSValue proc_fn, float x, float y); /* fn runs after the engine starts */

#endif

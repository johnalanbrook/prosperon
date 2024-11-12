#ifndef SCRIPT_H
#define SCRIPT_H

#include "quickjs.h"

void script_startup();
void script_stop();

void script_evalf(const char *format, ...);
JSValue script_eval(const char *file, const char *script);
void script_call_sym(JSValue sym, int argc, JSValue *argv);

#endif

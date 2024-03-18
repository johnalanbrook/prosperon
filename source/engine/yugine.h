#ifndef YUGINE_H
#define YUGINE_H

#include "script.h"

double apptime();
void print_stacktrace();
void engine_start(JSValue start_fn, JSValue proc_fn); /* fn runs after the engine starts */

void quit();


#endif

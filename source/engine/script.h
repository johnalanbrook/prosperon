#ifndef SCRIPT_H
#define SCRIPT_H

struct s7_scheme;
extern struct s7_scheme *s7;

void script_init();
void script_run(const char *script);

#endif

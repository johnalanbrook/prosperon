#ifndef SCRIPT_H
#define SCRIPT_H

void script_init();
void script_run(const char *script);
void script_dofile(const char *file);
void script_update();
void script_draw();
void script_editor();
void script_call(const char *f);

#endif

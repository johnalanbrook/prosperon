#ifndef ED_PROJECT_H
#define ED_PROJECT_H

#include "config.h"

struct gameproject {
    char name[127];
    char path[MAXPATH];
};

void editor_init_project(struct gameproject *gp);
void editor_save_projects();
void editor_load_projects();
void editor_proj_select_gui();
void editor_import_project(char *path);
void editor_make_project(char *path);

#endif

#ifndef EDITORSTATE_H
#define EDITORSTATE_H

void set_new_model(char *modelPath);

extern void (*asset_command)(char *asset);

void print_file(char *file);

#endif

#ifndef RESOURCES_H
#define RESOURCES_H

#include <stdio.h>
#include "stb_ds.h"
#include "string.h"
#include <time.h>

extern char *DATA_PATH;
extern int LOADED_GAME;

void resources_init();
char *get_filename_from_path(char *path, int extension);
char *dirname(const char *path);
char *makepath(char *dir, char *file);
char *str_replace_ext(const char *s, const char *newext);
FILE *res_open(char *path, const char *tag);
char **ls(const char *path);
int cp(const char *p1, const char *p2);
int fexists(const char *path);
time_t file_mod_secs(const char *file);
void pack_start(const char *name);
void pack_add(const char *path);
void pack_end();

void *slurp_file(const char *filename, size_t *size);
char *slurp_text(const char *filename, size_t *size);
int slurp_write(void *txt, const char *filename, size_t len);

char *seprint(char *fmt, ...);

#endif

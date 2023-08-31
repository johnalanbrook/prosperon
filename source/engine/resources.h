#ifndef RESOURCES_H
#define RESOURCES_H

#include <stdio.h>

extern char *DATA_PATH;
extern char *PREF_PATH;

extern const char *EXT_PREFAB;
extern const char *EXT_LEVEL;
extern const char *EXT_ASSET;
extern int stemlen;

void resources_init();

extern char **prefabs;
void findPrefabs();
void fill_extensions(char *paths, const char *path, const char *ext);
char *get_filename_from_path(char *path, int extension);
char *get_directory_from_path(char *path);
char *str_replace_ext(const char *s, const char *newext);
FILE *res_open(char *path, const char *tag);
FILE *path_open(const char *tag, const char *fmt, ...);
char *make_path(const char *file);

char *strdup(const char *s);

#endif

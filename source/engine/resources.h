#ifndef RESOURCES_H
#define RESOURCES_H



#include <stdio.h>

struct vec;

extern char *DATA_PATH;
extern char *PREF_PATH;

extern const char *EXT_PREFAB;
extern const char *EXT_LEVEL;
extern int stemlen;

void resources_init();

extern struct vec *prefabs;
void findPrefabs();
void fill_extensions(struct vec *vec, char *path, const char *ext);
char *get_filename_from_path(char *path, int extension);
char *get_directory_from_path(char *path);
FILE *res_open(char *path, const char *tag);
FILE *path_open(const char *fmt, const char *tag, ...);

#endif

#ifndef RESOURCES_H
#define RESOURCES_H

#include <stdio.h>

extern char *DATA_PATH;

void resources_init();
void fill_extensions(char *paths, const char *path, const char *ext);
char *get_filename_from_path(char *path, int extension);
char *get_directory_from_path(char *path);
char *str_replace_ext(const char *s, const char *newext);
FILE *res_open(char *path, const char *tag);
FILE *path_open(const char *tag, const char *fmt, ...);
char *make_path(const char *file);
char **ls(char *path);
int cp(char *p1, char *p2);
int fexists(char *path);
FILE *fopen_mkdir(char *path, char *mode);

void *slurp_file(const char *filename, size_t *size);
char *slurp_text(const char *filename, size_t *size);
int slurp_write(const char *txt, const char *filename);

char *seprint(char *fmt, ...);

void pack_engine(const char *fname);

#endif

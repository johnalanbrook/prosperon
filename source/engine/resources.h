#ifndef RESOURCES_H
#define RESOURCES_H

#include <stdio.h>

extern int stemlen;

void resources_init();
void fill_extensions(char *paths, const char *path, const char *ext);
char *get_filename_from_path(char *path, int extension);
char *get_directory_from_path(char *path);
char *str_replace_ext(const char *s, const char *newext);
FILE *res_open(char *path, const char *tag);
FILE *path_open(const char *tag, const char *fmt, ...);
char *make_path(const char *file);

char *strdup(const char *s);

unsigned char *slurp_file(const char *filename, long *size);
char *slurp_text(const char *filename);
int slurp_write(const char *txt, const char *filename);

void pack_engine();

#endif

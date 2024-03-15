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
char *get_directory_from_path(char *path);
char *str_replace_ext(const char *s, const char *newext);
FILE *res_open(char *path, const char *tag);
FILE *path_open(const char *tag, const char *fmt, ...);
char **ls(const char *path);
int cp(const char *p1, const char *p2);
int fexists(const char *path);
time_t file_mod_secs(const char *file);

char *dirname(const char *path);

void *slurp_file(const char *filename, size_t *size);
char *slurp_text(const char *filename, size_t *size);
int slurp_write(const char *txt, const char *filename, size_t len);

char *seprint(char *fmt, ...);

void pack_engine(const char *fname);

static inline void *stbarrdup(void *mem, size_t size, int len) {
  void *out = NULL;
  arrsetlen(out, len);
  memcpy(out,mem,size*len);
  return out;
}

#define arrconcat(a,b) do{for (int i = 0; i < arrlen(b); i++) arrput(a,b[i]);}while(0)
#define arrdup(a) (stbarrdup(a, sizeof(*a), arrlen(a)))

#endif

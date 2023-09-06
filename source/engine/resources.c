#include "resources.h"

#include "config.h"
#include "log.h"
#include <dirent.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "font.h"

#include <fcntl.h>
#include "cdb.h"

#ifndef __EMSCRIPTEN__
#include <ftw.h>
#endif

#include "stb_ds.h"

char *DATA_PATH = NULL;
char *PREF_PATH = NULL;

char **prefabs;

int stemlen = 0;

static const char *cur_ext = NULL;
struct dirent *c_dirent = NULL;

char pathbuf[MAXPATH + 1];

const char *DB_NAME = "test.db";

static struct cdb game_cdb;
static int loaded_cdb = 0;

void resources_init() {
  DATA_PATH = malloc(MAXPATH);
  getcwd(DATA_PATH, MAXPATH);
  strncat(DATA_PATH, "/", MAXPATH);

  if (!PREF_PATH)
    PREF_PATH = strdup("./tmp/");

  int fd;
  fd = open("test.cdb", O_RDONLY);
  cdb_init(&game_cdb, fd);
  loaded_cdb = 1;
}

char *get_filename_from_path(char *path, int extension) {
  char *dirpos = strrchr(path, '/');
  if (!dirpos)
    dirpos = path;

  char *end = strrchr(path, '\0');

  int offset = 0;
  if (!extension) {
    char *ext = strrchr(path, '.');
    offset = end - ext;
    YughInfo("Making %s without extension ...");
  }

  char *filename = malloc(sizeof(char) * (end - dirpos - offset + 1));
  strncpy(filename, dirpos, end - dirpos - offset);
  return filename;
}

char *get_directory_from_path(char *path) {
  const char *dirpos = strrchr(path, '/');
  char *directory = (char *)malloc(sizeof(char) * (dirpos - path + 1));
  strncpy(directory, path, dirpos - path);
  return directory;
}

FILE *res_open(char *path, const char *tag) {
  strncpy(pathbuf, DATA_PATH, MAXPATH);
  strncat(pathbuf, path, MAXPATH);
  FILE *f = fopen(pathbuf, tag);
  return f;
}

static char *ext_paths = NULL;

#ifndef __EMSCRIPTEN__

static int ext_check(const char *path, const struct stat *sb, int typeflag) {
  if (typeflag == FTW_F) {
    const char *ext = strrchr(path, '.');
    if (ext != NULL && !strcmp(ext, cur_ext)) {
      char newstr[255];
      strncpy(newstr, path, 255);
      arrput(prefabs, newstr);      
    }
  }

  return 0;
}

void fill_extensions(char *paths, const char *path, const char *ext) {
  cur_ext = ext;
  arrfree(paths);
  ext_paths = paths;
  ftw(".", ext_check, 10);
}
#else
void fill_extensions(char *paths, const char *path, const char *ext)
{};
#endif

char *str_replace_ext(const char *s, const char *newext) {
  static char ret[256];

  strncpy(ret, s, 256);
  char *ext = strrchr(ret, '.');
  strncpy(ext, newext, 10);

  return ret;
}

FILE *path_open(const char *tag, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vsprintf(pathbuf, fmt, args);
  va_end(args);

  FILE *f = fopen(pathbuf, tag);
  return f;
}

char *make_path(const char *file) {
  strncpy(pathbuf, DATA_PATH, MAXPATH);
  strncat(pathbuf, file, MAXPATH);
  return pathbuf;
}

char *strdup(const char *s) {
  char *new = malloc(sizeof(char) * (strlen(s) + 1));
  strcpy(new, s);
  return new;
}

unsigned char *slurp_file(const char *filename, size_t *size)
{
  if (cdb_find(&game_cdb, filename, strlen(filename))) {
    unsigned vlen, vpos;
    vpos = cdb_datapos(&game_cdb);
    vlen = cdb_datalen(&game_cdb);
    char *data = malloc(vlen);
    cdb_read(&game_cdb, data, vlen, vpos);
    if (size) *size = vlen;
    return strdup(data);
  }
  
  FILE *f;

  jump:
  f = fopen(filename, "rb");

  if (!f) return NULL;

  fseek(f, 0, SEEK_END);
  size_t fsize = ftell(f);
  rewind(f);
  void *slurp = malloc(fsize);
  fread(slurp, fsize, 1, f);
  fclose(f);

  if (size) *size = fsize;

  return slurp;
}

char *slurp_text(const char *filename)
{
  size_t len;
  char *str = slurp_file(filename, &len);
  char *retstr = malloc(len+1);
  memcpy(retstr, str, len);
  retstr[len] = 0;
  free(str);
  return retstr;
}

int slurp_write(const char *txt, const char *filename) {
  FILE *f = fopen(filename, "w");
  if (!f) return 1;

  fputs(txt, f);
  fclose(f);
  return 0;
}

#ifndef __EMSCRIPTEN__
static struct cdb_make cdbm;

static const char *pack_ext[] = {".qoi", ".qoa", ".js", ".wav", ".mp3", ".png", ".sf2", ".midi", ".lvl", ".glsl"};

static int ftw_pack(const char *path, const struct stat *sb, int flag)
{
  if (flag != FTW_F) return 0;

  int pack = 0;

  char *ext = strrchr(path, '.');

  if (!ext)
    return 0;

  for (int i = 0; i < 6; i++) {
    if (!strcmp(ext, pack_ext[i])) {
      pack = 1;
      break;
    }
  }

  if (!pack) return 0;

  size_t len;
  void *file = slurp_file(path, &len);
  cdb_make_add(&cdbm, &path[2], strlen(&path[2]), file, len);
    
  free(file);

  return 0;
}

void pack_engine(const char *fname)
{
  int fd;
  char *key, *va;
  unsigned klen, vlen;
  fd = open(fname, O_RDWR|O_CREAT);
  cdb_make_start(&cdbm, fd);
  ftw(".", ftw_pack, 20);
  cdb_make_finish(&cdbm);
}
#else
void pack_engine(const char *fname){
  YughError("Cannot pack engine on a web build.");
}
#endif

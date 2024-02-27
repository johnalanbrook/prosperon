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
#include <errno.h>
#include <unistd.h>
#include "font.h"

#include <fcntl.h>
#include "cdb.h"

#ifndef __EMSCRIPTEN__
#include <ftw.h>
#endif

#include "stb_ds.h"

#include "core.cdb.h"

char **prefabs;

static const char *cur_ext = NULL;
struct dirent *c_dirent = NULL;

char pathbuf[MAXPATH + 1];

static struct cdb corecdb;
static struct cdb game_cdb;

void resources_init() {
  int fd = open("test.cdb", O_RDONLY);
  cdb_init(&game_cdb, fd);
  cdb_initf(&corecdb, core_cdb, core_cdb_len);
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

char *dirname(const char *path)
{
  const char *dirpos = strrchr(path, '/');
  if (!dirpos) return ".";
  char *dir = malloc(dirpos-path+1);
  strncpy(dir,path,dirpos-path);
  return dir;
}

char *seprint(char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  char test[128];
  int len = vsnprintf(test, 128, fmt, args);
  if (len > 128) {
    char *test = malloc(len+1);
    vsnprintf(test, len+1, fmt, args);
    return strdup(test);
  }
  
  return strdup(test);
}

static char *ext_paths = NULL;

#ifndef __EMSCRIPTEN__
static char **ls_paths = NULL;

static int ls_ftw(const char *path, const struct stat *sb, int typeflag)
{
  if (typeflag == FTW_F && strlen(path) > 2)
    arrpush(ls_paths, strdup(&path[2]));

  return 0;
}

// TODO: Not reentrant
char **ls(const char *path)
{
  if (ls_paths) {
    for (int i = 0; i < arrlen(ls_paths); i++)
      free(ls_paths[i]);

    arrfree(ls_paths);
  }
  ftw(".", ls_ftw, 10);
  return ls_paths;
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
  vsnprintf(pathbuf, MAXPATH+1, fmt, args);
  va_end(args);

  FILE *f = fopen(pathbuf, tag);
  return f;
}

void *cdb_slurp(struct cdb *cdb, const char *file, size_t *size)
{
    unsigned vlen, vpos;
    vpos = cdb_datapos(cdb);
    vlen = cdb_datalen(cdb);
    char *data = malloc(vlen+1);
    cdb_read(cdb, data, vlen, vpos);
    if (size) *size = vlen;
    return data;
}

int fexists(const char *path)
{
  return !access(path,R_OK);
  
  int len = strlen(path);
  if (cdb_find(&game_cdb, path,len)) return 1;
  else if (cdb_find(&corecdb, path, len)) return 1;
  else if (!access(path, R_OK)) return 1;

  return 0;
}

void *os_slurp(const char *file, size_t *size)
{
  FILE *f;

  jump:
  f = fopen(file, "rb");

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

void *slurp_file(const char *filename, size_t *size)
{
  if (!access(filename, R_OK))
    return os_slurp(filename, size);
  else if (cdb_find(&game_cdb, filename, strlen(filename)))
    return cdb_slurp(&game_cdb, filename, size);
  else if (cdb_find(&corecdb, filename, strlen(filename)))
    return cdb_slurp(&corecdb, filename, size);

  return NULL;
}

char *slurp_text(const char *filename, size_t *size)
{
  size_t len;
  unsigned char *str = slurp_file(filename, &len);
  if (!str) return NULL;
  char *retstr = malloc(len+1);
  memcpy(retstr, str, len);
  retstr[len] = '\0';
  free(str);
  if (size) *size = len;
  return retstr;
}

int cp(const char *p1, const char *p2)
{
  size_t len;
  void *data = slurp_file(p1, &len);

  FILE *f = fopen_mkdir(p2, "w");
  if (!f) return 1;
  fwrite(data, len, 1, f);
  free(data);
  fclose(f);
  return 0;
}

void rek_mkdir(char *path) {
    char *sep = strrchr(path, '/');
    if(sep != NULL) {
        *sep = 0;
        rek_mkdir(path);
        *sep = '/';
    }
#if defined __WIN32    
    if(mkdir(path) && errno != EEXIST)
#else
    if (mkdir(path, 0777) && errno != EEXIST)
#endif    
        printf("error while trying to create '%s'\n%m\n", path); 
}

FILE *fopen_mkdir(const char *path, const char *mode) {
    char *sep = strrchr(path, '/');
    if(sep) { 
        char *path0 = strdup(path);
        path0[ sep - path ] = 0;
        rek_mkdir(path0);
        free(path0);
    }
    return fopen(path,mode);
}

int slurp_write(const char *txt, const char *filename, size_t len) {
  FILE *f = fopen_mkdir(filename, "w");
  if (!f) return 1;

  if (len < 0) len = strlen(txt);
  fwrite(txt, len, 1, f);
  fclose(f);
  return 0;
}

#ifndef __EMSCRIPTEN__
static struct cdb_make cdbm;

static const char *pack_ext[] = {".qoi", ".qoa", ".js", ".wav", ".mp3", ".png", ".sf2", ".midi", ".lvl", ".glsl", ".ttf"};

static int ftw_pack(const char *path, const struct stat *sb, int flag)
{
  if (flag != FTW_F) return 0;
  int pack = 0;
  char *ext = strrchr(path, '.');

  if (!ext)
    return 0;

  for (int i = 0; i < 11; i++) {
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

char **ls(char *path) { return NULL; }
#endif

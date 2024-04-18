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
#include "miniz.h"

#ifndef __EMSCRIPTEN__
#include <ftw.h>
#endif

#define SOKOL_FETCH_IMPL
#include "sokol/sokol_fetch.h"

#include "stb_ds.h"

#include "core.cdb.h"

#if defined(_WIN32)
#include <direct.h>
#define mkdir(x,y) _mkdir(x)
#endif

char **prefabs;

static const char *cur_ext = NULL;
struct dirent *c_dirent = NULL;

char pathbuf[MAXPATH + 1];

static mz_zip_archive corecdb;
static mz_zip_archive game_cdb;

int LOADED_GAME = 0;
uint8_t *gamebuf;
void *zipbuf;

sfetch_handle_t game_h;

static void response_cb(const sfetch_response_t *r)
{
  if (r->fetched) {
    zipbuf = malloc(r->data.size);
    memcpy(zipbuf, r->data.ptr, r->data.size);
    mz_zip_reader_init_mem(&game_cdb, zipbuf, r->data.size,0);
    
  }
  if (r->finished) {
    LOADED_GAME = 1;
    void *buf = sfetch_unbind_buffer(r->handle);
    free(buf);
    if (r->failed)
      LOADED_GAME = -1;
  }
}

void *gamedata;

void resources_init() {
  sfetch_setup(&(sfetch_desc_t){
    .max_requests = 1024,
    .num_channels = 4,
    .num_lanes = 8,
    .logger = { .func = sg_logging },
  });
  mz_zip_reader_init_mem(&corecdb, core_cdb, core_cdb_len, 0);  
  
#ifdef __EMSCRIPTEN__
  gamebuf = malloc(8*1024*1024);
  game_h = sfetch_send(&(sfetch_request_t){
    .path="game.zip",
    .callback = response_cb,
    .buffer = {
      .ptr = gamebuf,
      .size = 8*1024*1024
    }
  });
#else
  size_t gamesize;
  gamebuf = slurp_file("game.zip", &gamesize);
  if (gamebuf) {
    mz_zip_reader_init_mem(&game_cdb, gamebuf, gamesize, 0);
    free(gamebuf);
    return;
  }
#endif
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

time_t file_mod_secs(const char *file) {
  struct stat attr;
  stat(file, &attr);
  return attr.st_mtime;
}

// TODO: Not reentrant
char **ls(const char *path)
{
  if (ls_paths) {
    for (int i = 0; i < arrlen(ls_paths); i++)
      free(ls_paths[i]);

    arrfree(ls_paths);
  }
  
  ftw(path, ls_ftw, 10);
  return ls_paths;
}

static mz_zip_archive ar;

void pack_start(const char *name)
{
  memset(&ar, 0, sizeof(ar));
  int status = mz_zip_writer_init_file(&ar, name, 0);

}

void pack_add(const char *path) 
{
  mz_zip_writer_add_file(&ar, path, path, NULL, 0, MZ_BEST_COMPRESSION);
}

void pack_end()
{
  mz_zip_writer_finalize_archive(&ar);
  mz_zip_writer_end(&ar);
}

#else
void pack_start(const char *name){}
void pack_add(const char *path){}
void pack_end() {}
void fill_extensions(char *paths, const char *path, const char *ext)
{};
char **ls(const char *path) { return NULL; }
void pack(const char *name, const char *dir) {}
#endif

char *str_replace_ext(const char *s, const char *newext) {
  static char ret[256];

  strncpy(ret, s, 256);
  char *ext = strrchr(ret, '.');
  strncpy(ext, newext, 10);

  return ret;
}

int fexists(const char *path)
{
  int len = strlen(path);
  if (mz_zip_reader_locate_file(&game_cdb, path, NULL, 0) != -1) return 1;
  else if (mz_zip_reader_locate_file(&corecdb, path, NULL, 0) != -1) return 1;
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
  void *ret;
  if (!access(filename, R_OK))
    return os_slurp(filename, size);
  else if (ret = mz_zip_reader_extract_file_to_heap(&game_cdb, filename, size, 0))
    return ret;
  else if (ret = mz_zip_reader_extract_file_to_heap(&corecdb, filename, size, 0))
    return ret;

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

  FILE *f = fopen(p2, "w");
  if (!f) return 1;
  fwrite(data, len, 1, f);
  free(data);
  fclose(f);
  return 0;
}

int mkpath(char *path, mode_t mode)
{
    char tmp[256];
    char *p = NULL;
    size_t len;
    struct stat sb;

    strncpy(tmp, path, sizeof(tmp));
    len = strlen(tmp);
    if (len > 0 && tmp[len - 1] == '/')
        tmp[len - 1] = 0;

    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            if (stat(tmp, &sb) != 0) {
                if (errno != ENOENT || mkdir(tmp, mode) != 0) {
                    return -1;
                }
            } else if (!S_ISDIR(sb.st_mode)) {
                errno = ENOTDIR;
                return -1;
            }
            *p = '/';
        }
    }

    if (stat(tmp, &sb) != 0) {
        if (errno != ENOENT || mkdir(tmp, mode) != 0) {
            return -1;
        }
    } else if (!S_ISDIR(sb.st_mode)) {
        errno = ENOTDIR;
        return -1;
    }

    return 0;
}

int slurp_write(const char *txt, const char *filename, size_t len) {
  FILE *f = fopen(filename, "w");
  if (!f) return 1;

  if (len < 0) len = strlen(txt);
  fwrite(txt, len, 1, f);
  fclose(f);
  return 0;
}

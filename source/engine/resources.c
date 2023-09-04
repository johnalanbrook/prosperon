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

#include <sqlite3.h>

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
static sqlite3 *game_db = NULL;
static sqlite3_stmt *fopen_stmt;

#define sqlite_perr(db) YughError("Database error code %d, %s: %s", sqlite3_errcode(db), sqlite3_errstr(sqlite3_errcode(db)), sqlite3_errmsg(db));

void resources_init() {
  DATA_PATH = malloc(MAXPATH);
  getcwd(DATA_PATH, MAXPATH);
  strncat(DATA_PATH, "/", MAXPATH);

  if (!PREF_PATH)
    PREF_PATH = strdup("./tmp/");

  if (sqlite3_open_v2("test.db", &game_db, SQLITE_OPEN_READONLY, NULL)) {
    sqlite_perr(game_db);
    sqlite3_close(game_db);
    game_db = NULL;
    return;
  }

  if (sqlite3_prepare_v2(game_db, "select data from files where path=?1", -1, &fopen_stmt, NULL)) {
    sqlite_perr(game_db);
  }
  
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

unsigned char *slurp_file(const char *filename, long *size)
{
  if (game_db) {
    sqlite3_reset(fopen_stmt);
    
    if (sqlite3_bind_text(fopen_stmt, 1, filename, -1, NULL)) {
      sqlite_perr(game_db);
      goto jump;
    }

    if (sqlite3_step(fopen_stmt) == SQLITE_ERROR) {
      sqlite_perr(game_db);
      goto jump;
    }

    char *data = sqlite3_column_blob(fopen_stmt,0);
    if (!data)
      goto jump;
    
    return strdup(data);
  }
  FILE *f;

  jump:
  f = fopen(filename, "rb");

  if (!f) return NULL;

  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  fseek(f, 0, SEEK_SET);
  unsigned char *slurp = malloc(fsize + 1);
  fread(slurp, fsize, 1, f);
  fclose(f);

  if (size) *size = fsize;

  return slurp;
}

char *slurp_text(const char *filename) {
  if (game_db) {
    if (sqlite3_reset(fopen_stmt)) {
      sqlite_perr(game_db);
      goto jump;
    }
    
    if (sqlite3_bind_text(fopen_stmt, 1, filename, -1, NULL)) {
      sqlite_perr(game_db);
      goto jump;
    }

    if (sqlite3_step(fopen_stmt) == SQLITE_ERROR) {
      sqlite_perr(game_db);
      goto jump;
    }

    char *data = sqlite3_column_text(fopen_stmt,0);
    if (!data)
      goto jump;
    
    return strdup(data);
  }
  
  FILE *f;
  char *buf;

  jump:
  f = fopen(filename, "r");
  
  if (!f) {
    YughWarn("File %s doesn't exist.", filename);
    return NULL;
  }

  long int fsize;
  fseek(f, 0, SEEK_END);
  fsize = ftell(f);
  buf = malloc(fsize + 1);
  rewind(f);
  size_t r = fread(buf, sizeof(char), fsize, f);
  buf[r] = '\0';

  fclose(f);

  return buf;
}

int slurp_write(const char *txt, const char *filename) {
  FILE *f = fopen(filename, "w");
  if (!f) return 1;

  fputs(txt, f);
  fclose(f);
  return 0;
}

#ifndef __EMSCRIPTEN__
static sqlite3 *pack_db = NULL;
static sqlite3_stmt *pack_stmt;

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

  long len;
  void *file = slurp_file(path, &len);
  if (sqlite3_bind_text(pack_stmt, 1, &path[2], -1, NULL))
    sqlite_perr(pack_db);
    
  if (sqlite3_bind_blob(pack_stmt, 2, file, len, NULL))
    sqlite_perr(pack_db);
    
  if (sqlite3_step(pack_stmt) != SQLITE_DONE)
    sqlite_perr(pack_db);
    
  free(file);

  if (sqlite3_reset(pack_stmt))
    sqlite_perr(pack_db);

  return 0;
}

void pack_engine()
{
  sqlite3 *db;
  char *zErr = 0;
  if (sqlite3_open("test.db", &db)) {
    sqlite_perr(db);
    sqlite3_close(db);
    return;
  }

  if(sqlite3_exec(db, "create table files ( path text, data blob);", NULL, NULL, NULL))
    sqlite_perr(db);

  pack_db = db;
  if(sqlite3_prepare_v2(db, "insert into files (path, data) values (?1, ?2)", -1, &pack_stmt, NULL)) {
    sqlite_perr(db);
    sqlite3_close(db);
    return;
  }
  ftw(".", ftw_pack, 20);

  sqlite3_close(db);
}
#else
void packengine(){
  YughError("Cannot pack engine on a web build.");
}
#endif

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

#ifndef __EMSCRIPTEN__
#include <ftw.h>
#endif

#include "sokol/sokol_fetch.h"
#include "stb_ds.h"

#if defined(_WIN32)
#include <direct.h>
#define mkdir(x,y) _mkdir(x)
#endif

char **prefabs;

static const char *cur_ext = NULL;
struct dirent *c_dirent = NULL;

char pathbuf[MAXPATH + 1];

void *os_slurp(const char *file, size_t *size)
{
  FILE *f;

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

#if defined(__linux__)
	#include <unistd.h>
	#include <stdio.h>
#elif defined(__APPLE__)
	#include <mach-o/dyld.h>
#elif defined(_WIN32)
	#include <windows.h>
#endif

int get_executable_path(char *buffer, unsigned int buffer_size) {
	#if defined(__linux__)
		ssize_t len = readlink("/proc/self/exe", buffer, buffer_size - 1);
		if (len == -1) {
			return 0;
		}
		buffer[len] = '\0';
		return len;
	#elif defined(__APPLE__)
		if (_NSGetExecutablePath(buffer, &buffer_size) == 0) {
			return buffer_size;
		}
	#elif defined(_WIN32)
		return GetModuleFileName(NULL, buffer, buffer_size);
	#endif

	return 0;
}
void *gamedata;

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
  
  if (!stat(file,&attr))
    return attr.st_mtime;
    
  return -1;
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

#else
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

void *slurp_file(const char *filename, size_t *size)
{
  if (!access(filename, R_OK))
    return os_slurp(filename, size);
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

int slurp_write(void *txt, const char *filename, size_t len) {
  FILE *f = fopen(filename, "w");
  if (!f) return 1;

  if (len < 0) len = strlen(txt);
  fwrite(txt, len, 1, f);
  fclose(f);
  return 0;
}

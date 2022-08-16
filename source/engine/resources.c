#include "resources.h"

#include "config.h"
#include <dirent.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "vec.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define __USE_XOPEN_EXTENDED 1
#include <ftw.h>

char *DATA_PATH = NULL;
char *PREF_PATH = NULL;

struct vec *prefabs = NULL;

const char *EXT_PREFAB = ".prefab";
const char *EXT_LEVEL = ".level";
int stemlen = 0;

static const char *cur_ext = NULL;
struct dirent *c_dirent = NULL;
struct vec *c_vec = NULL;

char pathbuf[MAXPATH];


void resources_init()
{
    DATA_PATH = malloc(256);
    getcwd(DATA_PATH, 256);
    strncat(DATA_PATH, "/", 256);

    if (!PREF_PATH)
        PREF_PATH = strdup("./tmp/");
}

char *get_filename_from_path(char *path, int extension)
{
    char *dirpos = strrchr(path, '/');
    if (!dirpos)
	dirpos = path;

    char *end = strrchr(path, '\0');

    int offset = 0;
    if (!extension) {
	char *ext = strrchr(path, '.');
	offset = end - ext;
	printf("Making without extension ...\n");
    }

    char *filename = malloc(sizeof(char) * (end - dirpos - offset + 1));
    strncpy(filename, dirpos, end - dirpos - offset);
    return filename;
}

char *get_directory_from_path(char *path)
{
    const char *dirpos = strrchr(path, '/');
    char *directory = (char *) malloc(sizeof(char) * (dirpos - path + 1));
    strncpy(directory, path, dirpos - path);
    return directory;
}

FILE *res_open(char *path, const char *tag)
{
    strncpy(pathbuf, DATA_PATH, MAXPATH);
    strncat(pathbuf, path, MAXPATH);
    FILE *f = fopen(pathbuf, tag);
    return f;
}

static int ext_check(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
    if (typeflag == FTW_F) {
	const char *ext = strrchr(path, '.');
	if (ext != NULL && !strcmp(ext, cur_ext))
	    vec_add(c_vec, path);
    }

    return 0;
}

void fill_extensions(struct vec *vec, const char *path, const char *ext)
{
    c_vec = vec;
    cur_ext = ext;
    vec_clear(c_vec);
    nftw(path, &ext_check, 10, 0);
}

void findPrefabs()
{
    fill_extensions(prefabs, DATA_PATH, EXT_PREFAB);
}

FILE *path_open(const char *tag, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vsprintf(pathbuf, fmt, args);
    va_end(args);

    FILE *f = fopen(pathbuf, tag);
    return f;
}

char *make_path(const char *file)
{
    strncpy(pathbuf, DATA_PATH, MAXPATH);
    strncat(pathbuf, file, MAXPATH);
    return pathbuf;
}

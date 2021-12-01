#include "resources.h"

#include "config.h"
#include <dirent.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "vec.h"
#include <ftw.h>
#include <stdarg.h>
#include <stdio.h>

char *DATA_PATH = NULL;
char *PREF_PATH = NULL;

struct vec *prefabs = NULL;

const char *EXT_PREFAB = ".prefab";
const char *EXT_LEVEL = ".level";
int stemlen = 0;

static const char *cur_ext = NULL;
static DIR *cur_dir = NULL;
struct dirent *c_dirent = NULL;
struct vec *c_vec = NULL;

char pathbuf[MAXPATH];


void resources_init()
{
    char *dpath = SDL_GetBasePath();
    DATA_PATH = malloc(strlen(dpath) + 1);
    strcpy(DATA_PATH, dpath);

    char *ppath = SDL_GetPrefPath("Odplot", "Test Game");
    PREF_PATH = malloc(strlen(ppath) + 1);
    strcpy(PREF_PATH, ppath);
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

    char *filename =
	(char *) malloc(sizeof(char) * (end - dirpos - offset + 1));
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

static int ext_check(const char *path, const struct stat *sb, int typeflag,
		     struct FTW *ftwbuf)
{
    if (typeflag == FTW_F) {
	const char *ext = strrchr(path, '.');
	if (ext != NULL && !strcmp(ext, cur_ext))
	    vec_add(c_vec, path);
    }

    return 0;
}

void fill_extensions(struct vec *vec, char *path, const char *ext)
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

FILE *path_open(const char *fmt, const char *tag, ...)
{
    va_list args;
    va_start(args, fmt);
    vsprintf(pathbuf, fmt, args);
    va_end(args);

    FILE *f = fopen(pathbuf, tag);
    return f;
}
#include <string.h>
#include <stddef.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include "cdb.h"

static struct cdb_make cdbm;

/* Take all input files and zip them into a cdb */
int main(int argc, char **argv)
{
  static int cmd = 0;
  static char *file = "out.cdb";
  int fd;
  char *key, *va;
  unsigned klen, vlen;
  fd = open(file,O_RDWR|O_CREAT);

  cdb_make_start(&cdbm, fd);

  for (int i = 1; i < argc; i++) {
    FILE *f;
    char *file = argv[i];
    f = fopen(file, "rb");
    fseek(f,0,SEEK_END);
    size_t fsize = ftell(f);
    rewind(f);
    void *slurp = malloc(fsize);
    fread(slurp,fsize,1,f);
    fclose(f);
    cdb_make_add(&cdbm, file, strlen(file), slurp, fsize);
    free(slurp);
  }
  cdb_make_finish(&cdbm);

  return 0;
}

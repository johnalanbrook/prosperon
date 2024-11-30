#include "yugine.h"
#include "script.h"
#include <string.h>

#include "physfs.h"

int main(int argc, char **argv) {
  PHYSFS_init(argv[0]);
  char *base = PHYSFS_getBaseDir();
  PHYSFS_setWriteDir(base);
  PHYSFS_mount(base,NULL,0);
  script_startup(); // runs engine.js

  int argsize = 0;
  for (int i = 0; i < argc; i++) {
    argsize += strlen(argv[i]);
    if (argc > i+1) argsize++;
  }

  char cmdstr[argsize+1];
  cmdstr[0] = '\0';

  for (int i = 0; i < argc; i++) {
    strcat(cmdstr, argv[i]);
    if (argc > i+1) strcat(cmdstr, " ");
  }

  script_evalf("cmd_args('%s');", cmdstr);

  return 0;
}

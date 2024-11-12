#include "yugine.h"
#include "script.h"
#include <string.h>

int main(int argc, char **argv) {
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
  printf("evaling: %s\n", cmdstr);  
  script_evalf("cmd_args('%s');", cmdstr);

  return 0;
}

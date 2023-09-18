#include <stdio.h>
#include <stdarg.h>
#include "quickjs.h"
#include <string.h>
#include <stdlib.h>

static JSContext *js = NULL;
static JSRuntime *rt = NULL;

int main (int argc, char **argv)
{
  if (argc < 2) {
    printf("Must supply with one or more javascript files.\n");
    return 1;
  }

  rt = JS_NewRuntime();
  JS_SetMaxStackSize(rt,0);
  js = JS_NewContext(rt);

  for (int i = 1; i < argc; i++) {
    FILE *f = fopen(argv[i], "rb");
    if (!f) {
      printf("Could not find file %s.\n", argv[i]);
      continue;
    }

    fseek(f,0,SEEK_END);
    size_t fsize = ftell(f);
    rewind(f);
    void *script = malloc(fsize);
    fread(script,fsize,1,f);
    fclose(f);
    JSValue obj = JS_Eval(js, script, fsize, argv[i], JS_EVAL_FLAG_COMPILE_ONLY);
    size_t out_len;
    uint8_t *out;
    out = JS_WriteObject(js, &out_len, obj, JS_WRITE_OBJ_BYTECODE);

    printf(out);
    return 0;
  }

  return 0;
}

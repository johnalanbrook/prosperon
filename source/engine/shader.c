#include "shader.h"

#include "config.h"
#include "font.h"
#include "log.h"
#include "render.h"
#include "resources.h"
#include "stb_ds.h"
#include "timer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "time.h"

#define SHADER_BUF 10000

static struct shader *shaders;

struct shader *MakeShader(const char *vertpath, const char *fragpath) {
  if (arrcap(shaders) == 0)
    arrsetcap(shaders, 20);

  struct shader init = {
      .vertpath = vertpath,
      .fragpath = fragpath};
  shader_compile(&init);
  arrput(shaders, init);
  return &arrlast(shaders);
}

void shader_compile(struct shader *shader) {
  YughInfo("Making shader with %s and %s.", shader->vertpath, shader->fragpath);
  char spath[MAXPATH];
  sprintf(spath, "%s%s", "shaders/", shader->vertpath);
  const char *vsrc = slurp_text(spath, NULL);
  sprintf(spath, "%s%s", "shaders/", shader->fragpath);
  const char *fsrc = slurp_text(spath, NULL);

  shader->shd = sg_make_shader(&(sg_shader_desc){
      .vs.source = vsrc,
      .fs.source = fsrc,
      .label = shader->vertpath,
  });

  free(vsrc);
  free(fsrc);
}

void shader_compile_all() {
  for (int i = 0; i < arrlen(shaders); i++)
    shader_compile(&shaders[i]);
}


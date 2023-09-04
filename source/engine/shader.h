#ifndef SHADER_H
#define SHADER_H

#include "sokol/sokol_gfx.h"

struct shader {
  sg_shader shd;
  const char *vertpath;
  const char *fragpath;
};

void shader_compile_all();
struct shader *MakeShader(const char *vertpath, const char *fragpath);
void shader_compile(struct shader *shader);
#endif

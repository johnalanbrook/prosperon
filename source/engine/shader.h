#ifndef SHADER_H
#define SHADER_H

#include "mathc.h"
#include "sokol/sokol_gfx.h"

struct shader {
  sg_shader shd;
  const char *vertpath;
  const char *fragpath;
};

void shader_compile_all();
struct shader *MakeShader(const char *vertpath, const char *fragpath);
void shader_compile(struct shader *shader);
void shader_use(struct shader *shader);
/*
void shader_setbool(struct shader *shader, const char *name, int val);
void shader_setint(struct shader *shader, const char *name, int val);
void shader_setfloat(struct shader *shader, const char *name, float val);

void shader_setvec2(struct shader *shader, const char *name, mfloat_t val[2]);
void shader_setvec3(struct shader *shader, const char *name, mfloat_t val[3]);
void shader_setvec4(struct shader *shader, const char *name, mfloat_t val[4]);
void shader_setmat2(struct shader *shader, const char *name, mfloat_t val[4]);
void shader_setmat3(struct shader *shader, const char *name, mfloat_t val[9]);
void shader_setmat4(struct shader *shader, const char *name, mfloat_t val[16]);

void shader_setUBO(struct shader *shader, const char *name, unsigned int index);
*/
#endif

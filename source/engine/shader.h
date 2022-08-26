#ifndef SHADER_H
#define SHADER_H

#include "mathc.h"

struct mShader {
    unsigned int id;
    const char *vertpath;
    const char *fragpath;
};

void shader_compile_all();
struct mShader *MakeShader(const char *vertpath, const char *fragpath);
void shader_compile(struct mShader *shader);
void shader_use(struct mShader *shader);

void shader_setbool(struct mShader *shader, const char *name, int val);
void shader_setint(struct mShader *shader, const char *name, int val);
void shader_setfloat(struct mShader *shader, const char *name, float val);

void shader_setvec2(struct mShader *shader, const char *name, mfloat_t val[2]);
void shader_setvec3(struct mShader *shader, const char *name, mfloat_t val[3]);
void shader_setvec4(struct mShader *shader, const char *name, mfloat_t val[4]);
void shader_setmat2(struct mShader *shader, const char *name, mfloat_t val[4]);
void shader_setmat3(struct mShader *shader, const char *name, mfloat_t val[9]);
void shader_setmat4(struct mShader *shader, const char *name, mfloat_t val[16]);

void shader_setUBO(struct mShader *shader, const char *name, unsigned int index);

#endif

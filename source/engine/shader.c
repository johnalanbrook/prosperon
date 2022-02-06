#include "shader.h"

#include "render.h"
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "log.h"
#include "resources.h"

#define SHADER_BUF 10000

struct mShader *mshaders[255];
struct mShader **lastShader = mshaders;

struct mShader *MakeShader(const char *vertpath, const char *fragpath)
{
    struct mShader init = { 0, vertpath, fragpath };
    struct mShader *newshader =
	(struct mShader *) malloc(sizeof(struct mShader));
    memcpy(newshader, &init, sizeof(*newshader));
    *lastShader++ = newshader;
    shader_compile(newshader);
    return newshader;
}

int shader_compile_error(GLuint shader)
{
    GLint success = 0;
    GLchar infoLog[ERROR_BUFFER] = { '\0' };

    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success) return 0;

    glGetShaderInfoLog(shader, ERROR_BUFFER, NULL, infoLog);
    YughLog(0, LOG_ERROR, "Shader compilation error.\nLog: %s", infoLog);

    return 1;
}

int shader_link_error(GLuint shader)
{
    GLint success = 0;
    GLchar infoLog[ERROR_BUFFER] = { '\0' };

    glGetProgramiv(shader, GL_LINK_STATUS, &success);
    if (success) return 0;

    glGetProgramInfoLog(shader, ERROR_BUFFER, NULL, infoLog);
    YughLog(0, LOG_ERROR, "Shader link error.\nLog: %s", infoLog);

    return 1;
}

GLuint load_shader_from_file(const char *path, int type)
{
    char spath[MAXPATH] = {'\0'};

    sprintf(spath, "%s%s", "shaders/", path);
    FILE *f = fopen(make_path(spath), "r'");
    if (!path)
        perror(spath), exit(1);

    char buf[SHADER_BUF] = {'\0'};
    long int fsize;
    fseek(f, 0, SEEK_END);
    fsize = ftell(f);
    rewind(f);
    fread(buf, fsize, 1, f);

    fclose(f);


    GLuint id = glCreateShader(type);
    const char *code = buf;
    glShaderSource(id, 1, &code, NULL);
    glCompileShader(id);
    if ( shader_compile_error(id))
        return 0;

    return id;
}

void shader_compile(struct mShader *shader)
{
    GLuint vert = load_shader_from_file(shader->vertpath, GL_VERTEX_SHADER);
    GLuint frag = load_shader_from_file(shader->fragpath, GL_FRAGMENT_SHADER);

    shader->id = glCreateProgram();
    glAttachShader(shader->id, vert);
    glAttachShader(shader->id, frag);
    glLinkProgram(shader->id);
    shader_link_error(shader->id);

    glDeleteShader(vert);
    glDeleteShader(frag);
}

void shader_use(struct mShader *shader)
{
    glUseProgram(shader->id);
}

void shader_setbool(struct mShader *shader, const char *name, int val)
{
    glUniform1i(glGetUniformLocation(shader->id, name), val);
}

void shader_setint(struct mShader *shader, const char *name, int val)
{
    glUniform1i(glGetUniformLocation(shader->id, name), val);
}

void shader_setfloat(struct mShader *shader, const char *name, float val)
{
    glUniform1f(glGetUniformLocation(shader->id, name), val);
}

void shader_setvec2(struct mShader *shader, const char *name,
		    mfloat_t val[2])
{
    glUniform2fv(glGetUniformLocation(shader->id, name), 1, val);
}

void shader_setvec3(struct mShader *shader, const char *name,
		    mfloat_t val[3])
{
    glUniform3fv(glGetUniformLocation(shader->id, name), 1, val);
}

void shader_setvec4(struct mShader *shader, const char *name,
		    mfloat_t val[4])
{
    glUniform4fv(glGetUniformLocation(shader->id, name), 1, val);
}

void shader_setmat2(struct mShader *shader, const char *name,
		    mfloat_t val[4])
{
    glUniformMatrix2fv(glGetUniformLocation(shader->id, name), 1, GL_FALSE,
		       val);
}

void shader_setmat3(struct mShader *shader, const char *name,
		    mfloat_t val[9])
{
    glUniformMatrix3fv(glGetUniformLocation(shader->id, name), 1, GL_FALSE,
		       val);
}

void shader_setmat4(struct mShader *shader, const char *name,
		    mfloat_t val[16])
{
    shader_use(shader);
    glUniformMatrix4fv(glGetUniformLocation(shader->id, name), 1, GL_FALSE,
		       val);
}

void shader_setUBO(struct mShader *shader, const char *name,
		   unsigned int index)
{
    glUniformBlockBinding(shader->id,
			  glGetUniformBlockIndex(shader->id, name), index);
}



void shader_compile_all()
{
    struct mShader **curshader = mshaders;
    do {
	YughLog(0, LOG_INFO, "Compiled Shader %d", 1);
	shader_compile(*curshader);
    } while (++curshader != lastShader);

}

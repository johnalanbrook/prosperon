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

int shader_compile_error(int shader) {
  /*
      GLint success = 0;
      GLchar infoLog[ERROR_BUFFER] = { '\0' };

      glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
      if (success) return 0;

      glGetShaderInfoLog(shader, ERROR_BUFFER, NULL, infoLog);
      YughLog(0, LOG_ERROR, "Shader compilation error.\nLog: %s", infoLog);

      return 1;
  */
}

int shader_link_error(int shader) {
  /*
      GLint success = 0;
      GLchar infoLog[ERROR_BUFFER] = { '\0' };

      glGetProgramiv(shader, GL_LINK_STATUS, &success);
      if (success) return 0;

      glGetProgramInfoLog(shader, ERROR_BUFFER, NULL, infoLog);
      YughLog(0, LOG_ERROR, "Shader link error.\nLog: %s", infoLog);

      return 1;
  */
}

int load_shader_from_file(const char *path, int type) {
  char spath[MAXPATH] = {'\0'};

  sprintf(spath, "%s%s", "shaders/", path);
  FILE *f = fopen(make_path(spath), "r'");
  if (!path)
    perror(spath), exit(1);

  char *buf;
  long int fsize;
  fseek(f, 0, SEEK_END);
  fsize = ftell(f);
  buf = malloc(fsize + 1);
  rewind(f);
  size_t r = fread(buf, sizeof(char), fsize, f);
  buf[r] = '\0';

  fclose(f);

  /*
      GLuint id = glCreateShader(type);
      const char *code = buf;
      glShaderSource(id, 1, &code, NULL);
      glCompileShader(id);
      if (shader_compile_error(id)) {
          YughError("Error with shader %s.", path);
          return 0;
      }

      free(buf);


      return id;
   */
}

void shader_compile(struct shader *shader) {
  YughInfo("Making shader with %s and %s.", shader->vertpath, shader->fragpath);
  char spath[MAXPATH];
  sprintf(spath, "%s%s", "shaders/", shader->vertpath);
  const char *vsrc = slurp_text(spath);
  sprintf(spath, "%s%s", "shaders/", shader->fragpath);
  const char *fsrc = slurp_text(spath);

  shader->shd = sg_make_shader(&(sg_shader_desc){
      .vs.source = vsrc,
      .fs.source = fsrc,
      .label = shader->vertpath,
  });

  free(vsrc);
  free(fsrc);
}

void shader_use(struct shader *shader) {
  //    glUseProgram(shader->id);
}

void shader_compile_all() {
  for (int i = 0; i < arrlen(shaders); i++)
    shader_compile(&shaders[i]);
}


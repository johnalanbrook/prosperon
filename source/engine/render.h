#ifndef RENDER_H
#define RENDER_H

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>


struct uv_n {
  unsigned short u;
  unsigned short v;
};

struct st_n {
  struct uv_n s;
  struct uv_n t;
};

#endif

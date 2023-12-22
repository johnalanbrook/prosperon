#ifndef OPENGL_RENDER_H
#define OPENGL_RENDER_H

#if defined __linux__
  #define SOKOL_GLCORE33
#elif __EMSCRIPTEN__
  #define SOKOL_GLES3
#elif __WIN32
  #define SOKOL_D3D11
  #define SOKOL_WIN32_FORCE_MAIN
#elif __APPLE__
  #define SOKOL_METAL
#endif

#include "sokol/sokol_gfx.h"
#include "HandmadeMath.h"

#define RGBA_MAX 255

#include "window.h"

extern struct rgba color_white;
extern struct rgba color_black;

extern int renderMode;

extern HMM_Vec3 dirl_pos;

extern HMM_Mat4 projection;
extern HMM_Mat4 hudproj;

struct camera3d {
  
};

typedef struct camera3d camera3d;

struct draw_p {
  float x;
  float y;
};

#include <chipmunk/chipmunk.h>

enum RenderMode {
    LIT,
    UNLIT,
    WIREFRAME,
    DIRSHADOWMAP,
    OBJECTPICKER
};

void render_init();
void openglRender(struct window *window);
void opengl_rendermode(enum RenderMode r);

void openglInit3d(struct window *window);
void openglRender3d(struct window *window, camera3d *camera);
void capture_screen(int x, int y, int w, int h, const char *path);

void render_winsize();

void debug_draw_phys(int draw);

void set_cam_body(cpBody *body);
cpVect cam_pos();
float cam_zoom();
void add_zoom(float val);
HMM_Vec2 world2screen(HMM_Vec2 pos);
HMM_Vec2 screen2world(HMM_Vec2 pos);

sg_shader sg_compile_shader(const char *v, const char *f, sg_shader_desc *d);

void gif_rec_start(int w, int h, int cpf, int bitdepth);
void gif_rec_end(const char *path);

struct uv_n {
  unsigned short u;
  unsigned short v;
};

struct st_n {
  struct uv_n s;
  struct uv_n t;
};

struct rgba {
  unsigned char r;
  unsigned char g;
  unsigned char b;
  unsigned char a;
};

struct boundingbox {
  float t;
  float b;
  float r;
  float l;
};

struct rect {
  float h, w, x, y;
};

/* Normalized S,T coordinates for rendering */
struct glrect {
    float s0;
    float s1;
    float t0;
    float t1;
};

struct boundingbox cwh2bb(HMM_Vec2 c, HMM_Vec2 wh);
float *rgba2floats(float *r, struct rgba c);
extern sg_blend_state blend_trans;
#endif

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

struct mCamera;
struct window;

extern struct shader *spriteShader;
extern struct shader *animSpriteShader;

extern sg_image ddimg;

extern struct sprite *tsprite;

extern int renderMode;

extern HMM_Vec3 dirl_pos;

extern HMM_Mat4 projection;
extern HMM_Mat4 hudproj;

struct draw_p {
  float x;
  float y;
};

extern float gridScale;
extern float smallGridUnit;
extern float bigGridUnit;
extern float gridSmallThickness;
extern float gridBigThickness;
extern struct rgba gridBigColor;
extern struct rgba gridSmallColor;
extern float gridOpacity;
extern float editorFOV;
extern float shadowLookahead;
extern char objectName[];
extern int debugColorPickBO;

extern struct gameobject *selectedobject;

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
void openglRender3d(struct window *window, struct mCamera *camera);
void capture_screen(int x, int y, int w, int h, char *path);

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
void gif_rec_end(char *path);

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

static struct boundingbox cwh2bb(HMM_Vec2 c, HMM_Vec2 wh) {
  struct boundingbox bb = {
    .t = c.Y + wh.Y/2,
    .b = c.Y - wh.Y/2,
    .r = c.X + wh.X/2,
    .l = c.X - wh.X/2
  };

  return bb;
}

static float *rgba2floats(float *r, struct rgba c)
{
  r[0] = (float)c.r / RGBA_MAX;
  r[1] = (float)c.g / RGBA_MAX;
  r[2] = (float)c.b / RGBA_MAX;
  r[3] = (float)c.a / RGBA_MAX;
  return r;
}

static sg_blend_state blend_trans = {
  .enabled = true,
  .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
  .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
  .src_factor_alpha = SG_BLENDFACTOR_SRC_ALPHA,
  .dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA
};

#endif

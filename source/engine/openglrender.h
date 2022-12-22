#ifndef OPENGL_RENDER_H
#define OPENGL_RENDER_H

#include "render.h"

struct mCamera;
struct window;

extern struct shader *spriteShader;
extern struct shader *animSpriteShader;

extern struct sprite *tsprite;

extern int renderMode;

extern float gridScale;
extern float smallGridUnit;
extern float bigGridUnit;
extern float gridSmallThickness;
extern float gridBigThickness;
extern float gridBigColor[];
extern float gridSmallColor[];
extern float gridOpacity;
extern float editorFOV;
extern float shadowLookahead;
extern char objectName[];
extern GLuint debugColorPickBO;

extern struct gameobject *selectedobject;

#include <chipmunk/chipmunk.h>

enum RenderMode {
    LIT,
    UNLIT,
    WIREFRAME,
    DIRSHADOWMAP,
    OBJECTPICKER
};

void openglInit();
void openglRender(struct window *window);

void openglInit3d(struct window *window);
void openglRender3d(struct window *window, struct mCamera *camera);

void debug_draw_phys(int draw);

void BindUniformBlock(GLuint shaderID, const char *bufferName, GLuint bufferBind);

void set_cam_body(cpBody *body);
cpVect cam_pos();
void add_zoom(float val);

#endif

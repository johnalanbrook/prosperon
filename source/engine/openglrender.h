#ifndef OPENGL_RENDER_H
#define OPENGL_RENDER_H

#include "render.h"

struct mCamera;
struct window;

extern struct shader *spriteShader;
extern struct shader *animSpriteShader;

extern struct sprite *tsprite;

extern int renderMode;


extern float editorClose;
extern float editorFar;
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
extern float plane_size;
extern float near_plane;
extern float far_plane;
extern char objectName[];
extern GLuint debugColorPickBO;

extern struct gameobject *selectedobject;

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

void BindUniformBlock(GLuint shaderID, const char *bufferName,
		      GLuint bufferBind);

#endif

#ifndef OPENGL_RENDER_H
#define OPENGL_RENDER_H

#include "render.h"

struct mCamera;
struct mSDLWindow;

extern struct mShader *spriteShader;
extern struct mShader *animSpriteShader;

extern struct mSprite *tsprite;

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

extern struct mGameObject *selectedobject;

enum RenderMode {
    LIT,
    UNLIT,
    WIREFRAME,
    DIRSHADOWMAP,
    OBJECTPICKER
};

void openglInit();
void openglRender(struct mSDLWindow *window);

void openglInit3d(struct mSDLWindow *window);
void openglRender3d(struct mSDLWindow *window, struct mCamera *camera);

void BindUniformBlock(GLuint shaderID, const char *bufferName,
		      GLuint bufferBind);

#endif

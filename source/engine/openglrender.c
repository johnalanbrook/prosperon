#include "openglrender.h"

#include "sprite.h"
#include "shader.h"
#include "font.h"
#include "config.h"
#include "static_actor.h"
#include "gameobject.h"
#include "camera.h"
#include "window.h"
#include "debugdraw.h"
#include "log.h"
#include "datastream.h"

int renderMode = 0;

static GLuint UBO;


static GLuint gridVBO = 0;
static GLuint gridVAO = 0;

static GLuint quadVAO = 0;
static GLuint quadVBO = 0;

static GLuint depthMapFBO = 0;
static GLuint depthMap = 0;
const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;

static struct shader *outlineShader;
static struct shader *modelShader;
static struct shader *shadowShader;

struct shader *spriteShader = NULL;
struct shader *animSpriteShader = NULL;
static struct shader *textShader;
static struct shader *diffuseShader;

struct sFont *stdFont;

static struct shader *debugDepthQuad;
static struct shader *debugColorPickShader;
static struct shader *debugGridShader;
static struct shader *debugGizmoShader;

struct mStaticActor *gizmo;

float editorFOV = 45.f;
float editorClose = 0.1f;
float editorFar = 1000.f;
mfloat_t editorClearColor[4] = { 0.2f, 0.4f, 0.3f, 1.f };

float shadowLookahead = 8.5f;

mfloat_t gridSmallColor[3] = { 0.35f, 1.f, 0.9f };

mfloat_t gridBigColor[3] = { 0.92f, 0.92f, 0.68f };

float gridScale = 500.f;
float smallGridUnit = 1.f;
float bigGridUnit = 10.f;
float gridSmallThickness = 2.f;
float gridBigThickness = 7.f;
float gridOpacity = 0.3f;

mfloat_t proj[16];

float near_plane = -100.f, far_plane = 10.f, plane_size = 60.f;


// Debug render modes
bool renderGizmos = false;
bool showGrid = true;
bool debugDrawPhysics = false;
bool renderNav = false;

// Lighting effect flags
bool renderAO = true;
bool renderDynamicShadows = true;
bool renderRefraction = true;
bool renderReflection = true;

///// for editing
struct gameobject *selectedobject = NULL;
char objectName[200] = { '\0' };	// object name buffer

GLuint debugColorPickBO = 0;
GLuint debugColorPickTEX = 0;


struct sprite *tsprite = NULL;


static unsigned int projUBO;

void openglInit()
{
    if (!mainwin) {
        YughError("No window to init OpenGL on.", 1);
        exit(1);
    }

    ////// MAKE SHADERS
    spriteShader = MakeShader("spritevert.glsl", "spritefrag.glsl");
    animSpriteShader = MakeShader("animspritevert.glsl", "animspritefrag.glsl");
    textShader = MakeShader("textvert.glsl", "textfrag.glsl");



    shader_use(textShader);
    shader_setint(textShader, "text", 0);



    font_init(textShader);
    sprite_initialize();
    debugdraw_init();

    //glEnable(GL_STENCIL_TEST);


    glClearColor(editorClearColor[0], editorClearColor[1], editorClearColor[2], editorClearColor[3]);

    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //glDisable(GL_DEPTH_TEST);
    //glLineWidth(1.3f);

    //glEnable(GL_TEXTURE_3D);
    //glEnable(GL_MULTISAMPLE);
    //glLineWidth(2);


    glGenBuffers(1, &projUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, projUBO);
    glBufferData(GL_UNIFORM_BUFFER, 64, NULL, GL_DYNAMIC_DRAW);
    glBindBufferRange(GL_UNIFORM_BUFFER, 0, projUBO, 0, sizeof(float) * 16);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    shader_setUBO(spriteShader, "Projection", 0);
    shader_setUBO(textShader, "Projection", 0);
    shader_setUBO(animSpriteShader, "Projection", 0);
}


static struct mCamera mcamera = {0};
void openglRender(struct window *window)
{
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0.3f, 0.3f, 0.3f, 1.f);


    //////////// 2D projection
    mfloat_t projection[16] = { 0.f };
    mat4_ortho(projection, mcamera.transform.position[0],
	       window->width + mcamera.transform.position[0],
	       mcamera.transform.position[1],
	       window->height + mcamera.transform.position[1], -1.f, 1.f);
    glBindBuffer(GL_UNIFORM_BUFFER, projUBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, 64, projection);

    //shader_setmat4(vid_shader, "projection", projection);
   glEnable(GL_DEPTH_TEST);

    // Clear color and depth
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    ////// TEXT && GUI
    script_call_sym(window->gui_cb);

    ///// Sprites
    glDepthFunc(GL_LESS);
    shader_use(spriteShader);
    sprite_draw_all();

     glDepthFunc(GL_ALWAYS);
     shader_use(textShader);
     shader_setmat4(textShader, "projection", projection);

}



































void openglInit3d(struct window *window)
{
/*    TODO: IMG init doesn't work in C++
   int init =(0x00000001 | 0x00000002);
    int initted =IMG_Init(init);
    YughLog(0, SDL_LOG_PRIORITY_ERROR, "Init flags: %d\nInitted values: %d ", init, initted);
    if ((initted & (IMG_INIT_JPG | IMG_INIT_PNG)) != (IMG_INIT_JPG | IMG_INIT_PNG)) {
	YughLog(0, SDL_LOG_PRIORITY_ERROR,
	"IMG_Init: Failed to init required jpg and png support! SDL_IMG error: %s",
		 IMG_GetError());

    }
*/



    ////// MAKE SHADERS
    outlineShader = MakeShader("outlinevert.glsl", "outline.glsl");
    diffuseShader = MakeShader("simplevert.glsl", "albedofrag.glsl");
    modelShader = MakeShader("modelvert.glsl", "modelfrag.glsl");
    shadowShader = MakeShader("shadowvert.glsl", "shadowfrag.glsl");

    textShader = MakeShader("textvert.glsl", "textfrag.glsl");
    spriteShader = MakeShader("spritevert.glsl", "spritefrag.glsl");

    debugDepthQuad = MakeShader("postvert.glsl", "debugdepthfrag.glsl");
    debugColorPickShader =
	MakeShader("simplevert.glsl", "debugcolorfrag.glsl");
    debugGridShader = MakeShader("gridvert.glsl", "gridfrag.glsl");
    debugGizmoShader = MakeShader("gizmovert.glsl", "gizmofrag.glsl");

    stdFont = MakeFont("notosans.ttf", 300);

    shader_compile_all();

    mat4_perspective_fov(proj, editorFOV * DEG2RADS, window->width,
			 window->height, editorClose, editorFar);

    glEnable(GL_STENCIL_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_3D);
    glEnable(GL_MULTISAMPLE);
    glLineWidth(2);

    ////
    // Shadow mapping buffers
    ////
    glGenFramebuffers(1, &depthMapFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);

    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, SHADOW_WIDTH,
		 SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
			   GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    //glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //// Universal buffer to hold projection and light coordinates
    glGenBuffers(1, &UBO);
    glBindBuffer(GL_UNIFORM_BUFFER, UBO);

    glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(proj), NULL,
		 GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBufferRange(GL_UNIFORM_BUFFER, 0, UBO, 0, 2 * sizeof(proj));


    glBindBuffer(GL_UNIFORM_BUFFER, UBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(proj), proj);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    ////// debug color pick buffer
    glGenFramebuffers(1, &debugColorPickBO);
    glBindFramebuffer(GL_FRAMEBUFFER, debugColorPickBO);

    glGenTextures(1, &debugColorPickTEX);
    glBindTexture(GL_TEXTURE_2D, debugColorPickTEX);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCREEN_WIDTH, SCREEN_HEIGHT, 0,
		 GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			   GL_TEXTURE_2D, debugColorPickTEX, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    //////// Create grid
    float gridVertices[] = {
	-1.f, 0.f, 1.f, 0.f, 1.f,
	-1.f, 0.f, -1.f, 0.f, 0.f,
	1.f, 0.f, 1.f, 1.f, 1.f,
	1.f, 0.f, -1.f, 1.f, 0.f,
    };

    glGenVertexArrays(1, &gridVAO);
    glGenBuffers(1, &gridVBO);
    glBindVertexArray(gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(gridVertices), &gridVertices,
		 GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
			  (void *) 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
			  (void *) (3 * sizeof(float)));

    //////// Create post quad
    float quadVertices[] = {
	// positions        // texture Coords
	-1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
	-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
	1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
	1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
    };
    // setup plane VAO
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices,
		 GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
			  (void *) 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
			  (void *) (3 * sizeof(float)));

    //////

    //  skybox = new Skybox("skybox");

    BindUniformBlock(modelShader->id, "Matrices", 0);
    BindUniformBlock(outlineShader->id, "Matrices", 0);
    //  BindUniformBlock(skybox->shader->id, "Matrices", 0);
    BindUniformBlock(diffuseShader->id, "Matrices", 0);

    BindUniformBlock(debugGridShader->id, "Matrices", 0);
    BindUniformBlock(debugGizmoShader->id, "Matrices", 0);

    shader_use(debugDepthQuad);
    shader_setint(debugDepthQuad, "depthMap", 0);

    glBindTexture(GL_TEXTURE_2D, 0);

    //////////// 2D projection
    mfloat_t projection[16] = { 0.f };
    mat4_ortho(projection, 0.f, SCREEN_WIDTH, SCREEN_HEIGHT, 1.f, -1.f,
	       1.f);
    shader_setmat4(textShader, "projection", projection);
    shader_setmat4(spriteShader, "projection", projection);
    shader_setmat4(debugGizmoShader, "proj", projection);




}

void openglRender3d(struct window *window, struct mCamera *mcamera)
{
    //////// SET NEW PROJECTION
    // TODO: Make this not happen every frame
    mat4_perspective_fov(proj, editorFOV * DEG2RADS, window->width,
			 window->height, editorClose, editorFar);

    glBindBuffer(GL_UNIFORM_BUFFER, UBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(proj), proj);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    ////////// Render a depthmap from the perspective of the directional light

    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glCullFace(GL_BACK);
    glClear(GL_DEPTH_BUFFER_BIT);

    // Configure matrices with an orthogonal

    mfloat_t lightSpaceMatrix[16] = { 0.f };

/*
    if (dLight) {

	mfloat_t lightProjection[16] = { 0.f };
	mat4_ortho(lightProjection, -plane_size, plane_size, -plane_size,
		   plane_size, near_plane, far_plane);

	mfloat_t lookPos[3] = { 0.f };
	mfloat_t cam_fwd[3] = { 0.f };
	vec3_add(lookPos, mcamera->transform.position,
		 vec3_multiply_f(lookPos,
				 trans_forward(cam_fwd,
					       &mcamera->transform),
				 shadowLookahead));
	lookPos[1] = 0.f;

	mfloat_t lightLookPos[3] = { 0.f };
	mfloat_t light_fwd[3] = { 0.f };
	mat4_look_at(lightView,
		     vec3_subtract(lightLookPos, lookPos,
				   trans_forward(light_fwd,
						 &dLight->light.obj.
						 transform)), lookPos, UP);

	mat4_multiply(lightSpaceMatrix, lightProjection, lightView);
	//lightSpaceMatrix = lightProjection * lightView;

	if (renderDynamicShadows) {
	    shader_use(shadowShader);
	    shader_setmat4(shadowShader, "lightSpaceMatrix",
			   lightSpaceMatrix);
	    staticactor_draw_shadowcasters(shadowShader);
	}
    }
    */
    //////////////////////////

    // Back to the normal render
    window_makecurrent(window);


    glCullFace(GL_BACK);

    // Render the color thing for debug picking
    glBindFramebuffer(GL_FRAMEBUFFER, debugColorPickBO);
    glClearColor(editorClearColor[0], editorClearColor[1],
		 editorClearColor[2], editorClearColor[3]);
    glClear(GL_COLOR_BUFFER_BIT);
    shader_use(debugColorPickShader);
    staticactor_draw_dbg_color_pick(debugColorPickShader);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Clear color and depth
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
	    GL_STENCIL_BUFFER_BIT);

    if (renderMode == DIRSHADOWMAP) {
	// render Depth map to quad for visual debugging
	// ---------------------------------------------
	shader_use(debugDepthQuad);
	shader_setfloat(debugDepthQuad, "near_plane", near_plane);
	shader_setfloat(debugDepthQuad, "far_plane", far_plane);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
    } else if (renderMode == OBJECTPICKER) {
	// TODO: This rendering mode
	shader_use(debugColorPickShader);
    } else {

	glClearColor(editorClearColor[0], editorClearColor[1],
		     editorClearColor[2], editorClearColor[3]);
	glDepthMask(GL_TRUE);

	mfloat_t view[16] = { 0.f };
	getviewmatrix(view, mcamera);

	glBindBuffer(GL_UNIFORM_BUFFER, UBO);
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(view), sizeof(view),
			view);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	switch (renderMode) {
	case LIT:
	    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	    shader_use(modelShader);

/*
	    if (dLight)
		dlight_prepshader(dLight, modelShader);

	    pointlights_prepshader(modelShader);
	    spotlights_prepshader(modelShader);
*/

	    shader_setvec3(modelShader, "viewPos",
			   mcamera->transform.position);
	    shader_setmat4(modelShader, "lightSpaceMatrix",
			   lightSpaceMatrix);
	    shader_setint(modelShader, "shadowMap", 12);
	    glActiveTexture(GL_TEXTURE);
	    glBindTexture(GL_TEXTURE_2D, depthMap);
	    staticactor_draw_models(modelShader);


	    break;

	case UNLIT:
	    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	    shader_use(diffuseShader);
	    staticactor_draw_models(diffuseShader);

	    break;

	case WIREFRAME:
	    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	    shader_use(diffuseShader);
	    staticactor_draw_models(diffuseShader);

	    break;
	}

	//// skybox
	// draw skybox as last
	glDepthFunc(GL_LEQUAL);
	//    skybox->Draw(mcamera);
	glDepthFunc(GL_LESS);





	if (debugDrawPhysics) {
	    // Render physics world
	    //dynamicsWorld->debugDrawWorld();
	}
	// Draw outline
	if (selectedobject != NULL) {
	    // Draw the selected object outlined
	    glClearStencil(0);
	    glClear(GL_STENCIL_BUFFER_BIT);
	    glStencilFunc(GL_ALWAYS, 1, 0xFF);
	    glDepthFunc(GL_ALWAYS);
	    glDepthMask(false);
	    glColorMask(false, false, false, false);
	    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	    glStencilMask(0xFF);
	    shader_use(diffuseShader);
	    setup_model_transform(&selectedobject->transform,
				  diffuseShader, 1.f);
	    //selectedobject->draw(diffuseShader);

	    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
	    glDepthMask(true);
	    glColorMask(true, true, true, true);
	    glStencilMask(0x00);
	    shader_use(outlineShader);
	    setup_model_transform(&selectedobject->transform,
				  outlineShader, 1.f);
	    //selectedobject->draw(outlineShader);


	}

	glDepthFunc(GL_LESS);
	glStencilMask(0xFF);
	glStencilFunc(GL_ALWAYS, 0, 0xFF);

    }

    ////// TEXT && GUI
    //            glCullFace(GL_FRONT);
    glDepthFunc(GL_ALWAYS);
    shader_use(textShader);
    shader_setmat4(textShader, "projection", window->projection);
    mfloat_t fontpos[2] = { 25.f, 25.f };
    mfloat_t fontcolor[3] = { 0.5f, 0.8f, 0.2f };
    text_settype(stdFont);
    renderText("Sample text", fontpos, 0.4f,
	       fontcolor, -1.f);

    sprite_draw(tsprite);

    glDepthFunc(GL_LESS);


    ////// Render grid
    if (showGrid) {
	glDisable(GL_CULL_FACE);
	shader_use(debugGridShader);
	mfloat_t gmodel[16] = { 0.f };
	mfloat_t gridscale[3] = { 0.f };
	vec3(gridscale, gridScale, gridScale, gridScale);
	mat4_multiply_f(gmodel, gmodel, gridScale);
	// TODO: Hook into here to make the grid scalable
	shader_setmat4(debugGridShader, "model", gmodel);
	shader_setvec3(debugGridShader, "smallColor", gridSmallColor);
	shader_setvec3(debugGridShader, "bigColor", gridBigColor);
	shader_setfloat(debugGridShader, "gridScale", gridScale);
	shader_setfloat(debugGridShader, "smallUnit", smallGridUnit);
	shader_setfloat(debugGridShader, "bigUnit", bigGridUnit);
	shader_setfloat(debugGridShader, "smallThickness",
			gridSmallThickness);
	shader_setfloat(debugGridShader, "largeThickness",
			gridBigThickness);
	shader_setfloat(debugGridShader, "opacity", gridOpacity);
	glBindVertexArray(gridVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
    }
    ///// Render gizmos
    // These are things that are overlaid on everything else


    // glBindFramebuffer(GL_FRAMEBUFFER, 0); // back to default
    // glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    // glClear(GL_COLOR_BUFFER_BIT);

    // postShader.use();
    // glBindVertexArray(quadVAO);
    // glBindTexture(GL_TEXTURE_2D, fboTexture);
    // glDrawArrays(GL_TRIANGLES, 0, 6);
}

void BindUniformBlock(GLuint shaderID, const char *bufferName,
		      GLuint bufferBind)
{
    glUniformBlockBinding(shaderID,
			  glGetUniformBlockIndex(shaderID, bufferName),
			  bufferBind);
}

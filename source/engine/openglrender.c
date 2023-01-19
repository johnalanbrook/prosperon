#include "openglrender.h"

#include "sprite.h"
#include "shader.h"
#include "font.h"
#include "config.h"
#include "gameobject.h"
#include "camera.h"
#include "window.h"
#include "debugdraw.h"
#include "log.h"
#include "datastream.h"



int renderMode = 0;

static GLuint UBO;

struct shader *spriteShader = NULL;
struct shader *animSpriteShader = NULL;
static struct shader *textShader;

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

void debug_draw_phys(int draw) {
    debugDrawPhysics = draw;
}

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

    glClearColor(editorClearColor[0], editorClearColor[1], editorClearColor[2], editorClearColor[3]);

    //glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glGenBuffers(1, &projUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, projUBO);
    glBufferData(GL_UNIFORM_BUFFER, 64, NULL, GL_DYNAMIC_DRAW);
    glBindBufferRange(GL_UNIFORM_BUFFER, 0, projUBO, 0, sizeof(float) * 16);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    shader_setUBO(spriteShader, "Projection", 0);
    shader_setUBO(textShader, "Projection", 0);
    shader_setUBO(animSpriteShader, "Projection", 0);
}


static cpBody *camera = NULL;
void set_cam_body(cpBody *body) {
    camera = body;
}

cpVect cam_pos() {
    return camera ? cpBodyGetPosition(camera) : cpvzero;
}

static float zoom = 1.f;
void add_zoom(float val) { zoom = val; }

void openglRender(struct window *window)
{

    //////////// 2D projection
    mfloat_t projection[16] = { 0.f };
    cpVect pos = cam_pos();
     mat4_ortho(projection, pos.x,
	   window->width*zoom + pos.x,
	   pos.y,
	   window->height*zoom + pos.y, -1.f, 1.f);

    mfloat_t ui_projection[16] = { 0.f };
    mat4_ortho(ui_projection, 0,
        window->width,
        0,
        window->height, -1.f, 1.f);

    // Clear color and depth
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glBindBuffer(GL_UNIFORM_BUFFER, projUBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, 64, projection);


    glEnable(GL_DEPTH_TEST);
    ///// Sprites
    glDepthFunc(GL_LESS);
    shader_use(spriteShader);
    sprite_draw_all();



    glDisable(GL_DEPTH_TEST);
    //// DEBUG
    if (debugDrawPhysics)
        gameobject_draw_debugs();

    ////// TEXT && GUI
    glBindBuffer(GL_UNIFORM_BUFFER, projUBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, 64, ui_projection);

    call_gui();

    nuke_start();
    call_nk_gui();
    nuke_end();
}

void BindUniformBlock(GLuint shaderID, const char *bufferName, GLuint bufferBind)
{
    glUniformBlockBinding(shaderID, glGetUniformBlockIndex(shaderID, bufferName), bufferBind);
}

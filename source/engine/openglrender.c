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
#include "nuke.h"

int renderMode = LIT;

struct shader *spriteShader = NULL;
struct shader *wireframeShader = NULL;
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

struct sprite *tsprite = NULL;

void debug_draw_phys(int draw) {
    debugDrawPhysics = draw;
}

void opengl_rendermode(enum RenderMode r)
{
  renderMode = r;
}

sg_pipeline mainpip;
sg_pass_action pass_action = {0};

void openglInit()
{
    if (!mainwin) {
        YughError("No window to init OpenGL on.", 1);
        exit(1);
    }

    ////// MAKE SHADERS
    spriteShader = MakeShader("spritevert.glsl", "spritefrag.glsl");
    wireframeShader = MakeShader("spritevert.glsl", "spritewireframefrag.glsl");
    animSpriteShader = MakeShader("animspritevert.glsl", "animspritefrag.glsl");
    textShader = MakeShader("textvert.glsl", "textfrag.glsl");

    font_init(textShader);
    sprite_initialize();
    debugdraw_init();

    glClearColor(editorClearColor[0], editorClearColor[1], editorClearColor[2], editorClearColor[3]);

/*
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
*/
}


static cpBody *camera = NULL;
void set_cam_body(cpBody *body) {
    camera = body;
}

cpVect cam_pos() {
    return camera ? cpBodyGetPosition(camera) : cpvzero;
}
static float zoom = 1.f;
float cam_zoom() { return zoom; }


void add_zoom(float val) { zoom = val; }

mfloat_t projection[16] = {0.f};

void openglRender(struct window *window)
{
    sg_begin_default_pass(&pass_action, window->width, window->height);

    //////////// 2D projection
    cpVect pos = cam_pos();

     mat4_ortho(projection, pos.x - zoom*window->width/2,
	   pos.x + zoom*window->width/2,
	   pos.y - zoom*window->height/2,
	   pos.y + zoom*window->height/2, -1.f, 1.f);

    mfloat_t ui_projection[16] = { 0.f };
    mat4_ortho(ui_projection, 0,
        window->width,
        0,
        window->height, -1.f, 1.f);

//    sprite_draw_all();
//    gui_draw_img("pill1.png", 200, 200);
    float a[2] = {100,100};
    float w[3] = {1.f,1.f,1.f};
    renderText("TEST RENDER", a, 1.f, w, 0,-1);
    
    float b[2] = {50,50};
    renderText("TEST 2 RENDER", b, 1.f, w, 0,-1);

    /* UI Elements & Debug elements */
//    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
//    glDisable(GL_DEPTH_TEST);
    //// DEBUG
//    if (debugDrawPhysics)
//    gameobject_draw_debugs();

//    call_debugs();

    ////// TEXT && GUI
//    glBindBuffer(GL_UNIFORM_BUFFER, projUBO);
//    glBufferSubData(GL_UNIFORM_BUFFER, 0, 64, ui_projection);
      text_flush();

//    call_gui();

//    nuke_start();
//    call_nk_gui();
//    nuke_end();


  sg_end_pass();
  sg_commit();
}

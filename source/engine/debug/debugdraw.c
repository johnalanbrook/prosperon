#include "debugdraw.h"

#include "openglrender.h"

#include "shader.h"
#include "log.h"
#include <assert.h>
#include "debug.h"

#include "stb_ds.h"

static uint32_t circleVBO;
static uint32_t circleVAO;
static struct shader *circleShader;

static uint32_t gridVBO;
static uint32_t gridVAO;
static struct shader *gridShader;

static uint32_t rectVBO;
static uint32_t rectVAO;
static struct shader *rectShader;

void debugdraw_init()
{
    circleShader = MakeShader("circlevert.glsl", "circlefrag.glsl");
    shader_setUBO(circleShader, "Projection", 0);
    glGenBuffers(1, &circleVBO);
    glGenVertexArrays(1, &circleVAO);


    float gridverts[] = {
	-1.f, -1.f,
	1.f, -1.f,
	-1.f, 1.f,
	1.f, 1.f
    };

    gridShader = MakeShader("gridvert.glsl", "gridfrag.glsl");
    shader_setUBO(gridShader, "Projection", 0);
    glGenBuffers(1, &gridVBO);
    glGenVertexArrays(1, &gridVAO);
    glBindVertexArray(gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(gridverts), &gridverts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);

    rectShader = MakeShader("linevert.glsl", "linefrag.glsl");
    shader_setUBO(rectShader, "Projection", 0);
    glGenBuffers(1, &rectVBO);
    glGenVertexArrays(1, &rectVAO);
}

void draw_line(int x1, int y1, int x2, int y2, float *color)
{
    shader_use(rectShader);
    float verts[] = {
	x1, y1,
	x2, y2
    };

    draw_poly(verts, 2, color);
}

void draw_edge(cpVect  *points, int n, float *color)
{
    static_assert(sizeof(cpVect) == 2*sizeof(float));

    shader_use(rectShader);
    shader_setvec3(rectShader, "linecolor", color);
    glBindBuffer(GL_ARRAY_BUFFER, rectVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * n * 2, points, GL_DYNAMIC_DRAW);
    glBindVertexArray(rectVAO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);

    shader_setfloat(rectShader, "alpha", 1.f);
    glDrawArrays(GL_LINE_STRIP, 0, n);
}

void draw_circle(int x, int y, float radius, int pixels, float *color, int fill)
{
    shader_use(circleShader);

    float verts[] = {
	x - radius, y - radius, -1, -1,
	x + radius, y - radius, 1, -1,
	x - radius, y + radius, -1, 1,
	x + radius, y + radius, 1, 1
    };

    glBindBuffer(GL_ARRAY_BUFFER, circleVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);

    shader_setfloat(circleShader, "radius", radius);
    shader_setint(circleShader, "thickness", pixels);
    shader_setvec3(circleShader, "dbgColor", color);
    shader_setbool(circleShader, "fill", fill);

    glBindVertexArray(circleVAO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, NULL);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void draw_rect(int x, int y, int w, int h, float *color)
{
    float hw = w / 2.f;
    float hh = h / 2.f;

    float verts[] = {
	x - hw, y - hh,
	x + hw, y - hh,
	x + hw, y + hh,
	x - hw, y + hh
    };

    draw_poly(verts, 4, color);
}

void draw_grid(int width, int span)
{
    shader_use(gridShader);
    shader_setint(gridShader, "thickness", width);
    shader_setint(gridShader, "span", span);
    glBindVertexArray(gridVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);


}

void draw_point(int x, int y, float r, float *color)
{
    draw_circle(x, y, r, r, color, 0);
}

void draw_points(struct cpVect *points, int n, float size, float *color)
{
    for (int i = 0; i < n; i++)
        draw_point(points[i].x, points[i].y, size, color);
}

void draw_poly(float *points, int n, float *color)
{
    shader_use(rectShader);
    shader_setvec3(rectShader, "linecolor", color);
    glBindBuffer(GL_ARRAY_BUFFER, rectVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * n * 2, points, GL_DYNAMIC_DRAW);
    glBindVertexArray(rectVAO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);


    shader_setfloat(rectShader, "alpha", 0.1f);
    glDrawArrays(GL_POLYGON, 0, n);

    shader_setfloat(rectShader, "alpha", 1.f);
    glDrawArrays(GL_LINE_LOOP, 0, n);
}

void draw_polyvec(cpVect *points, int n, float *color)
{
    float drawvec[n*2];
    for (int i = 0; i < n; i++) {
        drawvec[i*2] = points[i].x;
        drawvec[i*2+1] = points[i].y;
    }

    draw_poly(drawvec, n, color);
}

void debugdraw_flush()
{

}

#include "debugdraw.h"

#include "openglrender.h"

#include "shader.h"

static uint32_t circleVBO;
static uint32_t circleVAO;
static struct mShader *circleShader;

static uint32_t gridVBO;
static uint32_t gridVAO;
static struct mShader *gridShader;

static uint32_t rectVBO;
static uint32_t rectVAO;
static struct mShader *rectShader;

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
    glBufferData(GL_ARRAY_BUFFER, sizeof(gridverts), &gridverts,
		 GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);

    rectShader = MakeShader("linevert.glsl", "linefrag.glsl");
    shader_setUBO(rectShader, "Projection", 0);
    glGenBuffers(1, &rectVBO);
    glGenVertexArrays(1, &rectVAO);
}

void draw_line(int x1, int y1, int x2, int y2)
{
    shader_use(rectShader);
    float verts[] = {
	x1, y1,
	x2, y2
    };

    glBindBuffer(GL_ARRAY_BUFFER, rectVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), &verts, GL_DYNAMIC_DRAW);
    glBindVertexArray(rectVAO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glDrawArrays(GL_LINE_STRIP, 0, 2);
}

void draw_edge(float *points, int n)
{
    shader_use(rectShader);
    glBindBuffer(GL_ARRAY_BUFFER, rectVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * n * 2, points,
		 GL_DYNAMIC_DRAW);
    glBindVertexArray(rectVAO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glDrawArrays(GL_LINE_STRIP, 0, n);
}

void draw_circle(int x, int y, float radius, int pixels)
{
    shader_use(circleShader);

    float verts[] = {
	x - radius, y - radius, -1.f, -1.f,
	x + radius, y - radius, 1.f, -1.f,
	x - radius, y + radius, -1.f, 1.f,
	x + radius, y + radius, 1.f, 1.f
    };

    glBindBuffer(GL_ARRAY_BUFFER, circleVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), &verts, GL_DYNAMIC_DRAW);

    shader_setfloat(circleShader, "radius", radius);
    shader_setint(circleShader, "thickness", pixels);

    glBindVertexArray(circleVAO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, NULL);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void draw_rect(int x, int y, int w, int h)
{
    float hw = w / 2.f;
    float hh = h / 2.f;

    float verts[] = {
	x - hw, y - hh,
	x + hw, y - hh,
	x + hw, y + hh,
	x - hw, y + hh
    };

    shader_use(rectShader);
    glBindBuffer(GL_ARRAY_BUFFER, rectVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), &verts, GL_DYNAMIC_DRAW);
    glBindVertexArray(rectVAO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glDrawArrays(GL_LINE_LOOP, 0, 4);
}

void draw_grid(int width, int span)
{
    shader_use(gridShader);
    shader_setint(gridShader, "thickness", width);
    shader_setint(gridShader, "span", span);
    glBindVertexArray(gridVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);


}

void draw_point(int x, int y, float r)
{
    draw_circle(x, y, r, r);
}

void draw_poly(float *points, int n)
{
    shader_use(rectShader);
    glBindBuffer(GL_ARRAY_BUFFER, rectVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * n * 2, points,
		 GL_DYNAMIC_DRAW);
    glBindVertexArray(rectVAO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glDrawArrays(GL_LINE_LOOP, 0, n);
}

void debugdraw_flush()
{

}

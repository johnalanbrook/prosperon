#include "debugdraw.h"

#include "openglrender.h"

#include "shader.h"
#include "log.h"
#include <assert.h>
#include "debug.h"
#include "window.h"
#include "2dphysics.h"
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
    shader_setUBO(circleShader, "Resolution", 0);
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

cpVect center_of_vects(cpVect *v, int n)
{
  cpVect c;
  for (int i = 0; i < n; i++) {
    c.x += v[i].x;
    c.y += v[i].y;
  }

  c.x /= n;
  c.y /= n;
  return c;
}

float vecs2m(cpVect a, cpVect b)
{
  return (b.y-a.y)/(b.x-a.x);
}

cpVect inflateline(cpVect a, cpVect b, float d)
{
  cpVect c;
  float m = vecs2m(a, b);
  c.x = d/sqrt(1/(pow(m,2)+1));
  c.y = d/sqrt(1+pow(m,2));
  return c;
}

cpVect inflatepoint(cpVect a, cpVect b, cpVect c, float d)
{
  cpVect ba = cpvnormalize(cpvsub(a,b));
  cpVect bc = cpvnormalize(cpvsub(c,b));
  cpVect avg = cpvadd(ba, bc);
  avg = cpvmult(avg, 0.5);
  float dot = cpvdot(ba, bc);
  dot /= cpvlength(ba);
  dot /= cpvlength(bc);
  float mid = acos(dot)/2;
  avg = cpvnormalize(avg);
  return cpvadd(b, cpvmult(avg, d/sin(mid)));
}

void inflatepoints(cpVect *r, cpVect *p, float d, int n)
{    if (d == 0) {
       for (int i = 0; i < n; i++)
         r[i] = p[i];

       return;
     }
     
     if (cpveql(p[0], p[n-1])) {
        r[0] = inflatepoint(p[n-2],p[0],p[1],d);
	r[n-1] = r[0];
      } else {
        cpVect outdir = cpvmult(cpvnormalize(cpvsub(p[0],p[1])),fabs(d));
	cpVect perp;
	if (d > 0)
          perp = cpvperp(outdir);
	else
	  perp = cpvrperp(outdir);
        r[0] = cpvadd(p[0],cpvadd(outdir,perp));
	
	outdir = cpvmult(cpvnormalize(cpvsub(p[n-1],p[n-2])),fabs(d));
	if (d > 0)
          perp = cpvrperp(outdir);
	else
	  perp = cpvperp(outdir);
        r[n-1] = cpvadd(p[n-1],cpvadd(outdir,perp));
      }

      for (int i = 0; i < n-2; i++)
        r[i+1] = inflatepoint(p[i],p[i+1],p[i+2], d);

}

void draw_edge(cpVect  *points, int n, float *color, int thickness)
{
    static_assert(sizeof(cpVect) == 2*sizeof(float));

    shader_use(rectShader);
    shader_setvec3(rectShader, "linecolor", color);

    if (thickness <= 1) {
      shader_setfloat(rectShader, "alpha", 1.f);
      glBindBuffer(GL_ARRAY_BUFFER, rectVBO);
      glBufferData(GL_ARRAY_BUFFER, sizeof(float) * n * 2, points, GL_DYNAMIC_DRAW);
      glBindVertexArray(rectVAO);
      glEnableVertexAttribArray(0);
      glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);

      shader_setfloat(rectShader, "alpha", 1.f);
      glDrawArrays(GL_LINE_STRIP, 0, n);
    } else {
      shader_setfloat(rectShader, "alpha", 0.1f);
      thickness /= 2;
      glBindBuffer(GL_ARRAY_BUFFER, rectVBO);
      glBindVertexArray(rectVAO);
      glEnableVertexAttribArray(0);
      glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);

      cpVect inflate_out[n];
      cpVect inflate_in[n];

      inflatepoints(inflate_out, points, thickness, n);
      inflatepoints(inflate_in, points, -thickness, n);
      cpVect interleaved[n*2];
      for (int i = 0; i < n; i++) {
        interleaved[i*2] = inflate_in[i];
	interleaved[i*2+1] = inflate_out[i];
      }
      glBufferData(GL_ARRAY_BUFFER, sizeof(interleaved), interleaved, GL_DYNAMIC_DRAW);
      glDrawArrays(GL_TRIANGLE_STRIP, 0, n*2);

      shader_setfloat(rectShader, "alpha", 1.f);

      cpVect inlined[n*2];
      for (int i = 0; i < n; i++)
        inlined[i] = inflate_out[n-1-i];
      memcpy(inlined+n,inflate_in,sizeof(inflate_in));
      glBufferData(GL_ARRAY_BUFFER,sizeof(inlined),inlined,GL_DYNAMIC_DRAW);
      glDrawArrays(GL_LINE_LOOP,0,n*2);
      
    }
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
    shader_setfloat(circleShader, "zoom", cam_zoom());

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

void draw_box(struct cpVect c, struct cpVect wh, struct color color)
{
  float col[3] = {(float)color.r/255, (float)color.g/255, (float)color.b/255};
  draw_rect(c.x, c.y, wh.x, wh.y, col);
}

void draw_arrow(struct cpVect start, struct cpVect end, struct color color)
{
  float col[3] = {(float)color.r/255, (float)color.g/255, (float)color.b/255};
  draw_line(start.x, start.y, end.x, end.y, col);
  
  draw_cppoint(end, 5, color);
}

void draw_grid(int width, int span)
{
    shader_use(gridShader);
    shader_setint(gridShader, "thickness", width);
    shader_setint(gridShader, "span", span);
    
    cpVect offset = cam_pos();
        offset = cpvmult(offset, 1/cam_zoom());
    offset.x -= mainwin->width/2;
    offset.y -= mainwin->height/2;

    shader_setvec2(gridShader, "offset", &offset);
    
    glBindVertexArray(gridVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void draw_point(int x, int y, float r, float *color)
{
    draw_circle(x,y,r,r,color,1);
}

void draw_cppoint(struct cpVect point, float r, struct color color)
{
    float col[3] = {(float)color.r/255, (float)color.g/255, (float)color.b/255};
    draw_point(point.x, point.y, r, col);
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

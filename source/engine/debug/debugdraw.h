#ifndef DEBUGDRAW_H
#define DEBUGDRAW_H

#include <chipmunk/chipmunk.h>
struct color;

void debugdraw_init();
void draw_line(int x1, int y1, int x2, int y2, float *color);
void draw_edge(struct cpVect *points, int n, struct color color, int thickness);
void draw_points(struct cpVect *points, int n, float size, float *color);
void draw_arrow(struct cpVect start, struct cpVect end, struct color, int capsize);
void draw_circle(int x, int y, float radius, int pixels, float *color, int fill);
void draw_grid(int width, int span);
void draw_rect(int x, int y, int w, int h, float *color);
void draw_box(struct cpVect c, struct cpVect wh, struct color color);
void draw_point(int x, int y, float r, float *color);
void draw_cppoint(struct cpVect point, float r, struct color color);
void draw_poly(float *points, int n, float *color);


void debugdraw_flush();		/* This is called once per frame to draw all queued elements */

cpVect inflatepoint(cpVect a, cpVect b, cpVect c, float d);
void inflatepoints(cpVect *r, cpVect *p, float d, int n);

#endif

#ifndef DEBUGDRAW_H
#define DEBUGDRAW_H

#include <chipmunk/chipmunk.h>
struct rgba;

void debugdraw_init();
void draw_cppoint(struct cpVect point, float r, struct rgba color);
void draw_points(struct cpVect *points, int n, float size, struct rgba color);

void draw_line(cpVect *points, int n, struct rgba color, float seg_len);
void draw_arrow(struct cpVect start, struct cpVect end, struct rgba, int capsize);
void draw_edge(struct cpVect *points, int n, struct rgba color, int thickness, int closed, int flags);

void draw_circle(int x, int y, float radius, int pixels, struct rgba color, int fill);

void draw_rect(int x, int y, int w, int h, struct rgba color);
void draw_box(struct cpVect c, struct cpVect wh, struct rgba color);
void draw_poly(cpVect *points, int n, struct rgba color);

void draw_grid(int width, int span, struct rgba color);

void debug_flush();

void debugdraw_flush();		/* This is called once per frame to draw all queued elements */

cpVect inflatepoint(cpVect a, cpVect b, cpVect c, float d);
void inflatepoints(cpVect *r, cpVect *p, float d, int n);

#endif

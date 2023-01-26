#ifndef DEBUGDRAW_H
#define DEBUGDRAW_H

struct cpVect;

void debugdraw_init();
void draw_line(int x1, int y1, int x2, int y2, float *color);
void draw_edge(struct cpVect *points, int n, float *color);
void draw_points(struct cpVect *points, int n, float size, float *color);
void draw_circle(int x, int y, float radius, int pixels, float *color, int fill);
void draw_grid(int width, int span);
void draw_rect(int x, int y, int w, int h, float *color);
void draw_point(int x, int y, float r, float *color);
void draw_poly(float *points, int n, float *color);


void debugdraw_flush();		/* This is called once per frame to draw all queued elements */


#endif

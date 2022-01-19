#ifndef DEBUGDRAW_H
#define DEBUGDRAW_H

void debugdraw_init();
void draw_line(int x1, int y1, int x2, int y2);
void draw_edge(float *points, int n);
void draw_circle(int x, int y, float radius, int pixels);
void draw_grid(int width, int span);
void draw_rect(int x, int y, int w, int h);
void draw_point(int x, int y, float r);
void draw_poly(float *points, int n);

void debugdraw_flush();		/* This is called once per frame to draw all queued elements */


#endif

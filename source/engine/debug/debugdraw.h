#ifndef DEBUGDRAW_H
#define DEBUGDRAW_H

#include "HandmadeMath.h"
struct rgba;

void debugdraw_init();
void draw_cppoint(HMM_Vec2 point, float r, struct rgba color);
void draw_points(HMM_Vec2 *points, int n, float size, struct rgba color);

void draw_line(HMM_Vec2 *points, int n, struct rgba color, float seg_len, float seg_speed);
void draw_edge(HMM_Vec2 *points, int n, struct rgba color, float thickness, int flags, struct rgba line_color, float line_seg);

/* pixels - how many pixels thick, segsize - dashed line seg len */
void draw_circle(HMM_Vec2 c, float radius, float pixels, struct rgba color, float seg);
void draw_box(HMM_Vec2 c, HMM_Vec2 wh, struct rgba color);
void draw_poly(HMM_Vec2 *points, int n, struct rgba color);

void draw_grid(float width, float span, struct rgba color);

void debug_flush(HMM_Mat4 *view);
void debug_newframe();
void debug_nextpass();

HMM_Vec2 *inflatepoints(HMM_Vec2 *p, float d, int n);

#endif

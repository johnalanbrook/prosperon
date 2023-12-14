#ifndef SPLINE_H
#define SPLINE_H

#include "HandmadeMath.h"

HMM_Vec2 *catmull_rom_ma_v2(HMM_Vec2 *cp, float ma);
HMM_Vec3 *catmull_rom_ma_v3(HMM_Vec3 *cp, float ma);
HMM_Vec4 *catmull_rom_ma_v4(HMM_Vec4 *cp, float ma);

HMM_Vec2 *bezier_cb_ma_v2(HMM_Vec2 *cp, float ma);
HMM_Vec2 spline_query(HMM_Vec2 *cp, float d, HMM_Mat4 *basis);

HMM_Vec2 catmull_rom_pos(HMM_Vec2 *cp, float d);
HMM_Vec2 catmull_rom_tan(HMM_Vec2 *cp, float d);
HMM_Vec2 catmull_rom_curv(HMM_Vec2 *cp, float d);
HMM_Vec2 catmull_rom_wig(HMM_Vec2 *cp, float d);

float catmull_rom_len(HMM_Vec2 *cp);

/* Returns closest point on a curve given a point p */
HMM_Vec2 catmull_rom_closest(HMM_Vec2 *cp, HMM_Vec2 p);

#endif

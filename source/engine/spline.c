#include "spline.h"
#include "stb_ds.h"
#include "log.h"
#include "transform.h"
#include "math.h"

static const HMM_Mat4 cubic_hermite_m = {
  2, -2, 1, 1,
  -3, 3, -2, -1,
  0, 0, 1, 0,
  1, 0, 0, 0
};

static const HMM_Mat4 cubic_hermite_dm = {
  0, 0, 0, 0,
  6, -6, 3, 3,
  -6, 6, -4, -2,
  0, 0, 1, 0
};

static const HMM_Mat4 cubic_hermite_ddm = {
  0, 0, 0, 0,
  0, 0, 0, 0,
  12, -12, 6, 6,
  -6, 6, -4, -2
};

static const HMM_Mat4 cubic_hermite_dddm = {
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,
  12, -12, 6, 6
};

static const HMM_Mat4 b_spline_m = {
  -1/6, 3/6, -3/6, 1,
  3/6, -6/6, 3/6, 0,
  -3/6, 0, 3/6, 0,
  1/6, 4/6, 1/6, 0
};

static const HMM_Mat4 b_spline_dm = {
  0, 0, 0, 0,
  -3/6, 9/6, -9/6, 3,
  6/6, -12/6, 6/6, 0,
  -3/6, 0, 3/6, 0
};

static const HMM_Mat4 b_spline_ddm = {
  0, 0, 0, 0,
  0, 0, 0, 0,
  -6/6, 18/6, -18/6, 6,
  6/6, -12/6, 6/6, 0
};

static const HMM_Mat4 b_spline_dddm = {
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,
  -6/6, 18/6, -18/6, 6
};

static const HMM_Mat4 bezier_m = {
  -1, 3, -3, 1,
  3, -6, 3, 0,
  -3, 3, 0, 0,
  1, 0, 0, 0
};

static const HMM_Mat4 bezier_dm = {
  0, 0, 0, 0,
  -3, 9, -9, 3,
  6, -12, 6, 0,
  -3, 3, 0, 0,
};

static const HMM_Mat4 bezier_ddm = {
  0, 0, 0, 0,
  0, 0, 0, 0,
  -6, 18, -18, 6,
  6, -12, 6, 0
};

static const HMM_Mat4 bezier_dddm = {
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,
  -6, 18, -18, 6
};

#define CAT_S 0.5

/* Position */
static const HMM_Mat4 catmull_rom_m = {
  -CAT_S, 2-CAT_S, CAT_S-2, CAT_S,
  2*CAT_S, CAT_S-3, 3-2*CAT_S, -CAT_S,
  -CAT_S, 0, CAT_S, 0,
  0, 1, 0, 0
};

/* Tangent */
static const HMM_Mat4 catmull_rom_dm = {
  0, 0, 0, 0,
  -3*CAT_S, 9*CAT_S, -9*CAT_S, 3*CAT_S,
  4*CAT_S, -10*CAT_S, 8*CAT_S, -2*CAT_S,
  -CAT_S, 0, CAT_S, 0,
};

/* Curvature */
static const HMM_Mat4 catmull_rom_ddm = {
  0, 0, 0, 0,
  0, 0, 0, 0,
  -9*CAT_S, 18*CAT_S, -18*CAT_S, 6*CAT_S,
  4*CAT_S, -10*CAT_S, 8*CAT_S, -2*CAT_S
};

/* Wiggle */
static const HMM_Mat4 catmull_rom_dddm = {
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,
  -9*CAT_S, 18*CAT_S, -18*CAT_S, 6*CAT_S
};

/*
  [t3 t2 t1 1] B [p1
      	          p2  that is, point 1, tangent at point 1, point 2, tan and point 2
		  t1
		  t2]

*/

HMM_Vec4 spline_CT(HMM_Mat4 *C, float t)
{
  float t2 = t*t;
  float t3 = t2*t;
  HMM_Vec4 T = {t3, t2, t, 1};
  return HMM_MulM4V4(*C, T);
}

HMM_Mat4 make_C(HMM_Vec2 *p, HMM_Mat4 *B)
{
  HMM_Mat4 G;
  G.Columns[0].xy = p[0];
  G.Columns[1].xy = p[1];
  G.Columns[2].xy = p[2];
  G.Columns[3].xy = p[3];
  return HMM_MulM4(G, *B);
}

HMM_Vec2 cubic_spline_d(HMM_Vec2 *p, HMM_Mat4 *m, float d)
{
  HMM_Mat4 C = make_C(p, m);
  return spline_CT(&C, d).xy;
}

HMM_Vec2 *spline_v2(HMM_Vec2 *p, HMM_Mat4 *m, int segs)
{
  HMM_Vec2 *ret = NULL;
  if (segs < 2) return NULL;
 
  HMM_Mat4 C = make_C(p, m);
  float s = (float)1/segs;

  for (float t = 0; t < 1; t += s)
    arrput(ret, spline_CT(&C, t).xy);

  return ret;
}

HMM_Vec2 *spline2d_min_seg(float u0, float u1, float min_seg, HMM_Mat4 *C, HMM_Vec2 *ret)
{
  HMM_Vec2 a = spline_CT(C, u0).xy;
  HMM_Vec2 b = spline_CT(C, u1).xy;
  if (HMM_DistV2(a,b) > min_seg) {
    float umid = (u0+u1)/2;
    spline2d_min_seg(u0, umid, min_seg, C, ret);
    spline2d_min_seg(umid, u1, min_seg, C, ret);
  }
  else
    arrput(ret, b);
}

HMM_Vec2 *catmull_rom_min_seg(HMM_Vec2 *a, HMM_Vec2 *b, HMM_Vec2 *c, HMM_Vec2 *d, float min_seg)
{
  HMM_Vec2 *ret = NULL;
  arrsetcap(ret, 1000);
  arrput(ret, *b);
//  spline2d_min_seg(0, 1, min_seg, &C, ret);
  return ret;
}

HMM_Vec2 *spline2d_min_angle_2(float u0, float u1, float max_angle, HMM_Mat4 *C, HMM_Vec2 *arr) 
{
  float ustep = (u1-u0)/4;
  float um0 = u0+ustep;
  float um1 = u0+(ustep*2);
  float um2 = u0+(ustep*3);
  
  HMM_Vec2 m0 = spline_CT(C, um0)._2;
  HMM_Vec2 m1 = spline_CT(C, um1)._2;
  HMM_Vec2 m2 = spline_CT(C,um2)._2;

  HMM_Vec2 a = spline_CT(C,u0)._2;
  HMM_Vec2 b = spline_CT(C,u1)._2;
  
  float ab = HMM_DistV2(a,b);
  float cdist = HMM_DistV2(a,m0) + HMM_DistV2(m0,m1) + HMM_DistV2(m1,m2) + HMM_DistV2(m2,b);

  if (cdist-ab > max_angle) {
    arr = spline2d_min_angle_2(u0,um1,max_angle,C,arr);
    arr = spline2d_min_angle_2(um1,u1,max_angle,C,arr);    
  } else
    arrput(arr,b);
  
  return arr;
}

HMM_Vec2 *spline_min_angle(HMM_Vec2 *p, HMM_Mat4 *B, float min_angle, HMM_Vec2 *arr)
{
  HMM_Mat4 C = make_C(p, B);
  arr = spline2d_min_angle_2(0,1,min_angle, &C, arr);
  return arr;
}

HMM_Vec2 *catmull_rom_ma_v2(HMM_Vec2 *cp, float ma)
{
  if (arrlen(cp) < 4) return NULL;
  HMM_Vec2 *ret = NULL;

  int segments = arrlen(cp)-3;
  arrsetcap(ret,segments*(ma>=2 ? 3 : 7));  
  arrput(ret, cp[1]); 
  for (int i = 0; i < arrlen(cp)-3; i++)
    ret = spline_min_angle(&cp[i], &catmull_rom_m, ma, ret);

  return ret;
}

HMM_Vec2 *bezier_cb_ma_v2(HMM_Vec2 *cp, float ma)
{
  if (arrlen(cp) < 4) return NULL;
  HMM_Vec2 *ret = NULL;
  int segments = arrlen(cp)-3;
  arrsetcap(ret,segments*(ma>=2?3:7));
  arrput(ret,cp[0]);
  for (int i = 0; i < arrlen(cp)-3; i+=3)
    ret = spline_min_angle(&cp[i], &bezier_m, ma, ret);

  return ret;
}

HMM_Vec2 catmull_rom_query(HMM_Vec2 *cp, float d, HMM_Mat4 *G)
{
  if (arrlen(cp) < 4 || d < 0 || d > 1) return HMM_V2(0,0);

  int segs = arrlen(cp)-3;
  float d_per_seg = (float)1/segs;
  float maxi = d_per_seg;
  int p1 = 2;
  while (maxi < d) {
    maxi += d_per_seg;
    p1++;
  }

   return cp[0];
//  return cubic_spline_d(p0, cp[p1], cp[p1+1], p3, G, d);
}

float spline_seglen(float t0, float t1, float max_angle, HMM_Mat4 *Cd, HMM_Mat4 *C)
{
  float total = 0;
  float step = 0.1;
  for (float i = t0; i < t1; i += step)
    total += HMM_LenV2(spline_CT(Cd, i).xy) * step;

  return total;

  /* Estimation via max angle */
/*  float total = 0.0;
  float tmid = (t0+t1)/2;
  HMM_Vec2 a = spline_CT(C, t0).xy;
  HMM_Vec2 b = spline_CT(C, t1).xy;
  HMM_Vec2 m = spline_CT(C, tmid).xy;

  if (HMM_AngleV2(m,b) > max_angle) {
    total += spline_seglen(t0, tmid, max_angle, Cd, C);
    total += spline_seglen(tmid, t1, max_angle, Cd, C);
  } else
   return HMM_LenV2(spline_CT(Cd, t0).xy)*(t1-t0);

  return total;
*/
}

float catmull_rom_len(HMM_Vec2 *cp)
{
  float len = 0.0;
  int segs = arrlen(cp)-3;
  float d_per_seg = (float)1/segs;
  float maxi = d_per_seg;
  for (int i = 0; i < arrlen(cp)-3; i++) {
    HMM_Mat4 C = make_C(&cp[i], &catmull_rom_m);
    HMM_Mat4 Cd = make_C(&cp[i], &catmull_rom_dm);
    len += spline_seglen(0, 1, 0.1, &Cd, &C);
  }
  return len;
}

/* d is from 0 to 1 for the entire spline */
HMM_Vec2 catmull_rom_pos(HMM_Vec2 *cp, float d) { return catmull_rom_query(cp,d,&catmull_rom_m); }
HMM_Vec2 catmull_rom_tan(HMM_Vec2 *cp, float d) { return catmull_rom_query(cp,d,&catmull_rom_dm); }
HMM_Vec2 catmull_rom_curv(HMM_Vec2 *cp, float d) { return catmull_rom_query(cp,d,&catmull_rom_ddm); }
HMM_Vec2 catmull_rom_wig(HMM_Vec2 *cp, float d) { return catmull_rom_query(cp,d,&catmull_rom_dddm); }

HMM_Vec2 catmull_rom_closest(HMM_Vec2 *cp, HMM_Vec2 p)
{

}

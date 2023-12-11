#include "spline.h"
#include "stb_ds.h"
#include "log.h"
#include "transform.h"

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

HMM_Vec4 spline_CT(HMM_Mat4 *C, float t)
{
  float t2 = t*t;
  float t3 = t2*t;
  HMM_Vec4 T = {t3, t2, t, 1};
  return HMM_MulM4V4(*C, T);
}

HMM_Mat4 make_G(HMM_Vec2 a, HMM_Vec2 b, HMM_Vec2 c, HMM_Vec2 d)
{
  HMM_Mat4 G;
  G.Columns[0].xy = a;
  G.Columns[1].xy = b;
  G.Columns[2].xy = c;
  G.Columns[3].xy = d;
  return G;
}

HMM_Mat4 make_C(HMM_Vec2 p0, HMM_Vec2 p1, HMM_Vec2 p2, HMM_Vec2 p3, HMM_Mat4 *B)
{
  HMM_Mat4 G = make_G(p0, p1, p2, p3);
  return HMM_MulM4(G, *B);
}

HMM_Vec2 cubic_spline_d(HMM_Vec2 p0, HMM_Vec2 p1, HMM_Vec2 p2, HMM_Vec2 p3, HMM_Mat4 *m, float d)
{
  HMM_Mat4 G = make_G(p0,p1,p2,p3);
  HMM_Mat4 C = HMM_MulM4(G, *m);
  return spline_CT(&C, d).xy;
}

HMM_Vec2 *spline_v2(HMM_Vec2 *a, HMM_Vec2 *b, HMM_Vec2 *c, HMM_Vec2 *d, HMM_Mat4 *m, int segs)
{
  HMM_Vec2 *ret = NULL;
  if (segs == 2) {
    arrput(ret, *b);
    arrput(ret, *c);
    return ret;
  }
  if (segs < 2) return NULL;

  HMM_Mat4 G = make_G(*a, *b, *c, *d);
  HMM_Mat4 C = HMM_MulM4(G, *m);
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
  HMM_Vec2 p0 = HMM_MulV2F(HMM_SubV2(*c, *a), CAT_S);
  HMM_Vec2 p3 = HMM_MulV2F(HMM_SubV2(*d, *b), CAT_S);

  HMM_Mat4 G = make_G(p0, *b, *c, p3);
  HMM_Mat4 C = HMM_MulM4(G, catmull_rom_m);
  HMM_Vec2 *ret = NULL;
  arrsetcap(ret, 100);
  arrput(ret, *b);
  spline2d_min_seg(0, 1, min_seg, &C, ret);
  return ret;
}

void *stbarrdup(void *mem, size_t size, int len) {
  void *out = NULL;
  arrsetlen(out, len);
  memcpy(out,mem,size*len);
  return out;
}

#define arrconcat(a,b) do{for (int i = 0; i < arrlen(b); i++) arrput(a,b[i]);}while(0)
#define arrdup(a) (stbarrdup(a, sizeof(*a), arrlen(a)))

static HMM_Vec2 *V2RET = NULL;
static HMM_Vec3 *V3RET = NULL;
static HMM_Vec4 *V4RET = NULL;

#define SPLINE_MIN(DIM) \
HMM_Vec##DIM *spline2d_min_angle_##DIM(float u0, float u1, float max_angle, HMM_Mat4 *C) \
{ \
  float umid = (u0 + u1)/2;\
  HMM_Vec##DIM a = spline_CT(C, u0)._##DIM;\
  HMM_Vec##DIM b = spline_CT(C, u1)._##DIM;\
  HMM_Vec##DIM m = spline_CT(C, umid)._##DIM;\
  if (HMM_AngleV##DIM(m,b) > max_angle) {\
    spline2d_min_angle_##DIM(u0, umid, max_angle, C);\
    spline2d_min_angle_##DIM(umid, u1, max_angle, C);\
  }\
  else\
    arrput(V##DIM##RET,b);\
}\

SPLINE_MIN(2)
SPLINE_MIN(3)

/* Computes non even points to give the best looking curve */
HMM_Vec2 *catmull_rom_min_angle(HMM_Vec2 *a, HMM_Vec2 *b, HMM_Vec2 *c, HMM_Vec2 *d, float min_angle)
{
  HMM_Mat4 G = make_G(*a, *b, *c, *d);
  HMM_Mat4 C = HMM_MulM4(G, catmull_rom_m);
  return spline2d_min_angle_2(0,1,min_angle*M_PI/180.0,&C);
}

#define CR_MA(DIM) \
HMM_Vec##DIM *catmull_rom_ma_v##DIM(HMM_Vec##DIM *cp, float ma) \
{ \
  if (arrlen(cp) < 4) return NULL; \
\
  if (V##DIM##RET) arrfree(V##DIM##RET);\
  arrsetcap(V##DIM##RET,100);\
  int segments = arrlen(cp)-3;\
  arrput(V##DIM##RET, cp[1]); \
  for (int i = 1; i < arrlen(cp)-2; i++) { \
    HMM_Vec##DIM p0 = HMM_MulV##DIM##F(HMM_SubV##DIM(cp[i+1], cp[i-1]), CAT_S);\
    HMM_Vec##DIM p3 = HMM_MulV##DIM##F(HMM_SubV##DIM(cp[i+2], cp[i]), CAT_S);\
    catmull_rom_min_angle(&p0, &cp[i], &cp[i+1], &p3, ma);\
  }\
  \
  return arrdup(V##DIM##RET);\
}\

CR_MA(2)
CR_MA(3)
CR_MA(4)

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

  HMM_Vec2 p0 = HMM_MulV2F(HMM_SubV2(cp[p1+1], cp[p1-1]), CAT_S);
  HMM_Vec2 p3 = HMM_MulV2F(HMM_SubV2(cp[p1+2], cp[p1]), CAT_S);

  return cubic_spline_d(p0, cp[p1], cp[p1+1], p3, G, d);
}

float catmull_rom_seglen(float t0, float t1, float max_angle, HMM_Mat4 *Cd, HMM_Mat4 *C)
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
    total += catmull_rom_seglen(t0, tmid, max_angle, Cd, C);
    total += catmull_rom_seglen(tmid, t1, max_angle, Cd, C);
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
  for (int i = 1; i < arrlen(cp)-2; i++) {
    HMM_Vec2 p0 = HMM_MulV2F(HMM_SubV2(cp[i+1], cp[i-1]), CAT_S);
    HMM_Vec2 p3 = HMM_MulV2F(HMM_SubV2(cp[i+2], cp[i]), CAT_S);
    HMM_Mat4 C = make_C(p0, cp[i], cp[i+1], p3, &catmull_rom_m);
    HMM_Mat4 Cd = make_C(p0, cp[i], cp[i+1], p3, &catmull_rom_dm);
    len += catmull_rom_seglen(0, 1, 0.1, &Cd, &C);
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

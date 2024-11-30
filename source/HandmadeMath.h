/*
  HandmadeMath.h v2.0.0

  This is a single header file with a bunch of useful types and functions for
  games and graphics. Consider it a lightweight alternative to GLM that works
  both C and C++.

  =============================================================================
  CONFIG
  =============================================================================

  By default, all angles in Handmade Math are specified in radians. However, it
  can be configured to use degrees or turns instead. Use one of the following
  defines to specify the default unit for angles:

    #define HANDMADE_MATH_USE_RADIANS
    #define HANDMADE_MATH_USE_DEGREES
    #define HANDMADE_MATH_USE_TURNS

  Regardless of the default angle, you can use the following functions to
  specify an angle in a particular unit:

    HMM_AngleRad(radians)
    HMM_AngleDeg(degrees)
    HMM_AngleTurn(turns)

  The definitions of these functions change depending on the default unit.

  -----------------------------------------------------------------------------

  Handmade Math ships with SSE (SIMD) implementations of several common
  operations. To disable the use of SSE intrinsics, you must define
  HANDMADE_MATH_NO_SSE before including this file:

    #define HANDMADE_MATH_NO_SSE
    #include "HandmadeMath.h"

  -----------------------------------------------------------------------------

  To use Handmade Math without the C runtime library, you must provide your own
  implementations of basic math functions. Otherwise, HandmadeMath.h will use
  the runtime library implementation of these functions.

  Define HANDMADE_MATH_PROVIDE_MATH_FUNCTIONS and provide your own
  implementations of HMM_SINF, HMM_COSF, HMM_TANF, HMM_ACOSF, and HMM_SQRTF
  before including HandmadeMath.h, like so:

    #define HANDMADE_MATH_PROVIDE_MATH_FUNCTIONS
    #define HMM_SINF MySinF
    #define HMM_COSF MyCosF
    #define HMM_TANF MyTanF
    #define HMM_ACOSF MyACosF
    #define HMM_SQRTF MySqrtF
    #include "HandmadeMath.h"

  By default, it is assumed that your math functions take radians. To use
  different units, you must define HMM_ANGLE_USER_TO_INTERNAL and
  HMM_ANGLE_INTERNAL_TO_USER. For example, if you want to use degrees in your
  code but your math functions use turns:

    #define HMM_ANGLE_USER_TO_INTERNAL(a) ((a)*HMM_DegToTurn)
    #define HMM_ANGLE_INTERNAL_TO_USER(a) ((a)*HMM_TurnToDeg)

  =============================================================================

  LICENSE

  This software is in the public domain. Where that dedictaion is not
  recognized, you are granted a perpetual, irrevocable license to copy,
  distribute, and modify this file as you see fit.

  =============================================================================

  CREDITS

  Originally written by Zakary Strange.

  Functionality:
   Zakary Strange (strangezak@protonmail.com && @strangezak)
   Matt Mascarenhas (@miblo_)
   Aleph
   FieryDrake (@fierydrake)
   Gingerbill (@TheGingerBill)
   Ben Visness (@bvisness)
   Trinton Bullard (@Peliex_Dev)
   @AntonDan
   Logan Forman (@dev_dwarf)

  Fixes:
   Jeroen van Rijn (@J_vanRijn)
   Kiljacken (@Kiljacken)
   Insofaras (@insofaras)
   Daniel Gibson (@DanielGibson)
*/

#ifndef HANDMADE_MATH_H
#define HANDMADE_MATH_H

#include <chipmunk/chipmunk.h>

/* let's figure out if SSE is really available (unless disabled anyway)
   (it isn't on non-x86/x86_64 platforms or even x86 without explicit SSE support)
   => only use "#ifdef HANDMADE_MATH__USE_SSE" to check for SSE support below this block! */
#ifndef HANDMADE_MATH_NO_SSE
#ifdef _MSC_VER /* MSVC supports SSE in amd64 mode or _M_IX86_FP >= 1 (2 means SSE2) */
#if defined(_M_AMD64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 1)
#define HANDMADE_MATH__USE_SSE 1
#endif
#else          /* not MSVC, probably GCC, clang, icc or something that doesn't support SSE anyway */
#ifdef __SSE__ /* they #define __SSE__ if it's supported */
#define HANDMADE_MATH__USE_SSE 1
#endif /*  __SSE__ */
#endif /* not _MSC_VER */
#endif /* #ifndef HANDMADE_MATH_NO_SSE */

#ifdef HANDMADE_MATH__USE_SSE
#include <xmmintrin.h>
#endif

#ifdef _MSC_VER
#pragma warning(disable : 4201)
#endif

#if defined(__GNUC__) || defined(__clang__)
#define HMM_DEPRECATED(msg) __attribute__((deprecated(msg)))
#elif defined(_MSC_VER)
#define HMM_DEPRECATED(msg) __declspec(deprecated(msg))
#else
#define HMM_DEPRECATED(msg)
#endif

#if !defined(HANDMADE_MATH_USE_DEGREES) && !defined(HANDMADE_MATH_USE_TURNS) && !defined(HANDMADE_MATH_USE_RADIANS)
#define HANDMADE_MATH_USE_RADIANS
#endif

#define HMM_PI 3.14159265358979323846
#define HMM_PI32 3.14159265359f
#define HMM_DEG180 180.0
#define HMM_DEG18032 180.0f
#define HMM_TURNHALF 0.5
#define HMM_TURNHALF32 0.5f
#define HMM_RadToDeg ((float)(HMM_DEG180 / HMM_PI))
#define HMM_RadToTurn ((float)(HMM_TURNHALF / HMM_PI))
#define HMM_DegToRad ((float)(HMM_PI / HMM_DEG180))
#define HMM_DegToTurn ((float)(HMM_TURNHALF / HMM_DEG180))
#define HMM_TurnToRad ((float)(HMM_PI / HMM_TURNHALF))
#define HMM_TurnToDeg ((float)(HMM_DEG180 / HMM_TURNHALF))

#if defined(HANDMADE_MATH_USE_RADIANS)
#define HMM_AngleRad(a) (a)
#define HMM_AngleDeg(a) ((a)*HMM_DegToRad)
#define HMM_AngleTurn(a) ((a)*HMM_TurnToRad)
#elif defined(HANDMADE_MATH_USE_DEGREES)
#define HMM_AngleRad(a) ((a)*HMM_RadToDeg)
#define HMM_AngleDeg(a) (a)
#define HMM_AngleTurn(a) ((a)*HMM_TurnToDeg)
#elif defined(HANDMADE_MATH_USE_TURNS)
#define HMM_AngleRad(a) ((a)*HMM_RadToTurn)
#define HMM_AngleDeg(a) ((a)*HMM_DegToTurn)
#define HMM_AngleTurn(a) (a)
#endif

#if !defined(HANDMADE_MATH_PROVIDE_MATH_FUNCTIONS)
#include <math.h>
#define HMM_SINF sinf
#define HMM_COSF cosf
#define HMM_TANF tanf
#define HMM_SQRTF sqrtf
#define HMM_ACOSF acosf
#endif

#if !defined(HMM_ANGLE_USER_TO_INTERNAL)
#define HMM_ANGLE_USER_TO_INTERNAL(a) (HMM_ToRad(a))
#endif

#if !defined(HMM_ANGLE_INTERNAL_TO_USER)
#if defined(HANDMADE_MATH_USE_RADIANS)
#define HMM_ANGLE_INTERNAL_TO_USER(a) (a)
#elif defined(HANDMADE_MATH_USE_DEGREES)
#define HMM_ANGLE_INTERNAL_TO_USER(a) ((a)*HMM_RadToDeg)
#elif defined(HANDMADE_MATH_USE_TURNS)
#define HMM_ANGLE_INTERNAL_TO_USER(a) ((a)*HMM_RadToTurn)
#endif
#endif

#define HMM_MIN(a, b) ((a) > (b) ? (b) : (a))
#define HMM_MAX(a, b) ((a) < (b) ? (b) : (a))
#define HMM_ABS(a) ((a) > 0 ? (a) : -(a))
#define HMM_MOD(a, m) (((a) % (m)) >= 0 ? ((a) % (m)) : (((a) % (m)) + (m)))
#define HMM_SQUARE(x) ((x) * (x))

#define HMMFMT_VEC3 "[%g,%g,%g]"
#define HMMPRINT_VEC3(vec) vec.x, vec.y, vec.z

#define FMT_VEC4 "[%g,%g,%g,%g]"
#define PRINT_VEC4(vec) vec.x, vec.y, vec.z, vec.w

#define FMT_M4 "[%g,%g,%g,%g\n%g,%g,%g,%g\n%g,%g,%g,%g\n%g,%g,%g,%g]"
#define PRINT_M4(m) m.e[0][0], m.e[0][1], m.e[0][2], m.e[0][3], m.e[1][0], m.e[1][1], m.e[1][2], m.e[1][3], m.e[2][0], m.e[2][1], m.e[2][2], m.e[2][3], m.e[3][0], m.e[3][1], m.e[3][2], m.e[3][3]

typedef union HMM_Vec2 {
  struct
  {
    float X, Y;
  };

  struct {
    float x, y;
  };

  struct
  {
    float U, V;
  };

  struct
  {
    float Left, Right;
  };

  struct
  {
    float Width, Height;
  };

  float Elements[2];
  float e[2];

} HMM_Vec2;

typedef union HMM_Vec3 {
  struct
  {
    float X, Y, Z;
  };

  struct { float x, y, z; };

  struct
  {
    float U, V, W;
  };

  struct
  {
    float R, G, B;
  };

  struct
  {
    HMM_Vec2 XY;
    float _Ignored0;
  };

  struct
  {
    HMM_Vec2 xy;
    float _Ignored4;
  };

  struct
  {
    float _Ignored1;
    HMM_Vec2 YZ;
  };

  struct
  {
    HMM_Vec2 UV;
    float _Ignored2;
  };

  struct
  {
    float _Ignored3;
    HMM_Vec2 VW;
  };

  struct
  {
    HMM_Vec2 cp;
    float _Ignored5;
  };

  float Elements[3];
  float e[3];

} HMM_Vec3;

typedef union HMM_Quat {
  struct
  {
    union {
      HMM_Vec3 XYZ;
      struct
      {
        float X, Y, Z;
      };
    };

    float W;
  };
  
  struct {float x, y, z, w;};
  
  float Elements[4];
  float e[4];

#ifdef HANDMADE_MATH__USE_SSE
  __m128 SSE;
#endif
} HMM_Quat;


typedef union HMM_Vec4 {
  struct
  {
    union {
      HMM_Vec3 XYZ;
      struct
      {
        float X, Y, Z;
      };
      HMM_Vec3 xyz;
    };

    float W;
  };
  struct
  {
    union {
      HMM_Vec3 RGB;
      struct
      {
        float R, G, B;
      };
    };

    float A;
  };

  struct
  {
    HMM_Vec2 XY;
    float _Ignored0;
    float _Ignored1;
  };

  struct {
    HMM_Vec2 xy;
    float _ig0;
    float _ig1;
  };

  struct {
    HMM_Vec2 _2;
    float _ig2;
    float _ig3;
  };

  struct {
    HMM_Vec3 _3;
    float _ig4;
  };

  struct
  {
    float _Ignored2;
    HMM_Vec2 YZ;
    float _Ignored3;
  };

  struct
  {
    float _Ignored4;
    float _Ignored5;
    HMM_Vec2 ZW;
  };
  
  struct
  {
    HMM_Vec2 cp;
    HMM_Vec2 wh;
  };

  HMM_Quat quat;
  struct {float x, y, z, w; };
  struct {float r, g, b, a; };
  struct {float u0, u1, v0, v1;};

  float Elements[4];
  float e[4];

#ifdef HANDMADE_MATH__USE_SSE
  __m128 SSE;
#endif

} HMM_Vec4;

typedef union HMM_Mat2 {
  float Elements[2][2];
  HMM_Vec2 Columns[2];

} HMM_Mat2;

typedef union HMM_Mat3 {
  float Elements[3][3];
  HMM_Vec3 Columns[3];

} HMM_Mat3;

typedef union HMM_Mat4 {
  float Elements[4][4];
  HMM_Vec4 Columns[4];
  HMM_Vec4 col[4];
  float e[4][4];
  float em[16];
} HMM_Mat4;

extern const HMM_Vec2 v2zero;
extern const HMM_Vec2 v2one;
extern const HMM_Vec3 v3zero;
extern const HMM_Vec3 v3one;
extern const HMM_Vec4 v4zero;

typedef signed int HMM_Bool;

extern const HMM_Vec3 vX;
extern const HMM_Vec3 vY;
extern const HMM_Vec3 vZ;

extern const HMM_Vec3 vUP;
extern const HMM_Vec3 vDOWN;
extern const HMM_Vec3 vFWD;
extern const HMM_Vec3 vBKWD;
extern const HMM_Vec3 vLEFT;
extern const HMM_Vec3 vRIGHT;

extern const HMM_Mat4 MAT1;

extern const HMM_Quat QUAT1;

/*
 * Angle unit conversion functions
 */
float HMM_ToRad(float Angle);
float HMM_ToDeg(float Angle);
float HMM_ToTurn(float Angle);

/*
 * Floating-point math functions
 */
float HMM_SinF(float Angle);
float HMM_CosF(float Angle);
float HMM_TanF(float Angle);
float HMM_ACosF(float Arg);
float HMM_SqrtF(float Float);
float HMM_InvSqrtF(float Float);

/*
 * Utility functions
 */
float HMM_Lerp(float A, float Time, float B);
float HMM_Clamp(float Min, float Value, float Max);
float frand(float max);

/*
 * Vector initialization
 */
HMM_Vec2 HMM_V2(float X, float Y);
HMM_Vec3 HMM_V3(float X, float Y, float Z);
HMM_Vec3 HMM_V3i(float i);
HMM_Vec4 HMM_V4(float X, float Y, float Z, float W);
HMM_Vec4 HMM_V4V(HMM_Vec3 Vector, float W);

/*
 * Binary vector operations
 */
HMM_Vec2 HMM_AddV2(HMM_Vec2 Left, HMM_Vec2 Right);
HMM_Vec3 HMM_AddV3(HMM_Vec3 Left, HMM_Vec3 Right);
HMM_Vec4 HMM_AddV4(HMM_Vec4 Left, HMM_Vec4 Right);
HMM_Vec2 HMM_SubV2(HMM_Vec2 Left, HMM_Vec2 Right);
HMM_Vec3 HMM_SubV3(HMM_Vec3 Left, HMM_Vec3 Right);
HMM_Vec4 HMM_SubV4(HMM_Vec4 Left, HMM_Vec4 Right);
HMM_Vec2 HMM_MulV2(HMM_Vec2 Left, HMM_Vec2 Right);
HMM_Vec2 HMM_MulV2F(HMM_Vec2 Left, float Right);
HMM_Vec3 HMM_MulV3(HMM_Vec3 Left, HMM_Vec3 Right);
HMM_Vec3 HMM_MulV3F(HMM_Vec3 Left, float Right);
HMM_Vec4 HMM_MulV4(HMM_Vec4 Left, HMM_Vec4 Right);
HMM_Vec4 HMM_MulV4F(HMM_Vec4 Left, float Right);
HMM_Vec2 HMM_DivV2(HMM_Vec2 Left, HMM_Vec2 Right);
HMM_Vec2 HMM_DivV2F(HMM_Vec2 Left, float Right);
HMM_Vec3 HMM_DivV3(HMM_Vec3 Left, HMM_Vec3 Right);
HMM_Vec3 HMM_DivV3F(HMM_Vec3 Left, float Right);
HMM_Vec4 HMM_DivV4(HMM_Vec4 Left, HMM_Vec4 Right);
HMM_Vec4 HMM_DivV4F(HMM_Vec4 Left, float Right);
HMM_Bool HMM_EqV2(HMM_Vec2 Left, HMM_Vec2 Right);
HMM_Bool HMM_EqV3(HMM_Vec3 Left, HMM_Vec3 Right);
HMM_Bool HMM_EqV4(HMM_Vec4 Left, HMM_Vec4 Right);
float HMM_DotV2(HMM_Vec2 Left, HMM_Vec2 Right);
HMM_Vec2 HMM_ProjV2(HMM_Vec2 a, HMM_Vec2 b);
float HMM_DotV3(HMM_Vec3 Left, HMM_Vec3 Right);
float HMM_DotV4(HMM_Vec4 Left, HMM_Vec4 Right);
HMM_Vec3 HMM_Cross(HMM_Vec3 Left, HMM_Vec3 Right);

/*
 * Unary vector operations
 */
float HMM_LenSqrV2(HMM_Vec2 A);
float HMM_LenSqrV3(HMM_Vec3 A);
float HMM_LenSqrV4(HMM_Vec4 A);
float HMM_LenV2(HMM_Vec2 A);
float HMM_AngleV2(HMM_Vec2 a, HMM_Vec2 b);
float HMM_DistV2(HMM_Vec2 a, HMM_Vec2 b);
HMM_Vec2 HMM_V2Rotate(HMM_Vec2 v, float angle);
float HMM_LenV3(HMM_Vec3 A);
float HMM_DistV3(HMM_Vec3 a, HMM_Vec3 b);
float HMM_AngleV3(HMM_Vec3 a, HMM_Vec3 b);
float HMM_LenV4(HMM_Vec4 A);
float HMM_AngleV4(HMM_Vec4 a, HMM_Vec4 b);
HMM_Vec2 HMM_NormV2(HMM_Vec2 A);
HMM_Vec3 HMM_NormV3(HMM_Vec3 A);
HMM_Vec4 HMM_NormV4(HMM_Vec4 A);

/*
 * Utility vector functions
 */
HMM_Vec2 HMM_LerpV2(HMM_Vec2 A, float Time, HMM_Vec2 B);
HMM_Vec3 HMM_LerpV3(HMM_Vec3 A, float Time, HMM_Vec3 B);
HMM_Vec4 HMM_LerpV4(HMM_Vec4 A, float Time, HMM_Vec4 B);

/*
 * SSE stuff
 */
HMM_Vec4 HMM_LinearCombineV4M4(HMM_Vec4 Left, HMM_Mat4 Right);

/*
 * 2x2 Matrices
 */
HMM_Mat2 HMM_M2(void);
HMM_Mat2 HMM_M2D(float Diagonal);
HMM_Mat2 HMM_TransposeM2(HMM_Mat2 Matrix);
HMM_Mat2 HMM_RotateM2(float angle);
HMM_Mat2 HMM_AddM2(HMM_Mat2 Left, HMM_Mat2 Right);
HMM_Mat2 HMM_SubM2(HMM_Mat2 Left, HMM_Mat2 Right);
HMM_Vec2 HMM_MulM2V2(HMM_Mat2 Matrix, HMM_Vec2 Vector);
HMM_Mat2 HMM_MulM2(HMM_Mat2 Left, HMM_Mat2 Right);
HMM_Mat2 HMM_MulM2F(HMM_Mat2 Matrix, float Scalar);
HMM_Mat2 HMM_DivM2F(HMM_Mat2 Matrix, float Scalar);
float HMM_DeterminantM2(HMM_Mat2 Matrix);
HMM_Mat2 HMM_InvGeneralM2(HMM_Mat2 Matrix);

/*
 * 3x3 Matrices
 */
HMM_Mat3 HMM_M3(void);
HMM_Mat3 HMM_M3D(float Diagonal);
HMM_Mat3 HMM_Translate2D(HMM_Vec2 p);
HMM_Mat3 HMM_RotateM3(float angle);
HMM_Mat3 HMM_ScaleM3(HMM_Vec2 s);
HMM_Mat3 HMM_TransposeM3(HMM_Mat3 Matrix);
HMM_Mat3 HMM_AddM3(HMM_Mat3 Left, HMM_Mat3 Right);
HMM_Mat3 HMM_SubM3(HMM_Mat3 Left, HMM_Mat3 Right);
HMM_Vec3 HMM_MulM3V3(HMM_Mat3 Matrix, HMM_Vec3 Vector);
HMM_Mat3 HMM_MulM3(HMM_Mat3 Left, HMM_Mat3 Right);
HMM_Mat3 HMM_MulM3F(HMM_Mat3 Matrix, float Scalar);
HMM_Mat3 HMM_DivM3F(HMM_Mat3 Matrix, float Scalar);
HMM_Mat2 HMM_ScaleM2(HMM_Vec2 Scale);
float HMM_DeterminantM3(HMM_Mat3 Matrix);
HMM_Mat3 HMM_M2Basis(HMM_Mat2 basis);
HMM_Mat3 HMM_InvGeneralM3(HMM_Mat3 Matrix);

/*
 * 4x4 Matrices
 */
HMM_Mat4 HMM_M4(void);
HMM_Mat4 HMM_M4D(float Diagonal);
HMM_Mat4 HMM_TransposeM4(HMM_Mat4 Matrix);
HMM_Mat4 HMM_AddM4(HMM_Mat4 Left, HMM_Mat4 Right);
HMM_Mat4 HMM_SubM4(HMM_Mat4 Left, HMM_Mat4 Right);
HMM_Mat4 HMM_MulM4(HMM_Mat4 Left, HMM_Mat4 Right);
HMM_Mat4 HMM_MulM4F(HMM_Mat4 Matrix, float Scalar);
HMM_Vec4 HMM_MulM4V4(HMM_Mat4 Matrix, HMM_Vec4 Vector);
HMM_Mat4 HMM_DivM4F(HMM_Mat4 Matrix, float Scalar);
float HMM_DeterminantM4(HMM_Mat4 Matrix);

// Returns a general-purpose inverse of an HMM_Mat4. Note that special-purpose inverses of many transformations
// are available and will be more efficient.
HMM_Mat4 HMM_InvGeneralM4(HMM_Mat4 Matrix);

/*
 * Common graphics transformations
 */

// Produces a right-handed orthographic projection matrix with Z ranging from -1 to 1 (the GL convention).
// Left, Right, Bottom, and Top specify the coordinates of their respective clipping planes.
// Near and Far specify the distances to the near and far clipping planes.
HMM_Mat4 HMM_Orthographic_RH_NO(float Left, float Right, float Bottom, float Top, float Near, float Far);

// Produces a right-handed orthographic projection matrix with Z ranging from 0 to 1 (the DirectX convention).
// Left, Right, Bottom, and Top specify the coordinates of their respective clipping planes.
// Near and Far specify the distances to the near and far clipping planes.
HMM_Mat4 HMM_Orthographic_RH_ZO(float Left, float Right, float Bottom, float Top, float Near, float Far);

// Produces a left-handed orthographic projection matrix with Z ranging from -1 to 1 (the GL convention).
// Left, Right, Bottom, and Top specify the coordinates of their respective clipping planes.
// Near and Far specify the distances to the near and far clipping planes.
HMM_Mat4 HMM_Orthographic_LH_NO(float Left, float Right, float Bottom, float Top, float Near, float Far);

// Produces a left-handed orthographic projection matrix with Z ranging from 0 to 1 (the DirectX convention).
// Left, Right, Bottom, and Top specify the coordinates of their respective clipping planes.
// Near and Far specify the distances to the near and far clipping planes.
HMM_Mat4 HMM_Orthographic_LH_ZO(float Left, float Right, float Bottom, float Top, float Near, float Far);

// Returns an inverse for the given orthographic projection matrix. Works for all orthographic
// projection matrices, regardless of handedness or NDC convention.
HMM_Mat4 HMM_InvOrthographic(HMM_Mat4 OrthoMatrix);
HMM_Mat4 HMM_Perspective_RH_NO(float FOV, float AspectRatio, float Near, float Far);
HMM_Mat4 HMM_Perspective_RH_ZO(float FOV, float AspectRatio, float Near, float Far);
HMM_Mat4 HMM_Perspective_LH_NO(float FOV, float AspectRatio, float Near, float Far);
HMM_Mat4 HMM_Perspective_LH_ZO(float FOV, float AspectRatio, float Near, float Far);

HMM_Mat4 HMM_Perspective_Metal(float FOV, float AspectRation, float Near, float Far);
HMM_Mat4 HMM_Orthographic_Metal(float l, float r, float b, float t, float near, float far);

HMM_Mat4 HMM_Orthographic_DX(float l, float r, float b, float t, float near, float far);
HMM_Mat4 HMM_Orthographic_GL(float l, float r, float b, float t, float near, float far);

HMM_Mat4 HMM_InvPerspective_RH(HMM_Mat4 PerspectiveMatrix);
HMM_Mat4 HMM_InvPerspective_LH(HMM_Mat4 PerspectiveMatrix);
HMM_Mat4 HMM_Translate(HMM_Vec3 Translation);
HMM_Mat4 HMM_InvTranslate(HMM_Mat4 TranslationMatrix);
HMM_Mat4 HMM_Rotate_RH(float Angle, HMM_Vec3 Axis);
HMM_Mat4 HMM_Rotate_LH(float Angle, HMM_Vec3 Axis);
HMM_Mat4 HMM_InvRotate(HMM_Mat4 RotationMatrix);
HMM_Mat4 HMM_Scale(HMM_Vec3 Scale);
HMM_Mat4 HMM_InvScale(HMM_Mat4 ScaleMatrix);
HMM_Mat4 _HMM_LookAt(HMM_Vec3 F, HMM_Vec3 S, HMM_Vec3 U, HMM_Vec3 Eye);
HMM_Mat4 HMM_LookAt_RH(HMM_Vec3 Eye, HMM_Vec3 Center, HMM_Vec3 Up);
HMM_Mat4 HMM_LookAt_LH(HMM_Vec3 Eye, HMM_Vec3 Center, HMM_Vec3 Up);
HMM_Mat4 HMM_InvLookAt(HMM_Mat4 Matrix);

/*
 * Quaternion operations
 */
HMM_Vec3 HMM_QVRot(HMM_Vec3 v, HMM_Quat q)
;
HMM_Quat HMM_Q(float X, float Y, float Z, float W);
HMM_Quat HMM_QV4(HMM_Vec4 Vector);
HMM_Quat HMM_AddQ(HMM_Quat Left, HMM_Quat Right);
HMM_Quat HMM_SubQ(HMM_Quat Left, HMM_Quat Right);
HMM_Quat HMM_MulQ(HMM_Quat Left, HMM_Quat Right);
HMM_Quat HMM_MulQF(HMM_Quat Left, float Multiplicative);
HMM_Quat HMM_DivQF(HMM_Quat Left, float Divnd);
float HMM_DotQ(HMM_Quat Left, HMM_Quat Right);
HMM_Quat HMM_InvQ(HMM_Quat Left);
HMM_Quat HMM_NormQ(HMM_Quat Quat);
HMM_Quat _HMM_MixQ(HMM_Quat Left, float MixLeft, HMM_Quat Right, float MixRight);
HMM_Quat HMM_NLerp(HMM_Quat Left, float Time, HMM_Quat Right);
HMM_Quat HMM_SLerp(HMM_Quat Left, float Time, HMM_Quat Right);
HMM_Mat4 HMM_QToM4(HMM_Quat Left);
HMM_Mat4 HMM_M4TRS(HMM_Vec3 t, HMM_Quat q, HMM_Vec3 s);

HMM_Quat HMM_M4ToQ_RH(HMM_Mat4 M);
HMM_Quat HMM_M4ToQ_LH(HMM_Mat4 M);
HMM_Quat HMM_QFromAxisAngle_RH(HMM_Vec3 Axis, float AngleOfRotation);
HMM_Quat HMM_QFromAxisAngle_LH(HMM_Vec3 Axis, float AngleOfRotation);

float HMM_Q_Roll(HMM_Quat q);
float HMM_Q_Yaw(HMM_Quat q);
float HMM_Q_Pitch(HMM_Quat q);

#endif /* HANDMADE_MATH_H */

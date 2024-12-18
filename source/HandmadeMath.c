#include "HandmadeMath.h"

const HMM_Vec2 v2zero = {0,0};
const HMM_Vec2 v2one = {1,1};
const HMM_Vec3 v3zero = {0,0,0};
const HMM_Vec3 v3one = {1,1,1};
const HMM_Vec4 v4zero = {0,0,0,0};

const HMM_Vec3 vX = {1.0,0.0,0.0};
const HMM_Vec3 vY = {0.0,1.0,0.0};
const HMM_Vec3 vZ = {0.0,0.0,1.0};

const HMM_Vec3 vUP = {0,1,0};
const HMM_Vec3 vDOWN = {0,-1,0};
const HMM_Vec3 vFWD = {0,0,-1};
const HMM_Vec3 vBKWD = {0,0,1};
const HMM_Vec3 vLEFT = {-1,0,0};
const HMM_Vec3 vRIGHT = {1,0,0};

const HMM_Mat4 MAT1 = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
const HMM_Quat QUAT1 = {0,0,0,1};

/*
 * Angle unit conversion functions
 */
float HMM_ToRad(float Angle) {
#if defined(HANDMADE_MATH_USE_RADIANS)
  float Result = Angle;
#elif defined(HANDMADE_MATH_USE_DEGREES)
  float Result = Angle * HMM_DegToRad;
#elif defined(HANDMADE_MATH_USE_TURNS)
  float Result = Angle * HMM_TurnToRad;
#endif

  return Result;
}

float HMM_ToDeg(float Angle) {
#if defined(HANDMADE_MATH_USE_RADIANS)
  float Result = Angle * HMM_RadToDeg;
#elif defined(HANDMADE_MATH_USE_DEGREES)
  float Result = Angle;
#elif defined(HANDMADE_MATH_USE_TURNS)
  float Result = Angle * HMM_TurnToDeg;
#endif

  return Result;
}

float HMM_ToTurn(float Angle) {
#if defined(HANDMADE_MATH_USE_RADIANS)
  float Result = Angle * HMM_RadToTurn;
#elif defined(HANDMADE_MATH_USE_DEGREES)
  float Result = Angle * HMM_DegToTurn;
#elif defined(HANDMADE_MATH_USE_TURNS)
  float Result = Angle;
#endif

  return Result;
}

/*
 * Floating-point math functions
 */

float HMM_SinF(float Angle) {
  return HMM_SINF(HMM_ANGLE_USER_TO_INTERNAL(Angle));
}

float HMM_CosF(float Angle) {
  return HMM_COSF(HMM_ANGLE_USER_TO_INTERNAL(Angle));
}

float HMM_TanF(float Angle) {
  return HMM_TANF(HMM_ANGLE_USER_TO_INTERNAL(Angle));
}

float HMM_ACosF(float Arg) {
  return HMM_ANGLE_INTERNAL_TO_USER(HMM_ACOSF(Arg));
}

float HMM_SqrtF(float Float) {

  float Result;

#ifdef HANDMADE_MATH__USE_SSE
  __m128 In = _mm_set_ss(Float);
  __m128 Out = _mm_sqrt_ss(In);
  Result = _mm_cvtss_f32(Out);
#else
  Result = HMM_SQRTF(Float);
#endif

  return Result;
}

float HMM_InvSqrtF(float Float) {

  float Result;

  Result = 1.0f / HMM_SqrtF(Float);

  return Result;
}

/*
 * Utility functions
 */

float HMM_Lerp(float A, float Time, float B) {
  return (1.0f - Time) * A + Time * B;
}

float HMM_Clamp(float Min, float Value, float Max) {

  float Result = Value;

  if (Result < Min) {
    Result = Min;
  }

  if (Result > Max) {
    Result = Max;
  }

  return Result;
}

float frand(float max) {
  return ((float)rand()/(float)(RAND_MAX))*max;
}

/*
 * Vector initialization
 */

HMM_Vec2 HMM_V2(float X, float Y) {

  HMM_Vec2 Result;
  Result.X = X;
  Result.Y = Y;

  return Result;
}

HMM_Vec3 HMM_V3(float X, float Y, float Z) {

  HMM_Vec3 Result;
  Result.X = X;
  Result.Y = Y;
  Result.Z = Z;

  return Result;
}

HMM_Vec3 HMM_V3i(float i)
{
  return (HMM_Vec3){i,i,i};
}

HMM_Vec4 HMM_V4(float X, float Y, float Z, float W) {

  HMM_Vec4 Result;

#ifdef HANDMADE_MATH__USE_SSE
  Result.SSE = _mm_setr_ps(X, Y, Z, W);
#else
  Result.X = X;
  Result.Y = Y;
  Result.Z = Z;
  Result.W = W;
#endif

  return Result;
}

HMM_Vec4 HMM_V4V(HMM_Vec3 Vector, float W) {

  HMM_Vec4 Result;

#ifdef HANDMADE_MATH__USE_SSE
  Result.SSE = _mm_setr_ps(Vector.X, Vector.Y, Vector.Z, W);
#else
  Result.XYZ = Vector;
  Result.W = W;
#endif

  return Result;
}

/*
 * Binary vector operations
 */

HMM_Vec2 HMM_AddV2(HMM_Vec2 Left, HMM_Vec2 Right) {

  HMM_Vec2 Result;
  Result.X = Left.X + Right.X;
  Result.Y = Left.Y + Right.Y;

  return Result;
}


HMM_Vec3 HMM_AddV3(HMM_Vec3 Left, HMM_Vec3 Right) {

  HMM_Vec3 Result;
  Result.X = Left.X + Right.X;
  Result.Y = Left.Y + Right.Y;
  Result.Z = Left.Z + Right.Z;

  return Result;
}

HMM_Vec4 HMM_AddV4(HMM_Vec4 Left, HMM_Vec4 Right) {

  HMM_Vec4 Result;

#ifdef HANDMADE_MATH__USE_SSE
  Result.SSE = _mm_add_ps(Left.SSE, Right.SSE);
#else
  Result.X = Left.X + Right.X;
  Result.Y = Left.Y + Right.Y;
  Result.Z = Left.Z + Right.Z;
  Result.W = Left.W + Right.W;
#endif

  return Result;
}

HMM_Vec2 HMM_SubV2(HMM_Vec2 Left, HMM_Vec2 Right) {

  HMM_Vec2 Result;
  Result.X = Left.X - Right.X;
  Result.Y = Left.Y - Right.Y;

  return Result;
}

HMM_Vec3 HMM_SubV3(HMM_Vec3 Left, HMM_Vec3 Right) {

  HMM_Vec3 Result;
  Result.X = Left.X - Right.X;
  Result.Y = Left.Y - Right.Y;
  Result.Z = Left.Z - Right.Z;

  return Result;
}

HMM_Vec4 HMM_SubV4(HMM_Vec4 Left, HMM_Vec4 Right) {

  HMM_Vec4 Result;

#ifdef HANDMADE_MATH__USE_SSE
  Result.SSE = _mm_sub_ps(Left.SSE, Right.SSE);
#else
  Result.X = Left.X - Right.X;
  Result.Y = Left.Y - Right.Y;
  Result.Z = Left.Z - Right.Z;
  Result.W = Left.W - Right.W;
#endif

  return Result;
}

HMM_Vec2 HMM_ScaleV2(HMM_Vec2 v, double s)
{
  return HMM_V2(v.X*s, v.Y*s);    
}

HMM_Vec2 HMM_MulV2(HMM_Vec2 Left, HMM_Vec2 Right) {

  HMM_Vec2 Result;
  Result.X = Left.X * Right.X;
  Result.Y = Left.Y * Right.Y;

  return Result;
}

HMM_Vec2 HMM_MulV2F(HMM_Vec2 Left, float Right) {

  HMM_Vec2 Result;
  Result.X = Left.X * Right;
  Result.Y = Left.Y * Right;

  return Result;
}

HMM_Vec3 HMM_MulV3(HMM_Vec3 Left, HMM_Vec3 Right) {

  HMM_Vec3 Result;
  Result.X = Left.X * Right.X;
  Result.Y = Left.Y * Right.Y;
  Result.Z = Left.Z * Right.Z;

  return Result;
}

HMM_Vec3 HMM_MulV3F(HMM_Vec3 Left, float Right) {

  HMM_Vec3 Result;
  Result.X = Left.X * Right;
  Result.Y = Left.Y * Right;
  Result.Z = Left.Z * Right;

  return Result;
}

HMM_Vec4 HMM_MulV4(HMM_Vec4 Left, HMM_Vec4 Right) {

  HMM_Vec4 Result;

#ifdef HANDMADE_MATH__USE_SSE
  Result.SSE = _mm_mul_ps(Left.SSE, Right.SSE);
#else
  Result.X = Left.X * Right.X;
  Result.Y = Left.Y * Right.Y;
  Result.Z = Left.Z * Right.Z;
  Result.W = Left.W * Right.W;
#endif

  return Result;
}

HMM_Vec4 HMM_MulV4F(HMM_Vec4 Left, float Right) {

  HMM_Vec4 Result;

#ifdef HANDMADE_MATH__USE_SSE
  __m128 Scalar = _mm_set1_ps(Right);
  Result.SSE = _mm_mul_ps(Left.SSE, Scalar);
#else
  Result.X = Left.X * Right;
  Result.Y = Left.Y * Right;
  Result.Z = Left.Z * Right;
  Result.W = Left.W * Right;
#endif

  return Result;
}

HMM_Vec2 HMM_DivV2(HMM_Vec2 Left, HMM_Vec2 Right) {

  HMM_Vec2 Result;
  Result.X = Left.X / Right.X;
  Result.Y = Left.Y / Right.Y;

  return Result;
}

HMM_Vec2 HMM_DivV2F(HMM_Vec2 Left, float Right) {

  HMM_Vec2 Result;
  Result.X = Left.X / Right;
  Result.Y = Left.Y / Right;

  return Result;
}

HMM_Vec3 HMM_DivV3(HMM_Vec3 Left, HMM_Vec3 Right) {

  HMM_Vec3 Result;
  Result.X = Left.X / Right.X;
  Result.Y = Left.Y / Right.Y;
  Result.Z = Left.Z / Right.Z;

  return Result;
}

HMM_Vec3 HMM_DivV3F(HMM_Vec3 Left, float Right) {

  HMM_Vec3 Result;
  Result.X = Left.X / Right;
  Result.Y = Left.Y / Right;
  Result.Z = Left.Z / Right;

  return Result;
}

HMM_Vec4 HMM_DivV4(HMM_Vec4 Left, HMM_Vec4 Right) {

  HMM_Vec4 Result;

#ifdef HANDMADE_MATH__USE_SSE
  Result.SSE = _mm_div_ps(Left.SSE, Right.SSE);
#else
  Result.X = Left.X / Right.X;
  Result.Y = Left.Y / Right.Y;
  Result.Z = Left.Z / Right.Z;
  Result.W = Left.W / Right.W;
#endif

  return Result;
}

HMM_Vec4 HMM_DivV4F(HMM_Vec4 Left, float Right) {

  HMM_Vec4 Result;

#ifdef HANDMADE_MATH__USE_SSE
  __m128 Scalar = _mm_set1_ps(Right);
  Result.SSE = _mm_div_ps(Left.SSE, Scalar);
#else
  Result.X = Left.X / Right;
  Result.Y = Left.Y / Right;
  Result.Z = Left.Z / Right;
  Result.W = Left.W / Right;
#endif

  return Result;
}

HMM_Bool HMM_EqV2(HMM_Vec2 Left, HMM_Vec2 Right) {
  return Left.X == Right.X && Left.Y == Right.Y;
}

HMM_Bool HMM_EqV3(HMM_Vec3 Left, HMM_Vec3 Right) {
  return Left.X == Right.X && Left.Y == Right.Y && Left.Z == Right.Z;
}

HMM_Bool HMM_EqV4(HMM_Vec4 Left, HMM_Vec4 Right) {
  return Left.X == Right.X && Left.Y == Right.Y && Left.Z == Right.Z && Left.W == Right.W;
}

float HMM_DotV2(HMM_Vec2 Left, HMM_Vec2 Right) {
  return (Left.X * Right.X) + (Left.Y * Right.Y);
}

HMM_Vec2 HMM_ProjV2(HMM_Vec2 a, HMM_Vec2 b)
{
  return HMM_MulV2F(b, HMM_DotV2(a,b)/HMM_DotV2(b,b));
}

float HMM_DotV3(HMM_Vec3 Left, HMM_Vec3 Right) {
  return (Left.X * Right.X) + (Left.Y * Right.Y) + (Left.Z * Right.Z);
}

float HMM_DotV4(HMM_Vec4 Left, HMM_Vec4 Right) {

  float Result;

  // NOTE(zak): IN the future if we wanna check what version SSE is support
  // we can use _mm_dp_ps (4.3) but for now we will use the old way.
  // Or a r = _mm_mul_ps(v1, v2), r = _mm_hadd_ps(r, r), r = _mm_hadd_ps(r, r) for SSE3
#ifdef HANDMADE_MATH__USE_SSE
  __m128 SSEResultOne = _mm_mul_ps(Left.SSE, Right.SSE);
  __m128 SSEResultTwo = _mm_shuffle_ps(SSEResultOne, SSEResultOne, _MM_SHUFFLE(2, 3, 0, 1));
  SSEResultOne = _mm_add_ps(SSEResultOne, SSEResultTwo);
  SSEResultTwo = _mm_shuffle_ps(SSEResultOne, SSEResultOne, _MM_SHUFFLE(0, 1, 2, 3));
  SSEResultOne = _mm_add_ps(SSEResultOne, SSEResultTwo);
  _mm_store_ss(&Result, SSEResultOne);
#else
  Result = ((Left.X * Right.X) + (Left.Z * Right.Z)) + ((Left.Y * Right.Y) + (Left.W * Right.W));
#endif

  return Result;
}

HMM_Vec3 HMM_Cross(HMM_Vec3 Left, HMM_Vec3 Right) {

  HMM_Vec3 Result;
  Result.X = (Left.Y * Right.Z) - (Left.Z * Right.Y);
  Result.Y = (Left.Z * Right.X) - (Left.X * Right.Z);
  Result.Z = (Left.X * Right.Y) - (Left.Y * Right.X);

  return Result;
}

/*
 * Unary vector operations
 */

float HMM_LenSqrV2(HMM_Vec2 A) {
  return HMM_DotV2(A, A);
}

float HMM_LenSqrV3(HMM_Vec3 A) {
  return HMM_DotV3(A, A);
}

float HMM_LenSqrV4(HMM_Vec4 A) {
  return HMM_DotV4(A, A);
}

float HMM_LenV2(HMM_Vec2 A) {
  return HMM_SqrtF(HMM_LenSqrV2(A));
}

float HMM_AngleV2(HMM_Vec2 a, HMM_Vec2 b)
{
  return acos(HMM_DotV2(a,b)/(HMM_LenV2(a)*HMM_LenV2(b)));
}

float HMM_DistV2(HMM_Vec2 a, HMM_Vec2 b) {
  return HMM_LenV2(HMM_SubV2(a,b));
}

HMM_Vec2 HMM_V2Rotate(HMM_Vec2 v, float angle)
{
  float r = HMM_LenV2(v);
  angle += atan2(v.x, v.y);
  return (HMM_Vec2){r*cos(angle), r*sin(angle)};
}

float HMM_LenV3(HMM_Vec3 A) {
  return HMM_SqrtF(HMM_LenSqrV3(A));
}

float HMM_DistV3(HMM_Vec3 a, HMM_Vec3 b) {
  return HMM_LenV3(HMM_SubV3(a,b));
}

float HMM_AngleV3(HMM_Vec3 a, HMM_Vec3 b)
{
  return acos(HMM_DotV3(a,b)/(HMM_LenV3(a)*HMM_LenV3(b)));
}

float HMM_LenV4(HMM_Vec4 A) {
  return HMM_SqrtF(HMM_LenSqrV4(A));
}

float HMM_AngleV4(HMM_Vec4 a, HMM_Vec4 b)
{
  return acos(HMM_DotV4(a,b)/(HMM_LenV4(a)*HMM_LenV4(b)));
}

HMM_Vec2 HMM_NormV2(HMM_Vec2 A) {
  return HMM_MulV2F(A, HMM_InvSqrtF(HMM_DotV2(A, A)));
}

HMM_Vec3 HMM_NormV3(HMM_Vec3 A) {
  return HMM_MulV3F(A, HMM_InvSqrtF(HMM_DotV3(A, A)));
}

HMM_Vec4 HMM_NormV4(HMM_Vec4 A) {
  return HMM_MulV4F(A, HMM_InvSqrtF(HMM_DotV4(A, A)));
}

/*
 * Utility vector functions
 */

HMM_Vec2 HMM_LerpV2(HMM_Vec2 A, float Time, HMM_Vec2 B) {
  return HMM_AddV2(HMM_MulV2F(A, 1.0f - Time), HMM_MulV2F(B, Time));
}

HMM_Vec3 HMM_LerpV3(HMM_Vec3 A, float Time, HMM_Vec3 B) {
  return HMM_AddV3(HMM_MulV3F(A, 1.0f - Time), HMM_MulV3F(B, Time));
}

HMM_Vec4 HMM_LerpV4(HMM_Vec4 A, float Time, HMM_Vec4 B) {
  return HMM_AddV4(HMM_MulV4F(A, 1.0f - Time), HMM_MulV4F(B, Time));
}

/*
 * SSE stuff
 */

HMM_Vec4 HMM_LinearCombineV4M4(HMM_Vec4 Left, HMM_Mat4 Right) {

  HMM_Vec4 Result;
#ifdef HANDMADE_MATH__USE_SSE
  Result.SSE = _mm_mul_ps(_mm_shuffle_ps(Left.SSE, Left.SSE, 0x00), Right.Columns[0].SSE);
  Result.SSE = _mm_add_ps(Result.SSE, _mm_mul_ps(_mm_shuffle_ps(Left.SSE, Left.SSE, 0x55), Right.Columns[1].SSE));
  Result.SSE = _mm_add_ps(Result.SSE, _mm_mul_ps(_mm_shuffle_ps(Left.SSE, Left.SSE, 0xaa), Right.Columns[2].SSE));
  Result.SSE = _mm_add_ps(Result.SSE, _mm_mul_ps(_mm_shuffle_ps(Left.SSE, Left.SSE, 0xff), Right.Columns[3].SSE));
#else
  Result.X = Left.Elements[0] * Right.Columns[0].X;
  Result.Y = Left.Elements[0] * Right.Columns[0].Y;
  Result.Z = Left.Elements[0] * Right.Columns[0].Z;
  Result.W = Left.Elements[0] * Right.Columns[0].W;

  Result.X += Left.Elements[1] * Right.Columns[1].X;
  Result.Y += Left.Elements[1] * Right.Columns[1].Y;
  Result.Z += Left.Elements[1] * Right.Columns[1].Z;
  Result.W += Left.Elements[1] * Right.Columns[1].W;

  Result.X += Left.Elements[2] * Right.Columns[2].X;
  Result.Y += Left.Elements[2] * Right.Columns[2].Y;
  Result.Z += Left.Elements[2] * Right.Columns[2].Z;
  Result.W += Left.Elements[2] * Right.Columns[2].W;

  Result.X += Left.Elements[3] * Right.Columns[3].X;
  Result.Y += Left.Elements[3] * Right.Columns[3].Y;
  Result.Z += Left.Elements[3] * Right.Columns[3].Z;
  Result.W += Left.Elements[3] * Right.Columns[3].W;
#endif

  return Result;
}

/*
 * 2x2 Matrices
 */

HMM_Mat2 HMM_M2(void) {
  HMM_Mat2 Result = {0};
  return Result;
}

HMM_Mat2 HMM_M2D(float Diagonal) {

  HMM_Mat2 Result = {0};
  Result.Elements[0][0] = Diagonal;
  Result.Elements[1][1] = Diagonal;

  return Result;
}

HMM_Mat2 HMM_TransposeM2(HMM_Mat2 Matrix) {

  HMM_Mat2 Result = Matrix;

  Result.Elements[0][1] = Matrix.Elements[1][0];
  Result.Elements[1][0] = Matrix.Elements[0][1];

  return Result;
}

HMM_Mat2 HMM_RotateM2(float angle)
{
  HMM_Mat2 result;
  result.Elements[0][0] = cos(angle);
  result.Elements[0][1] = sin(angle);
  result.Elements[1][0] = -result.Elements[0][1];
  result.Elements[1][1] = result.Elements[0][0];

  return result;
}


HMM_Mat2 HMM_AddM2(HMM_Mat2 Left, HMM_Mat2 Right) {

  HMM_Mat2 Result;

  Result.Elements[0][0] = Left.Elements[0][0] + Right.Elements[0][0];
  Result.Elements[0][1] = Left.Elements[0][1] + Right.Elements[0][1];
  Result.Elements[1][0] = Left.Elements[1][0] + Right.Elements[1][0];
  Result.Elements[1][1] = Left.Elements[1][1] + Right.Elements[1][1];

  return Result;
}

HMM_Mat2 HMM_SubM2(HMM_Mat2 Left, HMM_Mat2 Right) {

  HMM_Mat2 Result;

  Result.Elements[0][0] = Left.Elements[0][0] - Right.Elements[0][0];
  Result.Elements[0][1] = Left.Elements[0][1] - Right.Elements[0][1];
  Result.Elements[1][0] = Left.Elements[1][0] - Right.Elements[1][0];
  Result.Elements[1][1] = Left.Elements[1][1] - Right.Elements[1][1];

  return Result;
}

HMM_Vec2 HMM_MulM2V2(HMM_Mat2 Matrix, HMM_Vec2 Vector) {

  HMM_Vec2 Result;

  Result.X = Vector.Elements[0] * Matrix.Columns[0].X;
  Result.Y = Vector.Elements[0] * Matrix.Columns[0].Y;

  Result.X += Vector.Elements[1] * Matrix.Columns[1].X;
  Result.Y += Vector.Elements[1] * Matrix.Columns[1].Y;

  return Result;
}

HMM_Mat2 HMM_MulM2(HMM_Mat2 Left, HMM_Mat2 Right) {

  HMM_Mat2 Result;
  Result.Columns[0] = HMM_MulM2V2(Left, Right.Columns[0]);
  Result.Columns[1] = HMM_MulM2V2(Left, Right.Columns[1]);

  return Result;
}

HMM_Mat2 HMM_MulM2F(HMM_Mat2 Matrix, float Scalar) {

  HMM_Mat2 Result;

  Result.Elements[0][0] = Matrix.Elements[0][0] * Scalar;
  Result.Elements[0][1] = Matrix.Elements[0][1] * Scalar;
  Result.Elements[1][0] = Matrix.Elements[1][0] * Scalar;
  Result.Elements[1][1] = Matrix.Elements[1][1] * Scalar;

  return Result;
}

HMM_Mat2 HMM_DivM2F(HMM_Mat2 Matrix, float Scalar) {

  HMM_Mat2 Result;

  Result.Elements[0][0] = Matrix.Elements[0][0] / Scalar;
  Result.Elements[0][1] = Matrix.Elements[0][1] / Scalar;
  Result.Elements[1][0] = Matrix.Elements[1][0] / Scalar;
  Result.Elements[1][1] = Matrix.Elements[1][1] / Scalar;

  return Result;
}

float HMM_DeterminantM2(HMM_Mat2 Matrix) {
  return Matrix.Elements[0][0] * Matrix.Elements[1][1] - Matrix.Elements[0][1] * Matrix.Elements[1][0];
}

HMM_Mat2 HMM_InvGeneralM2(HMM_Mat2 Matrix) {

  HMM_Mat2 Result;
  float InvDeterminant = 1.0f / HMM_DeterminantM2(Matrix);
  Result.Elements[0][0] = InvDeterminant * +Matrix.Elements[1][1];
  Result.Elements[1][1] = InvDeterminant * +Matrix.Elements[0][0];
  Result.Elements[0][1] = InvDeterminant * -Matrix.Elements[0][1];
  Result.Elements[1][0] = InvDeterminant * -Matrix.Elements[1][0];

  return Result;
}

/*
 * 3x3 Matrices
 */

HMM_Mat3 HMM_M3(void) {
  HMM_Mat3 Result = {0};
  return Result;
}

HMM_Mat3 HMM_M3D(float Diagonal) {

  HMM_Mat3 Result = {0};
  Result.Elements[0][0] = Diagonal;
  Result.Elements[1][1] = Diagonal;
  Result.Elements[2][2] = Diagonal;

  return Result;
}

HMM_Mat3 HMM_Translate2D(HMM_Vec2 p)
{
  HMM_Mat3 res = HMM_M3D(1);
  res.Columns[2].XY = p;
  return res;
}

HMM_Mat3 HMM_RotateM3(float angle)
{
  HMM_Mat3 r = HMM_M3D(1);
  r.Columns[0] = (HMM_Vec3){cos(angle), sin(angle), 0};
  r.Columns[1] = (HMM_Vec3){-sin(angle), cos(angle), 0};
  return r;
}

HMM_Mat3 HMM_ScaleM3(HMM_Vec2 s)
{
  HMM_Mat3 sm = HMM_M3D(1);
  sm.Columns[0].X = s.x;
  sm.Columns[1].Y = s.y;
  return sm;
}

HMM_Mat3 HMM_TransposeM3(HMM_Mat3 Matrix) {

  HMM_Mat3 Result = Matrix;

  Result.Elements[0][1] = Matrix.Elements[1][0];
  Result.Elements[0][2] = Matrix.Elements[2][0];
  Result.Elements[1][0] = Matrix.Elements[0][1];
  Result.Elements[1][2] = Matrix.Elements[2][1];
  Result.Elements[2][1] = Matrix.Elements[1][2];
  Result.Elements[2][0] = Matrix.Elements[0][2];

  return Result;
}

HMM_Mat3 HMM_AddM3(HMM_Mat3 Left, HMM_Mat3 Right) {

  HMM_Mat3 Result;

  Result.Elements[0][0] = Left.Elements[0][0] + Right.Elements[0][0];
  Result.Elements[0][1] = Left.Elements[0][1] + Right.Elements[0][1];
  Result.Elements[0][2] = Left.Elements[0][2] + Right.Elements[0][2];
  Result.Elements[1][0] = Left.Elements[1][0] + Right.Elements[1][0];
  Result.Elements[1][1] = Left.Elements[1][1] + Right.Elements[1][1];
  Result.Elements[1][2] = Left.Elements[1][2] + Right.Elements[1][2];
  Result.Elements[2][0] = Left.Elements[2][0] + Right.Elements[2][0];
  Result.Elements[2][1] = Left.Elements[2][1] + Right.Elements[2][1];
  Result.Elements[2][2] = Left.Elements[2][2] + Right.Elements[2][2];

  return Result;
}

HMM_Mat3 HMM_SubM3(HMM_Mat3 Left, HMM_Mat3 Right) {

  HMM_Mat3 Result;

  Result.Elements[0][0] = Left.Elements[0][0] - Right.Elements[0][0];
  Result.Elements[0][1] = Left.Elements[0][1] - Right.Elements[0][1];
  Result.Elements[0][2] = Left.Elements[0][2] - Right.Elements[0][2];
  Result.Elements[1][0] = Left.Elements[1][0] - Right.Elements[1][0];
  Result.Elements[1][1] = Left.Elements[1][1] - Right.Elements[1][1];
  Result.Elements[1][2] = Left.Elements[1][2] - Right.Elements[1][2];
  Result.Elements[2][0] = Left.Elements[2][0] - Right.Elements[2][0];
  Result.Elements[2][1] = Left.Elements[2][1] - Right.Elements[2][1];
  Result.Elements[2][2] = Left.Elements[2][2] - Right.Elements[2][2];

  return Result;
}

HMM_Vec3 HMM_MulM3V3(HMM_Mat3 Matrix, HMM_Vec3 Vector) {
  HMM_Vec3 Result;

  Result.X = Vector.Elements[0] * Matrix.Columns[0].X;
  Result.Y = Vector.Elements[0] * Matrix.Columns[0].Y;
  Result.Z = Vector.Elements[0] * Matrix.Columns[0].Z;

  Result.X += Vector.Elements[1] * Matrix.Columns[1].X;
  Result.Y += Vector.Elements[1] * Matrix.Columns[1].Y;
  Result.Z += Vector.Elements[1] * Matrix.Columns[1].Z;

  Result.X += Vector.Elements[2] * Matrix.Columns[2].X;
  Result.Y += Vector.Elements[2] * Matrix.Columns[2].Y;
  Result.Z += Vector.Elements[2] * Matrix.Columns[2].Z;

  return Result;
}

HMM_Mat3 HMM_MulM3(HMM_Mat3 Left, HMM_Mat3 Right) {

  HMM_Mat3 Result;
  Result.Columns[0] = HMM_MulM3V3(Left, Right.Columns[0]);
  Result.Columns[1] = HMM_MulM3V3(Left, Right.Columns[1]);
  Result.Columns[2] = HMM_MulM3V3(Left, Right.Columns[2]);

  return Result;
}

/*HMM_Mat3 HMM_VMulM3(int c, ...)
{
  HMM_Mat3 res = HMM_M3D(1);
  va_list args;
  va_start(args, c);
  for (int i = 0; i < c; i++) {
    HMM_Mat3 m = va_arg(args, HMM_Mat3);
    res = HMM_MulM3(m, res);
  }
  va_end(args);
  return res;
}
*/
HMM_Mat3 HMM_MulM3F(HMM_Mat3 Matrix, float Scalar) {

  HMM_Mat3 Result;

  Result.Elements[0][0] = Matrix.Elements[0][0] * Scalar;
  Result.Elements[0][1] = Matrix.Elements[0][1] * Scalar;
  Result.Elements[0][2] = Matrix.Elements[0][2] * Scalar;
  Result.Elements[1][0] = Matrix.Elements[1][0] * Scalar;
  Result.Elements[1][1] = Matrix.Elements[1][1] * Scalar;
  Result.Elements[1][2] = Matrix.Elements[1][2] * Scalar;
  Result.Elements[2][0] = Matrix.Elements[2][0] * Scalar;
  Result.Elements[2][1] = Matrix.Elements[2][1] * Scalar;
  Result.Elements[2][2] = Matrix.Elements[2][2] * Scalar;

  return Result;
}

HMM_Mat3 HMM_DivM3F(HMM_Mat3 Matrix, float Scalar) {

  HMM_Mat3 Result;

  Result.Elements[0][0] = Matrix.Elements[0][0] / Scalar;
  Result.Elements[0][1] = Matrix.Elements[0][1] / Scalar;
  Result.Elements[0][2] = Matrix.Elements[0][2] / Scalar;
  Result.Elements[1][0] = Matrix.Elements[1][0] / Scalar;
  Result.Elements[1][1] = Matrix.Elements[1][1] / Scalar;
  Result.Elements[1][2] = Matrix.Elements[1][2] / Scalar;
  Result.Elements[2][0] = Matrix.Elements[2][0] / Scalar;
  Result.Elements[2][1] = Matrix.Elements[2][1] / Scalar;
  Result.Elements[2][2] = Matrix.Elements[2][2] / Scalar;

  return Result;
}

HMM_Mat2 HMM_ScaleM2(HMM_Vec2 Scale) {

  HMM_Mat2 Result = HMM_M2D(1.0f);
  Result.Elements[0][0] = Scale.X;
  Result.Elements[1][1] = Scale.Y;

  return Result;
}

float HMM_DeterminantM3(HMM_Mat3 Matrix) {

  HMM_Mat3 Cross;
  Cross.Columns[0] = HMM_Cross(Matrix.Columns[1], Matrix.Columns[2]);
  Cross.Columns[1] = HMM_Cross(Matrix.Columns[2], Matrix.Columns[0]);
  Cross.Columns[2] = HMM_Cross(Matrix.Columns[0], Matrix.Columns[1]);

  return HMM_DotV3(Cross.Columns[2], Matrix.Columns[2]);
}

HMM_Mat3 HMM_M2Basis(HMM_Mat2 basis)
{
  HMM_Mat3 m;
  m.Columns[0].XY = basis.Columns[0];
  m.Columns[1].XY = basis.Columns[1];
  m.Columns[2].Z = 1;
  return m;
}

HMM_Mat3 HMM_InvGeneralM3(HMM_Mat3 Matrix) {

  HMM_Mat3 Cross;
  Cross.Columns[0] = HMM_Cross(Matrix.Columns[1], Matrix.Columns[2]);
  Cross.Columns[1] = HMM_Cross(Matrix.Columns[2], Matrix.Columns[0]);
  Cross.Columns[2] = HMM_Cross(Matrix.Columns[0], Matrix.Columns[1]);

  float InvDeterminant = 1.0f / HMM_DotV3(Cross.Columns[2], Matrix.Columns[2]);

  HMM_Mat3 Result;
  Result.Columns[0] = HMM_MulV3F(Cross.Columns[0], InvDeterminant);
  Result.Columns[1] = HMM_MulV3F(Cross.Columns[1], InvDeterminant);
  Result.Columns[2] = HMM_MulV3F(Cross.Columns[2], InvDeterminant);

  return HMM_TransposeM3(Result);
}

/*
 * 4x4 Matrices
 */

HMM_Mat4 HMM_M4(void) {
  HMM_Mat4 Result = {0};
  return Result;
}

HMM_Mat4 HMM_M4D(float Diagonal) {

  HMM_Mat4 Result = {0};
  Result.Elements[0][0] = Diagonal;
  Result.Elements[1][1] = Diagonal;
  Result.Elements[2][2] = Diagonal;
  Result.Elements[3][3] = Diagonal;

  return Result;
}

HMM_Mat4 HMM_TransposeM4(HMM_Mat4 Matrix) {

  HMM_Mat4 Result = Matrix;
#ifdef HANDMADE_MATH__USE_SSE
  _MM_TRANSPOSE4_PS(Result.Columns[0].SSE, Result.Columns[1].SSE, Result.Columns[2].SSE, Result.Columns[3].SSE);
#else
  Result.Elements[0][1] = Matrix.Elements[1][0];
  Result.Elements[0][2] = Matrix.Elements[2][0];
  Result.Elements[0][3] = Matrix.Elements[3][0];
  Result.Elements[1][0] = Matrix.Elements[0][1];
  Result.Elements[1][2] = Matrix.Elements[2][1];
  Result.Elements[1][3] = Matrix.Elements[3][1];
  Result.Elements[2][1] = Matrix.Elements[1][2];
  Result.Elements[2][0] = Matrix.Elements[0][2];
  Result.Elements[2][3] = Matrix.Elements[3][2];
  Result.Elements[3][1] = Matrix.Elements[1][3];
  Result.Elements[3][2] = Matrix.Elements[2][3];
  Result.Elements[3][0] = Matrix.Elements[0][3];
#endif

  return Result;
}

HMM_Mat4 HMM_AddM4(HMM_Mat4 Left, HMM_Mat4 Right) {

  HMM_Mat4 Result;

#ifdef HANDMADE_MATH__USE_SSE
  Result.Columns[0].SSE = _mm_add_ps(Left.Columns[0].SSE, Right.Columns[0].SSE);
  Result.Columns[1].SSE = _mm_add_ps(Left.Columns[1].SSE, Right.Columns[1].SSE);
  Result.Columns[2].SSE = _mm_add_ps(Left.Columns[2].SSE, Right.Columns[2].SSE);
  Result.Columns[3].SSE = _mm_add_ps(Left.Columns[3].SSE, Right.Columns[3].SSE);
#else
  Result.Elements[0][0] = Left.Elements[0][0] + Right.Elements[0][0];
  Result.Elements[0][1] = Left.Elements[0][1] + Right.Elements[0][1];
  Result.Elements[0][2] = Left.Elements[0][2] + Right.Elements[0][2];
  Result.Elements[0][3] = Left.Elements[0][3] + Right.Elements[0][3];
  Result.Elements[1][0] = Left.Elements[1][0] + Right.Elements[1][0];
  Result.Elements[1][1] = Left.Elements[1][1] + Right.Elements[1][1];
  Result.Elements[1][2] = Left.Elements[1][2] + Right.Elements[1][2];
  Result.Elements[1][3] = Left.Elements[1][3] + Right.Elements[1][3];
  Result.Elements[2][0] = Left.Elements[2][0] + Right.Elements[2][0];
  Result.Elements[2][1] = Left.Elements[2][1] + Right.Elements[2][1];
  Result.Elements[2][2] = Left.Elements[2][2] + Right.Elements[2][2];
  Result.Elements[2][3] = Left.Elements[2][3] + Right.Elements[2][3];
  Result.Elements[3][0] = Left.Elements[3][0] + Right.Elements[3][0];
  Result.Elements[3][1] = Left.Elements[3][1] + Right.Elements[3][1];
  Result.Elements[3][2] = Left.Elements[3][2] + Right.Elements[3][2];
  Result.Elements[3][3] = Left.Elements[3][3] + Right.Elements[3][3];
#endif

  return Result;
}

HMM_Mat4 HMM_SubM4(HMM_Mat4 Left, HMM_Mat4 Right) {

  HMM_Mat4 Result;

#ifdef HANDMADE_MATH__USE_SSE
  Result.Columns[0].SSE = _mm_sub_ps(Left.Columns[0].SSE, Right.Columns[0].SSE);
  Result.Columns[1].SSE = _mm_sub_ps(Left.Columns[1].SSE, Right.Columns[1].SSE);
  Result.Columns[2].SSE = _mm_sub_ps(Left.Columns[2].SSE, Right.Columns[2].SSE);
  Result.Columns[3].SSE = _mm_sub_ps(Left.Columns[3].SSE, Right.Columns[3].SSE);
#else
  Result.Elements[0][0] = Left.Elements[0][0] - Right.Elements[0][0];
  Result.Elements[0][1] = Left.Elements[0][1] - Right.Elements[0][1];
  Result.Elements[0][2] = Left.Elements[0][2] - Right.Elements[0][2];
  Result.Elements[0][3] = Left.Elements[0][3] - Right.Elements[0][3];
  Result.Elements[1][0] = Left.Elements[1][0] - Right.Elements[1][0];
  Result.Elements[1][1] = Left.Elements[1][1] - Right.Elements[1][1];
  Result.Elements[1][2] = Left.Elements[1][2] - Right.Elements[1][2];
  Result.Elements[1][3] = Left.Elements[1][3] - Right.Elements[1][3];
  Result.Elements[2][0] = Left.Elements[2][0] - Right.Elements[2][0];
  Result.Elements[2][1] = Left.Elements[2][1] - Right.Elements[2][1];
  Result.Elements[2][2] = Left.Elements[2][2] - Right.Elements[2][2];
  Result.Elements[2][3] = Left.Elements[2][3] - Right.Elements[2][3];
  Result.Elements[3][0] = Left.Elements[3][0] - Right.Elements[3][0];
  Result.Elements[3][1] = Left.Elements[3][1] - Right.Elements[3][1];
  Result.Elements[3][2] = Left.Elements[3][2] - Right.Elements[3][2];
  Result.Elements[3][3] = Left.Elements[3][3] - Right.Elements[3][3];
#endif

  return Result;
}

HMM_Mat4 HMM_MulM4(HMM_Mat4 Left, HMM_Mat4 Right) {

  HMM_Mat4 Result;
  Result.Columns[0] = HMM_LinearCombineV4M4(Right.Columns[0], Left);
  Result.Columns[1] = HMM_LinearCombineV4M4(Right.Columns[1], Left);
  Result.Columns[2] = HMM_LinearCombineV4M4(Right.Columns[2], Left);
  Result.Columns[3] = HMM_LinearCombineV4M4(Right.Columns[3], Left);

  return Result;
}

HMM_Mat4 HMM_MulM4F(HMM_Mat4 Matrix, float Scalar) {

  HMM_Mat4 Result;

#ifdef HANDMADE_MATH__USE_SSE
  __m128 SSEScalar = _mm_set1_ps(Scalar);
  Result.Columns[0].SSE = _mm_mul_ps(Matrix.Columns[0].SSE, SSEScalar);
  Result.Columns[1].SSE = _mm_mul_ps(Matrix.Columns[1].SSE, SSEScalar);
  Result.Columns[2].SSE = _mm_mul_ps(Matrix.Columns[2].SSE, SSEScalar);
  Result.Columns[3].SSE = _mm_mul_ps(Matrix.Columns[3].SSE, SSEScalar);
#else
  Result.Elements[0][0] = Matrix.Elements[0][0] * Scalar;
  Result.Elements[0][1] = Matrix.Elements[0][1] * Scalar;
  Result.Elements[0][2] = Matrix.Elements[0][2] * Scalar;
  Result.Elements[0][3] = Matrix.Elements[0][3] * Scalar;
  Result.Elements[1][0] = Matrix.Elements[1][0] * Scalar;
  Result.Elements[1][1] = Matrix.Elements[1][1] * Scalar;
  Result.Elements[1][2] = Matrix.Elements[1][2] * Scalar;
  Result.Elements[1][3] = Matrix.Elements[1][3] * Scalar;
  Result.Elements[2][0] = Matrix.Elements[2][0] * Scalar;
  Result.Elements[2][1] = Matrix.Elements[2][1] * Scalar;
  Result.Elements[2][2] = Matrix.Elements[2][2] * Scalar;
  Result.Elements[2][3] = Matrix.Elements[2][3] * Scalar;
  Result.Elements[3][0] = Matrix.Elements[3][0] * Scalar;
  Result.Elements[3][1] = Matrix.Elements[3][1] * Scalar;
  Result.Elements[3][2] = Matrix.Elements[3][2] * Scalar;
  Result.Elements[3][3] = Matrix.Elements[3][3] * Scalar;
#endif

  return Result;
}

HMM_Vec4 HMM_MulM4V4(HMM_Mat4 Matrix, HMM_Vec4 Vector) {
  return HMM_LinearCombineV4M4(Vector, Matrix);
}

HMM_Mat4 HMM_DivM4F(HMM_Mat4 Matrix, float Scalar) {

  HMM_Mat4 Result;

#ifdef HANDMADE_MATH__USE_SSE
  __m128 SSEScalar = _mm_set1_ps(Scalar);
  Result.Columns[0].SSE = _mm_div_ps(Matrix.Columns[0].SSE, SSEScalar);
  Result.Columns[1].SSE = _mm_div_ps(Matrix.Columns[1].SSE, SSEScalar);
  Result.Columns[2].SSE = _mm_div_ps(Matrix.Columns[2].SSE, SSEScalar);
  Result.Columns[3].SSE = _mm_div_ps(Matrix.Columns[3].SSE, SSEScalar);
#else
  Result.Elements[0][0] = Matrix.Elements[0][0] / Scalar;
  Result.Elements[0][1] = Matrix.Elements[0][1] / Scalar;
  Result.Elements[0][2] = Matrix.Elements[0][2] / Scalar;
  Result.Elements[0][3] = Matrix.Elements[0][3] / Scalar;
  Result.Elements[1][0] = Matrix.Elements[1][0] / Scalar;
  Result.Elements[1][1] = Matrix.Elements[1][1] / Scalar;
  Result.Elements[1][2] = Matrix.Elements[1][2] / Scalar;
  Result.Elements[1][3] = Matrix.Elements[1][3] / Scalar;
  Result.Elements[2][0] = Matrix.Elements[2][0] / Scalar;
  Result.Elements[2][1] = Matrix.Elements[2][1] / Scalar;
  Result.Elements[2][2] = Matrix.Elements[2][2] / Scalar;
  Result.Elements[2][3] = Matrix.Elements[2][3] / Scalar;
  Result.Elements[3][0] = Matrix.Elements[3][0] / Scalar;
  Result.Elements[3][1] = Matrix.Elements[3][1] / Scalar;
  Result.Elements[3][2] = Matrix.Elements[3][2] / Scalar;
  Result.Elements[3][3] = Matrix.Elements[3][3] / Scalar;
#endif

  return Result;
}

float HMM_DeterminantM4(HMM_Mat4 Matrix) {

  HMM_Vec3 C01 = HMM_Cross(Matrix.Columns[0].XYZ, Matrix.Columns[1].XYZ);
  HMM_Vec3 C23 = HMM_Cross(Matrix.Columns[2].XYZ, Matrix.Columns[3].XYZ);
  HMM_Vec3 B10 = HMM_SubV3(HMM_MulV3F(Matrix.Columns[0].XYZ, Matrix.Columns[1].W), HMM_MulV3F(Matrix.Columns[1].XYZ, Matrix.Columns[0].W));
  HMM_Vec3 B32 = HMM_SubV3(HMM_MulV3F(Matrix.Columns[2].XYZ, Matrix.Columns[3].W), HMM_MulV3F(Matrix.Columns[3].XYZ, Matrix.Columns[2].W));

  return HMM_DotV3(C01, B32) + HMM_DotV3(C23, B10);
}

// Returns a general-purpose inverse of an HMM_Mat4. Note that special-purpose inverses of many transformations
// are available and will be more efficient.
HMM_Mat4 HMM_InvGeneralM4(HMM_Mat4 Matrix) {

  HMM_Vec3 C01 = HMM_Cross(Matrix.Columns[0].XYZ, Matrix.Columns[1].XYZ);
  HMM_Vec3 C23 = HMM_Cross(Matrix.Columns[2].XYZ, Matrix.Columns[3].XYZ);
  HMM_Vec3 B10 = HMM_SubV3(HMM_MulV3F(Matrix.Columns[0].XYZ, Matrix.Columns[1].W), HMM_MulV3F(Matrix.Columns[1].XYZ, Matrix.Columns[0].W));
  HMM_Vec3 B32 = HMM_SubV3(HMM_MulV3F(Matrix.Columns[2].XYZ, Matrix.Columns[3].W), HMM_MulV3F(Matrix.Columns[3].XYZ, Matrix.Columns[2].W));

  float InvDeterminant = 1.0f / (HMM_DotV3(C01, B32) + HMM_DotV3(C23, B10));
  C01 = HMM_MulV3F(C01, InvDeterminant);
  C23 = HMM_MulV3F(C23, InvDeterminant);
  B10 = HMM_MulV3F(B10, InvDeterminant);
  B32 = HMM_MulV3F(B32, InvDeterminant);

  HMM_Mat4 Result;
  Result.Columns[0] = HMM_V4V(HMM_AddV3(HMM_Cross(Matrix.Columns[1].XYZ, B32), HMM_MulV3F(C23, Matrix.Columns[1].W)), -HMM_DotV3(Matrix.Columns[1].XYZ, C23));
  Result.Columns[1] = HMM_V4V(HMM_SubV3(HMM_Cross(B32, Matrix.Columns[0].XYZ), HMM_MulV3F(C23, Matrix.Columns[0].W)), +HMM_DotV3(Matrix.Columns[0].XYZ, C23));
  Result.Columns[2] = HMM_V4V(HMM_AddV3(HMM_Cross(Matrix.Columns[3].XYZ, B10), HMM_MulV3F(C01, Matrix.Columns[3].W)), -HMM_DotV3(Matrix.Columns[3].XYZ, C01));
  Result.Columns[3] = HMM_V4V(HMM_SubV3(HMM_Cross(B10, Matrix.Columns[2].XYZ), HMM_MulV3F(C01, Matrix.Columns[2].W)), +HMM_DotV3(Matrix.Columns[2].XYZ, C01));

  return HMM_TransposeM4(Result);
}

/*
 * Common graphics transformations
 */

// Produces a right-handed orthographic projection matrix with Z ranging from -1 to 1 (the GL convention).
// Left, Right, Bottom, and Top specify the coordinates of their respective clipping planes.
// Near and Far specify the distances to the near and far clipping planes.
HMM_Mat4 HMM_Orthographic_RH_NO(float Left, float Right, float Bottom, float Top, float Near, float Far) {

  HMM_Mat4 Result = {0};

  Result.Elements[0][0] = 2.0f / (Right - Left);
  Result.Elements[1][1] = 2.0f / (Top - Bottom);
  Result.Elements[2][2] = 2.0f / (Near - Far);
  Result.Elements[3][3] = 1.0f;

  Result.Elements[3][0] = (Left + Right) / (Left - Right);
  Result.Elements[3][1] = (Bottom + Top) / (Bottom - Top);
  Result.Elements[3][2] = (Near + Far) / (Near - Far);

  return Result;
}

// Produces a right-handed orthographic projection matrix with Z ranging from 0 to 1 (the DirectX convention).
// Left, Right, Bottom, and Top specify the coordinates of their respective clipping planes.
// Near and Far specify the distances to the near and far clipping planes.
HMM_Mat4 HMM_Orthographic_RH_ZO(float Left, float Right, float Bottom, float Top, float Near, float Far) {

  HMM_Mat4 Result = {0};

  Result.Elements[0][0] = 2.0f / (Right - Left);
  Result.Elements[1][1] = 2.0f / (Top - Bottom);
  Result.Elements[2][2] = 1.0f / (Near - Far);
  Result.Elements[3][3] = 1.0f;

  Result.Elements[3][0] = (Left + Right) / (Left - Right);
  Result.Elements[3][1] = (Bottom + Top) / (Bottom - Top);
  Result.Elements[3][2] = (Near) / (Near - Far);

  return Result;
}

// Produces a left-handed orthographic projection matrix with Z ranging from -1 to 1 (the GL convention).
// Left, Right, Bottom, and Top specify the coordinates of their respective clipping planes.
// Near and Far specify the distances to the near and far clipping planes.
HMM_Mat4 HMM_Orthographic_LH_NO(float Left, float Right, float Bottom, float Top, float Near, float Far) {

  HMM_Mat4 Result = HMM_Orthographic_RH_NO(Left, Right, Bottom, Top, Near, Far);
  Result.Elements[2][2] = -Result.Elements[2][2];

  return Result;
}

// Produces a left-handed orthographic projection matrix with Z ranging from 0 to 1 (the DirectX convention).
// Left, Right, Bottom, and Top specify the coordinates of their respective clipping planes.
// Near and Far specify the distances to the near and far clipping planes.
HMM_Mat4 HMM_Orthographic_LH_ZO(float Left, float Right, float Bottom, float Top, float Near, float Far) {

  HMM_Mat4 Result = HMM_Orthographic_RH_ZO(Left, Right, Bottom, Top, Near, Far);
  Result.Elements[2][2] = -Result.Elements[2][2];

  return Result;
}

// Returns an inverse for the given orthographic projection matrix. Works for all orthographic
// projection matrices, regardless of handedness or NDC convention.
HMM_Mat4 HMM_InvOrthographic(HMM_Mat4 OrthoMatrix) {

  HMM_Mat4 Result = {0};
  Result.Elements[0][0] = 1.0f / OrthoMatrix.Elements[0][0];
  Result.Elements[1][1] = 1.0f / OrthoMatrix.Elements[1][1];
  Result.Elements[2][2] = 1.0f / OrthoMatrix.Elements[2][2];
  Result.Elements[3][3] = 1.0f;

  Result.Elements[3][0] = -OrthoMatrix.Elements[3][0] * Result.Elements[0][0];
  Result.Elements[3][1] = -OrthoMatrix.Elements[3][1] * Result.Elements[1][1];
  Result.Elements[3][2] = -OrthoMatrix.Elements[3][2] * Result.Elements[2][2];

  return Result;
}

HMM_Mat4 HMM_Orthographic_DX(float l, float r, float b, float t, float near, float far)
{
  return HMM_Orthographic_LH_ZO(l,r,b,t,near,far);
}

HMM_Mat4 HMM_Orthographic_GL(float l, float r, float b, float t, float near, float far)
{
  return HMM_Orthographic_LH_NO(l,r,b,t,near,far);
}

HMM_Mat4 HMM_Orthographic_Metal(float l, float r, float b, float t, float near, float far)
{
  HMM_Mat4 adjust = {0};
  adjust.e[0][0] = 1;
  adjust.e[1][1] = 1;
  adjust.e[2][2] = 0.5;
  adjust.e[3][2] = 0.5;
  adjust.e[3][3] = 1;
  HMM_Mat4 opengl = HMM_Orthographic_LH_ZO(l, r, b, t, near, far);
  return HMM_MulM4(opengl, adjust);
}

HMM_Mat4 HMM_Perspective_Metal(float FOV, float AspectRatio, float Near, float Far)
{
  HMM_Mat4 adjust = {0};
  adjust.e[0][0] = 1;
  adjust.e[1][1] = 1;
  adjust.e[2][2] = 0.5;
  adjust.e[3][2] = 0.5;
  adjust.e[3][3] = 1;
  HMM_Mat4 opengl = HMM_Perspective_LH_NO(FOV, AspectRatio, Near, Far);
  return HMM_MulM4(opengl,adjust);
  
}

HMM_Mat4 HMM_Perspective_RH_NO(float FOV, float AspectRatio, float Near, float Far) {

  HMM_Mat4 Result = {0};

  // See https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/gluPerspective.xml

  float Cotangent = 1.0f / HMM_TanF(FOV / 2.0f);
  Result.Elements[0][0] = Cotangent / AspectRatio;
  Result.Elements[1][1] = Cotangent;
  Result.Elements[2][3] = -1.0f;

  Result.Elements[2][2] = (Near + Far) / (Near - Far);
  Result.Elements[3][2] = (2.0f * Near * Far) / (Near - Far);

  return Result;
}

HMM_Mat4 HMM_Perspective_RH_ZO(float FOV, float AspectRatio, float Near, float Far) {

  HMM_Mat4 Result = {0};

  // See https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/gluPerspective.xml

  float Cotangent = 1.0f / HMM_TanF(FOV / 2.0f);
  Result.Elements[0][0] = Cotangent / AspectRatio;
  Result.Elements[1][1] = Cotangent;
  Result.Elements[2][3] = -1.0f;

  Result.Elements[2][2] = (Far) / (Near - Far);
  Result.Elements[3][2] = (Near * Far) / (Near - Far);

  return Result;
}

HMM_Mat4 HMM_Perspective_LH_NO(float FOV, float AspectRatio, float Near, float Far) {

  HMM_Mat4 Result = HMM_Perspective_RH_NO(FOV, AspectRatio, Near, Far);
  Result.Elements[2][2] = -Result.Elements[2][2];
  Result.Elements[2][3] = -Result.Elements[2][3];

  return Result;
}

HMM_Mat4 HMM_Perspective_LH_ZO(float FOV, float AspectRatio, float Near, float Far) {

  HMM_Mat4 Result = HMM_Perspective_RH_ZO(FOV, AspectRatio, Near, Far);
  Result.Elements[2][2] = -Result.Elements[2][2];
  Result.Elements[2][3] = -Result.Elements[2][3];

  return Result;
}

HMM_Mat4 HMM_InvPerspective_RH(HMM_Mat4 PerspectiveMatrix) {

  HMM_Mat4 Result = {0};
  Result.Elements[0][0] = 1.0f / PerspectiveMatrix.Elements[0][0];
  Result.Elements[1][1] = 1.0f / PerspectiveMatrix.Elements[1][1];
  Result.Elements[2][2] = 0.0f;

  Result.Elements[2][3] = 1.0f / PerspectiveMatrix.Elements[3][2];
  Result.Elements[3][3] = PerspectiveMatrix.Elements[2][2] * Result.Elements[2][3];
  Result.Elements[3][2] = PerspectiveMatrix.Elements[2][3];

  return Result;
}

HMM_Mat4 HMM_InvPerspective_LH(HMM_Mat4 PerspectiveMatrix) {

  HMM_Mat4 Result = {0};
  Result.Elements[0][0] = 1.0f / PerspectiveMatrix.Elements[0][0];
  Result.Elements[1][1] = 1.0f / PerspectiveMatrix.Elements[1][1];
  Result.Elements[2][2] = 0.0f;

  Result.Elements[2][3] = 1.0f / PerspectiveMatrix.Elements[3][2];
  Result.Elements[3][3] = PerspectiveMatrix.Elements[2][2] * -Result.Elements[2][3];
  Result.Elements[3][2] = PerspectiveMatrix.Elements[2][3];

  return Result;
}

HMM_Mat4 HMM_Translate(HMM_Vec3 Translation) {

  HMM_Mat4 Result = HMM_M4D(1.0f);
  Result.Elements[3][0] = Translation.X;
  Result.Elements[3][1] = Translation.Y;
  Result.Elements[3][2] = Translation.Z;

  return Result;
}

HMM_Mat4 HMM_InvTranslate(HMM_Mat4 TranslationMatrix) {

  HMM_Mat4 Result = TranslationMatrix;
  Result.Elements[3][0] = -Result.Elements[3][0];
  Result.Elements[3][1] = -Result.Elements[3][1];
  Result.Elements[3][2] = -Result.Elements[3][2];

  return Result;
}

HMM_Mat4 HMM_Rotate_RH(float Angle, HMM_Vec3 Axis) {

  HMM_Mat4 Result = HMM_M4D(1.0f);

  Axis = HMM_NormV3(Axis);

  float SinTheta = HMM_SinF(Angle);
  float CosTheta = HMM_CosF(Angle);
  float CosValue = 1.0f - CosTheta;

  Result.Elements[0][0] = (Axis.X * Axis.X * CosValue) + CosTheta;
  Result.Elements[0][1] = (Axis.X * Axis.Y * CosValue) + (Axis.Z * SinTheta);
  Result.Elements[0][2] = (Axis.X * Axis.Z * CosValue) - (Axis.Y * SinTheta);

  Result.Elements[1][0] = (Axis.Y * Axis.X * CosValue) - (Axis.Z * SinTheta);
  Result.Elements[1][1] = (Axis.Y * Axis.Y * CosValue) + CosTheta;
  Result.Elements[1][2] = (Axis.Y * Axis.Z * CosValue) + (Axis.X * SinTheta);

  Result.Elements[2][0] = (Axis.Z * Axis.X * CosValue) + (Axis.Y * SinTheta);
  Result.Elements[2][1] = (Axis.Z * Axis.Y * CosValue) - (Axis.X * SinTheta);
  Result.Elements[2][2] = (Axis.Z * Axis.Z * CosValue) + CosTheta;

  return Result;
}

HMM_Mat4 HMM_Rotate_LH(float Angle, HMM_Vec3 Axis) {
  /* NOTE(lcf): Matrix will be inverse/transpose of RH. */
  return HMM_Rotate_RH(-Angle, Axis);
}

HMM_Mat4 HMM_InvRotate(HMM_Mat4 RotationMatrix) {
  return HMM_TransposeM4(RotationMatrix);
}

HMM_Mat4 HMM_Scale(HMM_Vec3 Scale) {

  HMM_Mat4 Result = HMM_M4D(1.0f);
  Result.Elements[0][0] = Scale.X;
  Result.Elements[1][1] = Scale.Y;
  Result.Elements[2][2] = Scale.Z;

  return Result;
}

HMM_Mat4 HMM_InvScale(HMM_Mat4 ScaleMatrix) {

  HMM_Mat4 Result = ScaleMatrix;
  Result.Elements[0][0] = 1.0f / Result.Elements[0][0];
  Result.Elements[1][1] = 1.0f / Result.Elements[1][1];
  Result.Elements[2][2] = 1.0f / Result.Elements[2][2];

  return Result;
}

HMM_Mat4 _HMM_LookAt(HMM_Vec3 F, HMM_Vec3 S, HMM_Vec3 U, HMM_Vec3 Eye) {
  HMM_Mat4 Result;

  Result.Elements[0][0] = S.X;
  Result.Elements[0][1] = U.X;
  Result.Elements[0][2] = -F.X;
  Result.Elements[0][3] = 0.0f;

  Result.Elements[1][0] = S.Y;
  Result.Elements[1][1] = U.Y;
  Result.Elements[1][2] = -F.Y;
  Result.Elements[1][3] = 0.0f;

  Result.Elements[2][0] = S.Z;
  Result.Elements[2][1] = U.Z;
  Result.Elements[2][2] = -F.Z;
  Result.Elements[2][3] = 0.0f;

  Result.Elements[3][0] = -HMM_DotV3(S, Eye);
  Result.Elements[3][1] = -HMM_DotV3(U, Eye);
  Result.Elements[3][2] = HMM_DotV3(F, Eye);
  Result.Elements[3][3] = 1.0f;

  return Result;
}

HMM_Mat4 HMM_LookAt_RH(HMM_Vec3 Eye, HMM_Vec3 Center, HMM_Vec3 Up) {

  HMM_Vec3 F = HMM_NormV3(HMM_SubV3(Center, Eye));
  HMM_Vec3 S = HMM_NormV3(HMM_Cross(F, Up));
  HMM_Vec3 U = HMM_Cross(S, F);

  return _HMM_LookAt(F, S, U, Eye);
}

HMM_Mat4 HMM_LookAt_LH(HMM_Vec3 Eye, HMM_Vec3 Center, HMM_Vec3 Up) {

  HMM_Vec3 F = HMM_NormV3(HMM_SubV3(Eye, Center));
  HMM_Vec3 S = HMM_NormV3(HMM_Cross(F, Up));
  HMM_Vec3 U = HMM_Cross(S, F);

  return _HMM_LookAt(F, S, U, Eye);
}

HMM_Mat4 HMM_InvLookAt(HMM_Mat4 Matrix) {
  HMM_Mat4 Result;

  HMM_Mat3 Rotation = {0};
  Rotation.Columns[0] = Matrix.Columns[0].XYZ;
  Rotation.Columns[1] = Matrix.Columns[1].XYZ;
  Rotation.Columns[2] = Matrix.Columns[2].XYZ;
  Rotation = HMM_TransposeM3(Rotation);

  Result.Columns[0] = HMM_V4V(Rotation.Columns[0], 0.0f);
  Result.Columns[1] = HMM_V4V(Rotation.Columns[1], 0.0f);
  Result.Columns[2] = HMM_V4V(Rotation.Columns[2], 0.0f);
  Result.Columns[3] = HMM_MulV4F(Matrix.Columns[3], -1.0f);
  Result.Elements[3][0] = -1.0f * Matrix.Elements[3][0] /
                          (Rotation.Elements[0][0] + Rotation.Elements[0][1] + Rotation.Elements[0][2]);
  Result.Elements[3][1] = -1.0f * Matrix.Elements[3][1] /
                          (Rotation.Elements[1][0] + Rotation.Elements[1][1] + Rotation.Elements[1][2]);
  Result.Elements[3][2] = -1.0f * Matrix.Elements[3][2] /
                          (Rotation.Elements[2][0] + Rotation.Elements[2][1] + Rotation.Elements[2][2]);
  Result.Elements[3][3] = 1.0f;

  return Result;
}

/*
 * Quaternion operations
 */

HMM_Vec3 HMM_QVRot(HMM_Vec3 v, HMM_Quat q)
{
  HMM_Vec3 t = HMM_MulV3F(HMM_Cross(q.XYZ, v), 2);
  HMM_Vec3 r = HMM_AddV3(v, HMM_MulV3F(t, q.W));
  r = HMM_AddV3(r, HMM_Cross(q.XYZ, t));
  return r;
}

HMM_Quat HMM_Q(float X, float Y, float Z, float W) {

  HMM_Quat Result;

#ifdef HANDMADE_MATH__USE_SSE
  Result.SSE = _mm_setr_ps(X, Y, Z, W);
#else
  Result.X = X;
  Result.Y = Y;
  Result.Z = Z;
  Result.W = W;
#endif

  return Result;
}

HMM_Quat HMM_QV4(HMM_Vec4 Vector) {

  HMM_Quat Result;

#ifdef HANDMADE_MATH__USE_SSE
  Result.SSE = Vector.SSE;
#else
  Result.X = Vector.X;
  Result.Y = Vector.Y;
  Result.Z = Vector.Z;
  Result.W = Vector.W;
#endif

  return Result;
}

HMM_Quat HMM_AddQ(HMM_Quat Left, HMM_Quat Right) {

  HMM_Quat Result;

#ifdef HANDMADE_MATH__USE_SSE
  Result.SSE = _mm_add_ps(Left.SSE, Right.SSE);
#else

  Result.X = Left.X + Right.X;
  Result.Y = Left.Y + Right.Y;
  Result.Z = Left.Z + Right.Z;
  Result.W = Left.W + Right.W;
#endif

  return Result;
}

HMM_Quat HMM_SubQ(HMM_Quat Left, HMM_Quat Right) {

  HMM_Quat Result;

#ifdef HANDMADE_MATH__USE_SSE
  Result.SSE = _mm_sub_ps(Left.SSE, Right.SSE);
#else
  Result.X = Left.X - Right.X;
  Result.Y = Left.Y - Right.Y;
  Result.Z = Left.Z - Right.Z;
  Result.W = Left.W - Right.W;
#endif

  return Result;
}

HMM_Quat HMM_MulQ(HMM_Quat Left, HMM_Quat Right) {

  HMM_Quat Result;

#ifdef HANDMADE_MATH__USE_SSE
  __m128 SSEResultOne = _mm_xor_ps(_mm_shuffle_ps(Left.SSE, Left.SSE, _MM_SHUFFLE(0, 0, 0, 0)), _mm_setr_ps(0.f, -0.f, 0.f, -0.f));
  __m128 SSEResultTwo = _mm_shuffle_ps(Right.SSE, Right.SSE, _MM_SHUFFLE(0, 1, 2, 3));
  __m128 SSEResultThree = _mm_mul_ps(SSEResultTwo, SSEResultOne);

  SSEResultOne = _mm_xor_ps(_mm_shuffle_ps(Left.SSE, Left.SSE, _MM_SHUFFLE(1, 1, 1, 1)), _mm_setr_ps(0.f, 0.f, -0.f, -0.f));
  SSEResultTwo = _mm_shuffle_ps(Right.SSE, Right.SSE, _MM_SHUFFLE(1, 0, 3, 2));
  SSEResultThree = _mm_add_ps(SSEResultThree, _mm_mul_ps(SSEResultTwo, SSEResultOne));

  SSEResultOne = _mm_xor_ps(_mm_shuffle_ps(Left.SSE, Left.SSE, _MM_SHUFFLE(2, 2, 2, 2)), _mm_setr_ps(-0.f, 0.f, 0.f, -0.f));
  SSEResultTwo = _mm_shuffle_ps(Right.SSE, Right.SSE, _MM_SHUFFLE(2, 3, 0, 1));
  SSEResultThree = _mm_add_ps(SSEResultThree, _mm_mul_ps(SSEResultTwo, SSEResultOne));

  SSEResultOne = _mm_shuffle_ps(Left.SSE, Left.SSE, _MM_SHUFFLE(3, 3, 3, 3));
  SSEResultTwo = _mm_shuffle_ps(Right.SSE, Right.SSE, _MM_SHUFFLE(3, 2, 1, 0));
  Result.SSE = _mm_add_ps(SSEResultThree, _mm_mul_ps(SSEResultTwo, SSEResultOne));
#else
  Result.X = Right.Elements[3] * +Left.Elements[0];
  Result.Y = Right.Elements[2] * -Left.Elements[0];
  Result.Z = Right.Elements[1] * +Left.Elements[0];
  Result.W = Right.Elements[0] * -Left.Elements[0];

  Result.X += Right.Elements[2] * +Left.Elements[1];
  Result.Y += Right.Elements[3] * +Left.Elements[1];
  Result.Z += Right.Elements[0] * -Left.Elements[1];
  Result.W += Right.Elements[1] * -Left.Elements[1];

  Result.X += Right.Elements[1] * -Left.Elements[2];
  Result.Y += Right.Elements[0] * +Left.Elements[2];
  Result.Z += Right.Elements[3] * +Left.Elements[2];
  Result.W += Right.Elements[2] * -Left.Elements[2];

  Result.X += Right.Elements[0] * +Left.Elements[3];
  Result.Y += Right.Elements[1] * +Left.Elements[3];
  Result.Z += Right.Elements[2] * +Left.Elements[3];
  Result.W += Right.Elements[3] * +Left.Elements[3];
#endif

  return Result;
}

HMM_Quat HMM_MulQF(HMM_Quat Left, float Multiplicative) {

  HMM_Quat Result;

#ifdef HANDMADE_MATH__USE_SSE
  __m128 Scalar = _mm_set1_ps(Multiplicative);
  Result.SSE = _mm_mul_ps(Left.SSE, Scalar);
#else
  Result.X = Left.X * Multiplicative;
  Result.Y = Left.Y * Multiplicative;
  Result.Z = Left.Z * Multiplicative;
  Result.W = Left.W * Multiplicative;
#endif

  return Result;
}

HMM_Quat HMM_DivQF(HMM_Quat Left, float Divnd) {

  HMM_Quat Result;

#ifdef HANDMADE_MATH__USE_SSE
  __m128 Scalar = _mm_set1_ps(Divnd);
  Result.SSE = _mm_div_ps(Left.SSE, Scalar);
#else
  Result.X = Left.X / Divnd;
  Result.Y = Left.Y / Divnd;
  Result.Z = Left.Z / Divnd;
  Result.W = Left.W / Divnd;
#endif

  return Result;
}

float HMM_DotQ(HMM_Quat Left, HMM_Quat Right) {

  float Result;

#ifdef HANDMADE_MATH__USE_SSE
  __m128 SSEResultOne = _mm_mul_ps(Left.SSE, Right.SSE);
  __m128 SSEResultTwo = _mm_shuffle_ps(SSEResultOne, SSEResultOne, _MM_SHUFFLE(2, 3, 0, 1));
  SSEResultOne = _mm_add_ps(SSEResultOne, SSEResultTwo);
  SSEResultTwo = _mm_shuffle_ps(SSEResultOne, SSEResultOne, _MM_SHUFFLE(0, 1, 2, 3));
  SSEResultOne = _mm_add_ps(SSEResultOne, SSEResultTwo);
  _mm_store_ss(&Result, SSEResultOne);
#else
  Result = ((Left.X * Right.X) + (Left.Z * Right.Z)) + ((Left.Y * Right.Y) + (Left.W * Right.W));
#endif

  return Result;
}

HMM_Quat HMM_InvQ(HMM_Quat Left) {

  HMM_Quat Result;
  Result.X = -Left.X;
  Result.Y = -Left.Y;
  Result.Z = -Left.Z;
  Result.W = Left.W;

  return HMM_DivQF(Result, (HMM_DotQ(Left, Left)));
}

HMM_Quat HMM_NormQ(HMM_Quat Quat) {

  /* NOTE(lcf): Take advantage of SSE implementation in HMM_NormV4 */
  HMM_Vec4 Vec = {Quat.X, Quat.Y, Quat.Z, Quat.W};
  Vec = HMM_NormV4(Vec);
  HMM_Quat Result = {Vec.X, Vec.Y, Vec.Z, Vec.W};

  return Result;
}

HMM_Quat _HMM_MixQ(HMM_Quat Left, float MixLeft, HMM_Quat Right, float MixRight) {
  HMM_Quat Result;

#ifdef HANDMADE_MATH__USE_SSE
  __m128 ScalarLeft = _mm_set1_ps(MixLeft);
  __m128 ScalarRight = _mm_set1_ps(MixRight);
  __m128 SSEResultOne = _mm_mul_ps(Left.SSE, ScalarLeft);
  __m128 SSEResultTwo = _mm_mul_ps(Right.SSE, ScalarRight);
  Result.SSE = _mm_add_ps(SSEResultOne, SSEResultTwo);
#else
  Result.X = Left.X * MixLeft + Right.X * MixRight;
  Result.Y = Left.Y * MixLeft + Right.Y * MixRight;
  Result.Z = Left.Z * MixLeft + Right.Z * MixRight;
  Result.W = Left.W * MixLeft + Right.W * MixRight;
#endif

  return Result;
}

HMM_Quat HMM_NLerp(HMM_Quat Left, float Time, HMM_Quat Right) {

  HMM_Quat Result = _HMM_MixQ(Left, 1.0f - Time, Right, Time);
  Result = HMM_NormQ(Result);

  return Result;
}

HMM_Quat HMM_SLerp(HMM_Quat Left, float Time, HMM_Quat Right) {

  HMM_Quat Result;

  float Cos_Theta = HMM_DotQ(Left, Right);

  if (Cos_Theta < 0.0f) { /* NOTE(lcf): Take shortest path on Hyper-sphere */
    Cos_Theta = -Cos_Theta;
    Right = HMM_Q(-Right.X, -Right.Y, -Right.Z, -Right.W);
  }

  /* NOTE(lcf): Use Normalized Linear interpolation when vectors are roughly not L.I. */
  if (Cos_Theta > 0.9995f) {
    Result = HMM_NLerp(Left, Time, Right);
  } else {
    float Angle = HMM_ACosF(Cos_Theta);
    float MixLeft = HMM_SinF((1.0f - Time) * Angle);
    float MixRight = HMM_SinF(Time * Angle);

    Result = _HMM_MixQ(Left, MixLeft, Right, MixRight);
    Result = HMM_NormQ(Result);
  }

  return Result;
}

HMM_Mat4 HMM_QToM4(HMM_Quat Left) {

  HMM_Mat4 Result;

  HMM_Quat NormalizedQ = HMM_NormQ(Left);

  float XX, YY, ZZ,
      XY, XZ, YZ,
      WX, WY, WZ;

  XX = NormalizedQ.X * NormalizedQ.X;
  YY = NormalizedQ.Y * NormalizedQ.Y;
  ZZ = NormalizedQ.Z * NormalizedQ.Z;
  XY = NormalizedQ.X * NormalizedQ.Y;
  XZ = NormalizedQ.X * NormalizedQ.Z;
  YZ = NormalizedQ.Y * NormalizedQ.Z;
  WX = NormalizedQ.W * NormalizedQ.X;
  WY = NormalizedQ.W * NormalizedQ.Y;
  WZ = NormalizedQ.W * NormalizedQ.Z;

  Result.Elements[0][0] = 1.0f - 2.0f * (YY + ZZ);
  Result.Elements[0][1] = 2.0f * (XY + WZ);
  Result.Elements[0][2] = 2.0f * (XZ - WY);
  Result.Elements[0][3] = 0.0f;

  Result.Elements[1][0] = 2.0f * (XY - WZ);
  Result.Elements[1][1] = 1.0f - 2.0f * (XX + ZZ);
  Result.Elements[1][2] = 2.0f * (YZ + WX);
  Result.Elements[1][3] = 0.0f;

  Result.Elements[2][0] = 2.0f * (XZ + WY);
  Result.Elements[2][1] = 2.0f * (YZ - WX);
  Result.Elements[2][2] = 1.0f - 2.0f * (XX + YY);
  Result.Elements[2][3] = 0.0f;

  Result.Elements[3][0] = 0.0f;
  Result.Elements[3][1] = 0.0f;
  Result.Elements[3][2] = 0.0f;
  Result.Elements[3][3] = 1.0f;

  return Result;
}

// this is right handed
HMM_Mat4 HMM_M4TRS(HMM_Vec3 t, HMM_Quat q, HMM_Vec3 s)
{
  HMM_Mat4 l;
  float *lm = (float*)&l;
  
  lm[0] = (1 - 2 * q.y*q.y - 2 * q.z*q.z) * s.x;
  lm[1] = (2 * q.x*q.y + 2 * q.z*q.w) * s.x;
  lm[2] = (2 * q.x*q.z - 2 * q.y*q.w) * s.x;
  lm[3] = 0.f;
  
  lm[4] = (2 * q.x*q.y - 2 * q.z*q.w) * s.y;
  lm[5] = (1 - 2 * q.x*q.x - 2 * q.z*q.z) * s.y;
  lm[6] = (2 * q.y*q.z + 2 * q.x*q.w) * s.y;
  lm[7] = 0.f;
  
  lm[8] = (2 * q.x*q.z + 2 * q.y*q.w) * s.z;
  lm[9] = (2 * q.y*q.z - 2 * q.x*q.w) * s.z;
  lm[10] = (1 - 2 * q.x*q.x - 2 * q.y*q.y) * s.z;
  lm[11] = 0.f;
  
  lm[12] = t.x;
  lm[13] = t.y;
  lm[14] = t.z;
  lm[15] = 1.f;
  
  return l;
}

// This method taken from Mike Day at Insomniac Games.
// https://d3cw3dd2w32x2b.cloudfront.net/wp-content/uploads/2015/01/matrix-to-quat.pdf
//
// Note that as mentioned at the top of the paper, the paper assumes the matrix
// would be *post*-multiplied to a vector to rotate it, meaning the matrix is
// the transpose of what we're dealing with. But, because our matrices are
// stored in column-major order, the indices *appear* to match the paper.
//
// For example, m12 in the paper is row 1, column 2. We need to transpose it to
// row 2, column 1. But, because the column comes first when referencing
// elements, it looks like M.Elements[1][2].
//
// Don't be confused! Or if you must be confused, at least trust this
// comment. :)
HMM_Quat HMM_M4ToQ_RH(HMM_Mat4 M) {
  float T;
  HMM_Quat Q;

  if (M.Elements[2][2] < 0.0f) {
    if (M.Elements[0][0] > M.Elements[1][1]) {

      T = 1 + M.Elements[0][0] - M.Elements[1][1] - M.Elements[2][2];
      Q = HMM_Q(
          T,
          M.Elements[0][1] + M.Elements[1][0],
          M.Elements[2][0] + M.Elements[0][2],
          M.Elements[1][2] - M.Elements[2][1]);
    } else {

      T = 1 - M.Elements[0][0] + M.Elements[1][1] - M.Elements[2][2];
      Q = HMM_Q(
          M.Elements[0][1] + M.Elements[1][0],
          T,
          M.Elements[1][2] + M.Elements[2][1],
          M.Elements[2][0] - M.Elements[0][2]);
    }
  } else {
    if (M.Elements[0][0] < -M.Elements[1][1]) {

      T = 1 - M.Elements[0][0] - M.Elements[1][1] + M.Elements[2][2];
      Q = HMM_Q(
          M.Elements[2][0] + M.Elements[0][2],
          M.Elements[1][2] + M.Elements[2][1],
          T,
          M.Elements[0][1] - M.Elements[1][0]);
    } else {

      T = 1 + M.Elements[0][0] + M.Elements[1][1] + M.Elements[2][2];
      Q = HMM_Q(
          M.Elements[1][2] - M.Elements[2][1],
          M.Elements[2][0] - M.Elements[0][2],
          M.Elements[0][1] - M.Elements[1][0],
          T);
    }
  }

  Q = HMM_MulQF(Q, 0.5f / HMM_SqrtF(T));

  return Q;
}

HMM_Quat HMM_M4ToQ_LH(HMM_Mat4 M) {
  float T;
  HMM_Quat Q;

  if (M.Elements[2][2] < 0.0f) {
    if (M.Elements[0][0] > M.Elements[1][1]) {

      T = 1 + M.Elements[0][0] - M.Elements[1][1] - M.Elements[2][2];
      Q = HMM_Q(
          T,
          M.Elements[0][1] + M.Elements[1][0],
          M.Elements[2][0] + M.Elements[0][2],
          M.Elements[2][1] - M.Elements[1][2]);
    } else {

      T = 1 - M.Elements[0][0] + M.Elements[1][1] - M.Elements[2][2];
      Q = HMM_Q(
          M.Elements[0][1] + M.Elements[1][0],
          T,
          M.Elements[1][2] + M.Elements[2][1],
          M.Elements[0][2] - M.Elements[2][0]);
    }
  } else {
    if (M.Elements[0][0] < -M.Elements[1][1]) {

      T = 1 - M.Elements[0][0] - M.Elements[1][1] + M.Elements[2][2];
      Q = HMM_Q(
          M.Elements[2][0] + M.Elements[0][2],
          M.Elements[1][2] + M.Elements[2][1],
          T,
          M.Elements[1][0] - M.Elements[0][1]);
    } else {

      T = 1 + M.Elements[0][0] + M.Elements[1][1] + M.Elements[2][2];
      Q = HMM_Q(
          M.Elements[2][1] - M.Elements[1][2],
          M.Elements[0][2] - M.Elements[2][0],
          M.Elements[1][0] - M.Elements[0][2],
          T);
    }
  }

  Q = HMM_MulQF(Q, 0.5f / HMM_SqrtF(T));

  return Q;
}

float HMM_Q_Roll(HMM_Quat q)
{
  return atan2(2.0*(q.x*q.y + q.w*q.z), q.w*q.w + q.x*q.x - q.y*q.y - q.z*q.z);
}

float HMM_Q_Pitch(HMM_Quat q)
{
  return asin(-2.0*(q.x*q.z - q.w*q.y));
}

float HMM_Q_Yaw(HMM_Quat q)
{
  return atan2(2.0*(q.y*q.z + q.w*q.x), q.w*q.w - q.x*q.x - q.y*q.y + q.z*q.z);
}

HMM_Quat HMM_QFromAxisAngle_RH(HMM_Vec3 Axis, float AngleOfRotation) {

  HMM_Quat Result;

  HMM_Vec3 AxisNormalized = HMM_NormV3(Axis);
  float SineOfRotation = HMM_SinF(AngleOfRotation / 2.0f);

  Result.XYZ = HMM_MulV3F(AxisNormalized, SineOfRotation);
  Result.W = HMM_CosF(AngleOfRotation / 2.0f);

  return Result;
}

HMM_Quat HMM_QFromAxisAngle_LH(HMM_Vec3 Axis, float AngleOfRotation) {

  return HMM_QFromAxisAngle_RH(Axis, -AngleOfRotation);
}

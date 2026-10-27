// Макет математики UE 4.27: FVector и прочие — на float (не double, как в UE5). FVector3f/FVector3d, UE_PI и т. п. отсутствуют.
#pragma once
// TP_STUB_MODULE: Core

#undef PI
#define PI (3.1415926535897932f)
#define SMALL_NUMBER (1.e-8f)
#define KINDA_SMALL_NUMBER (1.e-4f)
#define BIG_NUMBER (3.4e+38f)
#define EULERS_NUMBER (2.71828182845904523536f)
#define UE_GOLDEN_RATIO (1.6180339887498948482045868343656381f)
#define FLOAT_NON_FRACTIONAL (8388608.f)
#define DOUBLE_PI (3.141592653589793238462643383279502884197169399)
#define INV_PI (0.31830988618f)
#define HALF_PI (1.57079632679f)
#define TWO_PI (6.28318530717f)
#define PI_SQUARED (9.86960440108f)
#define DELTA (0.00001f)
#define FLOAT_NORMAL_THRESH (0.0001f)
#define THRESH_POINT_ON_PLANE (0.10f)
#define THRESH_NORMALS_ARE_PARALLEL (0.999845f)
#define THRESH_NORMALS_ARE_ORTHOGONAL (0.017455f)
#define THRESH_VECTOR_NORMALIZED (0.01f)
#define THRESH_QUAT_NORMALIZED (0.01f)
#define UE_SQRT_2 (1.4142135623730950488016887242097f)
#define UE_HALF_SQRT_2 (0.70710678118654752440084436210485f)

namespace EAxis { enum Type { None, X, Y, Z }; }
namespace EAxisList { enum Type { None = 0, X = 1, Y = 2, Z = 4, Screen = 8, XY = X | Y, XZ = X | Z, YZ = Y | Z, XYZ = X | Y | Z, All = XYZ | Screen }; }

struct FVector; struct FVector2D; struct FVector4; struct FRotator; struct FQuat; struct FMatrix; struct FTransform; struct FLinearColor; struct FColor; struct FIntPoint; struct FIntVector; struct FPlane; struct FBox; struct FBox2D; struct FSphere;

struct FMath
{
	// Базовые (UnrealMathUtility.h 4.27): шаблоны с одним T — смешивать float и double нельзя.
	template<class T> static constexpr T Abs(const T A) { return (A >= (T)0) ? A : -A; }
	template<class T> static constexpr T Sign(const T A) { return (A > (T)0) ? (T)1 : ((A < (T)0) ? (T)-1 : (T)0); }
	template<class T> static constexpr T Max(const T A, const T B) { return (A >= B) ? A : B; }
	template<class T> static constexpr T Min(const T A, const T B) { return (A <= B) ? A : B; }
	template<class T, class A> static T Max(const TArray<T, A>& Values, int32* MinIndex = nullptr);
	template<class T, class A> static T Min(const TArray<T, A>& Values, int32* MinIndex = nullptr);
	template<class T> static T Max3(const T A, const T B, const T C) { return Max(Max(A, B), C); }
	template<class T> static T Min3(const T A, const T B, const T C) { return Min(Min(A, B), C); }
	template<class T> static T Square(const T A) { return A * A; }
	template<class T> static T Cube(const T A) { return A * A * A; }
	template<class T> static T Clamp(const T X, const T Min, const T Max) { return X < Min ? Min : X < Max ? X : Max; }
	template<class T> static T Wrap(const T X, const T Min, const T Max);
	template<class T> static T DivideAndRoundUp(T Dividend, T Divisor) { return (Dividend + Divisor - 1) / Divisor; }
	template<class T> static T DivideAndRoundDown(T Dividend, T Divisor) { return Dividend / Divisor; }
	template<class T> static bool IsPowerOfTwo(T Value) { return ((Value & (Value - 1)) == (T)0); }
	template<class T> static bool IsWithin(const T& TestValue, const T& MinValue, const T& MaxValue) { return ((TestValue >= MinValue) && (TestValue < MaxValue)); }
	template<class T> static bool IsWithinInclusive(const T& TestValue, const T& MinValue, const T& MaxValue) { return ((TestValue >= MinValue) && (TestValue <= MaxValue)); }
	template<class T> static T GridSnap(T Location, T Grid);
	template<class T, class U> static T Lerp(const T& A, const T& B, const U& Alpha) { return (T)(A + Alpha * (B - A)); }
	template<class T, class U> static T LerpStable(const T& A, const T& B, const U& Alpha) { return (T)((A * (1.0f - Alpha)) + (B * Alpha)); }
	template<class T, class U> static T BiLerp(const T& P00, const T& P10, const T& P01, const T& P11, const U& FracX, const U& FracY);
	template<class T, class U> static T CubicInterp(const T& P0, const T& T0, const T& P1, const T& T1, const U& A);
	template<class T> static T InterpEaseIn(const T& A, const T& B, float Alpha, float Exp);
	template<class T> static T InterpEaseOut(const T& A, const T& B, float Alpha, float Exp);
	template<class T> static T InterpEaseInOut(const T& A, const T& B, float Alpha, float Exp);
	template<class T> static T InterpSinIn(const T& A, const T& B, float Alpha);
	template<class T> static T InterpSinOut(const T& A, const T& B, float Alpha);
	template<class T> static T InterpSinInOut(const T& A, const T& B, float Alpha);
	template<class T> static T InterpExpoInOut(const T& A, const T& B, float Alpha);
	template<class T> static T InterpCircularInOut(const T& A, const T& B, float Alpha);
	template<class T> static T InterpStep(const T& A, const T& B, float Alpha, int32 Steps);
	template<class U> static FRotator Lerp(const FRotator& A, const FRotator& B, const U& Alpha);
	template<class U> static FQuat Lerp(const FQuat& A, const FQuat& B, const U& Alpha);
	template<class T> static auto RadiansToDegrees(T const& RadVal) -> decltype(RadVal * (180.f / PI)) { return RadVal * (180.f / PI); }
	template<class T> static auto DegreesToRadians(T const& DegVal) -> decltype(DegVal * (PI / 180.f)) { return DegVal * (PI / 180.f); }

	static float Sqrt(float Value);
	static float InvSqrt(float F); static float InvSqrtEst(float F);
	static float Sin(float Value);
	static float Cos(float Value);
	static float Tan(float Value);
	static float Asin(float Value);
	static float Acos(float Value);
	static float Atan(float Value);
	static float Atan2(float Y, float X);
	static float Sinh(float Value);
	static void SinCos(float* ScalarSin, float* ScalarCos, float Value);
	static float FastAsin(float Value);
	static float Exp(float Value);
	static float Exp2(float Value);
	static float Loge(float Value);
	static float LogX(float Base, float Value);
	static float Log2(float Value);
	static float Pow(float A, float B);
	static float Fmod(float X, float Y);
	static float Fractional(float Value);
	static float Frac(float Value);
	static float Modf(const float InValue, float* OutIntPart);
	static int32 TruncToInt(float F);
	static float TruncToFloat(float F);
	static int32 FloorToInt(float F);
	static float FloorToFloat(float F); static double FloorToDouble(double F);
	static int32 RoundToInt(float F);
	static float RoundToFloat(float F); static double RoundToDouble(double F);
	static int32 CeilToInt(float F);
	static float CeilToFloat(float F); static double CeilToDouble(double F);
	static float RoundHalfFromZero(float F); static float RoundHalfToZero(float F); static float RoundFromZero(float F); static float RoundToZero(float F);
	static bool IsNaN(float A);
	static bool IsFinite(float A);
	static bool IsNegativeFloat(const float& A);
	static bool IsNearlyEqual(float A, float B, float ErrorTolerance = SMALL_NUMBER);
	static bool IsNearlyEqual(double A, double B, double ErrorTolerance = SMALL_NUMBER);
	static bool IsNearlyZero(float Value, float ErrorTolerance = SMALL_NUMBER);
	static bool IsNearlyZero(double Value, double ErrorTolerance = SMALL_NUMBER);
	static float FloatSelect(float Comparand, float ValueGEZero, float ValueLTZero);
	static uint32 FloorLog2(uint32 Value); static uint32 CeilLogTwo(uint32 Arg); static uint32 RoundUpToPowerOfTwo(uint32 Arg);
	static uint32 CountBits(uint64 Bits);

	static int32 Rand(); static void RandInit(int32 Seed); static float FRand(); static void SRandInit(int32 Seed); static int32 GetRandSeed(); static float SRand();
	static int32 RandHelper(int32 A);
	static int32 RandRange(int32 Min, int32 Max);
	static float RandRange(float InMin, float InMax);
	static float FRandRange(float InMin, float InMax);
	static bool RandBool();
	static FVector VRand();
	static FVector VRandCone(FVector const& Dir, float ConeHalfAngleRad);
	static FVector VRandCone(FVector const& Dir, float HorizontalConeHalfAngleRad, float VerticalConeHalfAngleRad);
	static FVector2D RandPointInCircle(float CircleRadius);
	static FVector RandPointInBox(const FBox& Box);

	static float ClampAngle(float AngleDegrees, float MinAngleDegrees, float MaxAngleDegrees);
	static float UnwindDegrees(float A); static float UnwindRadians(float A);
	static void WindRelativeAnglesDegrees(float InAngle0, float& InOutAngle1);
	static float FindDeltaAngleDegrees(float A1, float A2);
	static float FindDeltaAngleRadians(float A1, float A2);
	static float FixedTurn(float InCurrent, float InDesired, float InDeltaRate);
	static float GetRangePct(float MinValue, float MaxValue, float Value);
	static float GetRangePct(FVector2D const& Range, float Value);
	static float GetRangeValue(FVector2D const& Range, float Pct);
	static float GetMappedRangeValueClamped(const FVector2D& InputRange, const FVector2D& OutputRange, const float Value);
	static float GetMappedRangeValueUnclamped(const FVector2D& InputRange, const FVector2D& OutputRange, const float Value);
	static float SmoothStep(float A, float B, float X);
	static float FInterpTo(float Current, float Target, float DeltaTime, float InterpSpeed);
	static float FInterpConstantTo(float Current, float Target, float DeltaTime, float InterpSpeed);
	static FVector VInterpTo(const FVector& Current, const FVector& Target, float DeltaTime, float InterpSpeed);
	static FVector VInterpConstantTo(const FVector& Current, const FVector& Target, float DeltaTime, float InterpSpeed);
	static FVector VInterpNormalRotationTo(const FVector& Current, const FVector& Target, float DeltaTime, float RotationSpeedDegrees);
	static FVector2D Vector2DInterpTo(const FVector2D& Current, const FVector2D& Target, float DeltaTime, float InterpSpeed);
	static FVector2D Vector2DInterpConstantTo(const FVector2D& Current, const FVector2D& Target, float DeltaTime, float InterpSpeed);
	static FRotator RInterpTo(const FRotator& Current, const FRotator& Target, float DeltaTime, float InterpSpeed);
	static FRotator RInterpConstantTo(const FRotator& Current, const FRotator& Target, float DeltaTime, float InterpSpeed);
	static FQuat QInterpTo(const FQuat& Current, const FQuat& Target, float DeltaTime, float InterpSpeed);
	static FQuat QInterpConstantTo(const FQuat& Current, const FQuat& Target, float DeltaTime, float InterpSpeed);
	static FLinearColor CInterpTo(const FLinearColor& Current, const FLinearColor& Target, float DeltaTime, float InterpSpeed);
	static float PerlinNoise1D(const float Value);
	static float PerlinNoise2D(const FVector2D& Location);
	static float PerlinNoise3D(const FVector& Location);
	static FVector ClosestPointOnLine(const FVector& LineStart, const FVector& LineEnd, const FVector& Point);
	static FVector ClosestPointOnInfiniteLine(const FVector& LineStart, const FVector& LineEnd, const FVector& Point);
	static FVector ClosestPointOnSegment(const FVector& Point, const FVector& StartPoint, const FVector& EndPoint);
	static FVector2D ClosestPointOnSegment2D(const FVector2D& Point, const FVector2D& StartPoint, const FVector2D& EndPoint);
	static float PointDistToLine(const FVector& Point, const FVector& Direction, const FVector& Origin);
	static float PointDistToSegment(const FVector& Point, const FVector& StartPoint, const FVector& EndPoint);
	static float PointDistToSegmentSquared(const FVector& Point, const FVector& StartPoint, const FVector& EndPoint);
	static void SegmentDistToSegment(FVector A1, FVector B1, FVector A2, FVector B2, FVector& OutP1, FVector& OutP2);
	static void SegmentDistToSegmentSafe(FVector A1, FVector B1, FVector A2, FVector B2, FVector& OutP1, FVector& OutP2);
	static bool SegmentIntersection2D(const FVector& SegmentStartA, const FVector& SegmentEndA, const FVector& SegmentStartB, const FVector& SegmentEndB, FVector& out_IntersectionPoint);
	static FVector LinePlaneIntersection(const FVector& Point1, const FVector& Point2, const FVector& PlaneOrigin, const FVector& PlaneNormal);
	static FVector LinePlaneIntersection(const FVector& Point1, const FVector& Point2, const FPlane& Plane);
	static FVector RayPlaneIntersection(const FVector& RayOrigin, const FVector& RayDirection, const FPlane& Plane);
	static bool LineSphereIntersection(const FVector& Start, const FVector& Dir, float Length, const FVector& Origin, float Radius);
	static bool LineBoxIntersection(const FBox& Box, const FVector& Start, const FVector& End, const FVector& Direction);
	static bool SphereAABBIntersection(const FVector& SphereCenter, const float RadiusSquared, const FBox& AABB);
	static bool PointBoxIntersection(const FVector& Point, const FBox& Box);
	static FVector GetReflectionVector(const FVector& Direction, const FVector& SurfaceNormal);
	static bool GetDotDistance(FVector2D& OutDotDist, const FVector& Direction, const FVector& AxisX, const FVector& AxisY, const FVector& AxisZ);
	static FVector2D GetAzimuthAndElevation(const FVector& Direction, const FVector& AxisX, const FVector& AxisY, const FVector& AxisZ);
};
struct FGenericPlatformMath : public FMath {};

// ---------- FVector2D ----------
struct FVector2D
{
	float X; float Y;
	static const FVector2D ZeroVector; static const FVector2D UnitVector; static const FVector2D Unit45Deg;
	FVector2D(); FVector2D(float InX, float InY); explicit FVector2D(float InF); FVector2D(FIntPoint InPos); explicit FVector2D(EForceInit); explicit FVector2D(const FVector& V); explicit FVector2D(const FVector4& V);
	FVector2D operator+(const FVector2D& V) const; FVector2D operator-(const FVector2D& V) const; FVector2D operator*(float Scale) const; FVector2D operator/(float Scale) const;
	FVector2D operator+(float A) const; FVector2D operator-(float A) const;
	FVector2D operator*(const FVector2D& V) const; FVector2D operator/(const FVector2D& V) const;
	float operator|(const FVector2D& V) const; float operator^(const FVector2D& V) const;
	bool operator==(const FVector2D& V) const; bool operator!=(const FVector2D& V) const; bool operator<(const FVector2D& Other) const; bool operator>(const FVector2D& Other) const;
	FVector2D operator-() const;
	FVector2D operator+=(const FVector2D& V); FVector2D operator-=(const FVector2D& V); FVector2D operator*=(float Scale); FVector2D operator/=(float V); FVector2D operator*=(const FVector2D& V); FVector2D operator/=(const FVector2D& V);
	float& operator[](int32 Index); float operator[](int32 Index) const;
	static float DotProduct(const FVector2D& A, const FVector2D& B); static float DistSquared(const FVector2D& V1, const FVector2D& V2); static float Distance(const FVector2D& V1, const FVector2D& V2); static float CrossProduct(const FVector2D& A, const FVector2D& B);
	static FVector2D Max(const FVector2D& A, const FVector2D& B); static FVector2D Min(const FVector2D& A, const FVector2D& B);
	bool Equals(const FVector2D& V, float Tolerance = KINDA_SMALL_NUMBER) const;
	void Set(float InX, float InY);
	float GetMax() const; float GetAbsMax() const; float GetMin() const;
	float Size() const; float SizeSquared() const;
	FVector2D GetRotated(float AngleDeg) const;
	FVector2D GetSafeNormal(float Tolerance = SMALL_NUMBER) const; void Normalize(float Tolerance = SMALL_NUMBER);
	bool IsNearlyZero(float Tolerance = KINDA_SMALL_NUMBER) const; bool IsZero() const;
	FIntPoint IntPoint() const; FVector2D RoundToVector() const;
	FVector2D ClampAxes(float MinAxisVal, float MaxAxisVal) const;
	FVector2D GetSignVector() const; FVector2D GetAbs() const;
	FString ToString() const;
	FVector SphericalToUnitCartesian() const;
};
inline FVector2D operator*(float Scale, const FVector2D& V) { return V * Scale; }

// ---------- FVector ----------
struct FVector
{
	float X; float Y; float Z;
	static const FVector ZeroVector; static const FVector OneVector; static const FVector UpVector; static const FVector DownVector;
	static const FVector ForwardVector; static const FVector BackwardVector; static const FVector RightVector; static const FVector LeftVector;
	static const FVector XAxisVector; static const FVector YAxisVector; static const FVector ZAxisVector;
	FVector(); explicit FVector(float InF); FVector(float InX, float InY, float InZ);
	explicit FVector(const FVector2D V, float InZ); FVector(const FVector4& V); explicit FVector(const FLinearColor& InColor); explicit FVector(FIntVector InVector); explicit FVector(FIntPoint A); explicit FVector(EForceInit);
	FVector operator^(const FVector& V) const; static FVector CrossProduct(const FVector& A, const FVector& B);
	float operator|(const FVector& V) const; static float DotProduct(const FVector& A, const FVector& B);
	FVector operator+(const FVector& V) const; FVector operator-(const FVector& V) const;
	FVector operator-(float Bias) const; FVector operator+(float Bias) const;
	FVector operator*(float Scale) const; FVector operator/(float Scale) const;
	FVector operator*(const FVector& V) const; FVector operator/(const FVector& V) const;
	bool operator==(const FVector& V) const; bool operator!=(const FVector& V) const;
	bool Equals(const FVector& V, float Tolerance = KINDA_SMALL_NUMBER) const; bool AllComponentsEqual(float Tolerance = KINDA_SMALL_NUMBER) const;
	FVector operator-() const;
	FVector operator+=(const FVector& V); FVector operator-=(const FVector& V); FVector operator*=(float Scale); FVector operator/=(float V); FVector operator*=(const FVector& V); FVector operator/=(const FVector& V);
	float& operator[](int32 Index); float operator[](int32 Index) const;
	float& Component(int32 Index); float Component(int32 Index) const;
	float GetComponentForAxis(EAxis::Type Axis) const; void SetComponentForAxis(EAxis::Type Axis, float Component);
	void Set(float InX, float InY, float InZ);
	float GetMax() const; float GetAbsMax() const; float GetMin() const; float GetAbsMin() const;
	FVector ComponentMin(const FVector& Other) const; FVector ComponentMax(const FVector& Other) const;
	FVector GetAbs() const;
	float Size() const; float SizeSquared() const; float Size2D() const; float SizeSquared2D() const;
	bool IsNearlyZero(float Tolerance = KINDA_SMALL_NUMBER) const; bool IsZero() const;
	bool IsUnit(float LengthSquaredTolerance = KINDA_SMALL_NUMBER) const; bool IsNormalized() const;
	bool Normalize(float Tolerance = SMALL_NUMBER);
	FVector GetUnsafeNormal() const; FVector GetUnsafeNormal2D() const;
	FVector GetSafeNormal(float Tolerance = SMALL_NUMBER) const; FVector GetSafeNormal2D(float Tolerance = SMALL_NUMBER) const;
	void ToDirectionAndLength(FVector& OutDir, float& OutLength) const;
	FVector GetSignVector() const; FVector Projection() const;
	FVector GridSnap(const float& GridSz) const; FVector BoundToCube(float Radius) const; FVector BoundToBox(const FVector& Min, const FVector Max) const;
	FVector GetClampedToSize(float Min, float Max) const; FVector GetClampedToSize2D(float Min, float Max) const;
	FVector GetClampedToMaxSize(float MaxSize) const; FVector GetClampedToMaxSize2D(float MaxSize) const;
	void AddBounded(const FVector& V, float Radius = MAX_int16);
	FVector Reciprocal() const;
	bool IsUniform(float Tolerance = KINDA_SMALL_NUMBER) const;
	FVector MirrorByVector(const FVector& MirrorNormal) const; FVector MirrorByPlane(const FPlane& Plane) const;
	FVector RotateAngleAxis(const float AngleDeg, const FVector& Axis) const;
	float CosineAngle2D(FVector B) const;
	FVector ProjectOnTo(const FVector& A) const; FVector ProjectOnToNormal(const FVector& Normal) const;
	FRotator ToOrientationRotator() const; FQuat ToOrientationQuat() const; FRotator Rotation() const;
	void FindBestAxisVectors(FVector& Axis1, FVector& Axis2) const;
	void UnwindEuler();
	bool ContainsNaN() const;
	FString ToString() const; FText ToText() const; FString ToCompactString() const; bool InitFromString(const FString& InSourceString);
	FVector2D UnitCartesianToSpherical() const;
	float HeadingAngle() const;
	static void CreateOrthonormalBasis(FVector& XAxis, FVector& YAxis, FVector& ZAxis);
	static bool PointsAreSame(const FVector& P, const FVector& Q); static bool PointsAreNear(const FVector& Point1, const FVector& Point2, float Dist);
	static float PointPlaneDist(const FVector& Point, const FVector& PlaneBase, const FVector& PlaneNormal);
	static FVector PointPlaneProject(const FVector& Point, const FPlane& Plane);
	static FVector PointPlaneProject(const FVector& Point, const FVector& A, const FVector& B, const FVector& C);
	static FVector PointPlaneProject(const FVector& Point, const FVector& PlaneBase, const FVector& PlaneNormal);
	static FVector VectorPlaneProject(const FVector& V, const FVector& PlaneNormal);
	static float Dist(const FVector& V1, const FVector& V2); static float Distance(const FVector& V1, const FVector& V2);
	static float DistXY(const FVector& V1, const FVector& V2); static float Dist2D(const FVector& V1, const FVector& V2);
	static float DistSquared(const FVector& V1, const FVector& V2); static float DistSquaredXY(const FVector& V1, const FVector& V2); static float DistSquared2D(const FVector& V1, const FVector& V2);
	static float BoxPushOut(const FVector& Normal, const FVector& Size);
	static bool Parallel(const FVector& Normal1, const FVector& Normal2, float ParallelCosineThreshold = THRESH_NORMALS_ARE_PARALLEL);
	static bool Coincident(const FVector& Normal1, const FVector& Normal2, float ParallelCosineThreshold = THRESH_NORMALS_ARE_PARALLEL);
	static bool Orthogonal(const FVector& Normal1, const FVector& Normal2, float OrthogonalCosineThreshold = THRESH_NORMALS_ARE_ORTHOGONAL);
	static bool Coplanar(const FVector& Base1, const FVector& Normal1, const FVector& Base2, const FVector& Normal2, float ParallelCosineThreshold = THRESH_NORMALS_ARE_PARALLEL);
	static float Triple(const FVector& X, const FVector& Y, const FVector& Z);
	static FVector RadiansToDegrees(const FVector& RadVector); static FVector DegreesToRadians(const FVector& DegVector);
};
template<typename T> inline typename std::enable_if<std::is_arithmetic<T>::value, FVector>::type operator*(T Scale, const FVector& V) { return V.operator*((float)Scale); }
uint32 GetTypeHash(const FVector& V);
uint32 GetTypeHash(const FVector2D& V);
struct FVector_NetQuantize : public FVector { FVector_NetQuantize(); explicit FVector_NetQuantize(EForceInit E); FVector_NetQuantize(float x, float y, float z); FVector_NetQuantize(const FVector& V); };
struct FVector_NetQuantize10 : public FVector { FVector_NetQuantize10(); FVector_NetQuantize10(const FVector& V); };
struct FVector_NetQuantize100 : public FVector { FVector_NetQuantize100(); FVector_NetQuantize100(const FVector& V); };
struct FVector_NetQuantizeNormal : public FVector { FVector_NetQuantizeNormal(); FVector_NetQuantizeNormal(const FVector& V); };

struct FVector4
{
	float X, Y, Z, W;
	FVector4(const FVector& InVector, float InW = 1.0f); FVector4(const FLinearColor& InColor); explicit FVector4(float InX = 0.0f, float InY = 0.0f, float InZ = 0.0f, float InW = 1.0f);
	FVector4(FVector2D InXY, FVector2D InZW); explicit FVector4(EForceInit);
	float& operator[](int32 ComponentIndex); float operator[](int32 ComponentIndex) const;
	FVector4 operator-() const; FVector4 operator+(const FVector4& V) const; FVector4 operator-(const FVector4& V) const; FVector4 operator*(float Scale) const; FVector4 operator/(float Scale) const; FVector4 operator*(const FVector4& V) const;
	FVector4 operator+=(const FVector4& V); FVector4 operator*=(float S);
	bool operator==(const FVector4& V) const; bool operator!=(const FVector4& V) const;
	FVector4 GetSafeNormal(float Tolerance = SMALL_NUMBER) const; float Size3() const; float SizeSquared3() const; float Size() const;
	void Set(float InX, float InY, float InZ, float InW);
	FRotator Rotation() const;
};

struct FIntPoint
{
	int32 X, Y;
	static const FIntPoint ZeroValue; static const FIntPoint NoneValue;
	FIntPoint(); FIntPoint(int32 InX, int32 InY); explicit FIntPoint(int32 InXY); explicit FIntPoint(EForceInit);
	FIntPoint operator+(const FIntPoint& O) const; FIntPoint operator-(const FIntPoint& O) const; FIntPoint operator*(int32 S) const; FIntPoint operator/(int32 D) const;
	FIntPoint& operator+=(const FIntPoint& O); FIntPoint& operator-=(const FIntPoint& O);
	bool operator==(const FIntPoint& O) const; bool operator!=(const FIntPoint& O) const;
	int32& operator[](int32 I); int32 operator[](int32 I) const;
	int32 Size() const; int32 SizeSquared() const; int32 GetMax() const; int32 GetMin() const;
	FString ToString() const;
};
uint32 GetTypeHash(const FIntPoint& P);
struct FIntVector
{
	int32 X, Y, Z;
	static const FIntVector ZeroValue; static const FIntVector NoneValue;
	FIntVector(); FIntVector(int32 InX, int32 InY, int32 InZ); explicit FIntVector(int32 InValue); explicit FIntVector(FVector InVector); explicit FIntVector(EForceInit);
	FIntVector operator+(const FIntVector& O) const; FIntVector operator-(const FIntVector& O) const; FIntVector operator*(int32 S) const; FIntVector operator/(int32 D) const;
	FIntVector& operator+=(const FIntVector& O);
	bool operator==(const FIntVector& O) const; bool operator!=(const FIntVector& O) const;
	int32& operator[](int32 I); const int32& operator()(int32 I) const; int32 operator[](int32 I) const;
	int32 GetMax() const; int32 GetMin() const; int32 Size() const; bool IsZero() const;
	FString ToString() const;
};
uint32 GetTypeHash(const FIntVector& P);
struct FIntRect { FIntPoint Min, Max; FIntRect(); FIntRect(int32 X0, int32 Y0, int32 X1, int32 Y1); FIntRect(FIntPoint InMin, FIntPoint InMax); int32 Width() const; int32 Height() const; FIntPoint Size() const; bool Contains(FIntPoint P) const; int32 Area() const; };

// ---------- FRotator ----------
struct FRotator
{
	float Pitch; float Yaw; float Roll;
	static const FRotator ZeroRotator;
	FRotator(); explicit FRotator(float InF); FRotator(float InPitch, float InYaw, float InRoll); explicit FRotator(EForceInit); explicit FRotator(const FQuat& Quat);
	FRotator operator+(const FRotator& R) const; FRotator operator-(const FRotator& R) const; FRotator operator*(float Scale) const; FRotator operator*=(float Scale);
	bool operator==(const FRotator& R) const; bool operator!=(const FRotator& V) const;
	FRotator operator+=(const FRotator& R); FRotator operator-=(const FRotator& R);
	bool IsNearlyZero(float Tolerance = KINDA_SMALL_NUMBER) const; bool IsZero() const;
	bool Equals(const FRotator& R, float Tolerance = KINDA_SMALL_NUMBER) const;
	FRotator Add(float DeltaPitch, float DeltaYaw, float DeltaRoll);
	FRotator GetInverse() const; FRotator GridSnap(const FRotator& RotGrid) const;
	FVector Vector() const; FQuat Quaternion() const; FVector Euler() const;
	FVector RotateVector(const FVector& V) const; FVector UnrotateVector(const FVector& V) const;
	FRotator Clamp() const; FRotator GetNormalized() const; FRotator GetDenormalized() const;
	float GetComponentForAxis(EAxis::Type Axis) const; void SetComponentForAxis(EAxis::Type Axis, float Component);
	void Normalize();
	void GetWindingAndRemainder(FRotator& Winding, FRotator& Remainder) const;
	float GetManhattanDistance(const FRotator& Rotator) const;
	FRotator GetEquivalentRotator() const;
	void SetClosestToMe(FRotator& MakeClosest) const;
	FString ToString() const; FString ToCompactString() const;
	bool ContainsNaN() const;
	static float ClampAxis(float Angle); static float NormalizeAxis(float Angle);
	static uint8 CompressAxisToByte(float Angle); static float DecompressAxisFromByte(uint8 Angle);
	static FRotator MakeFromEuler(const FVector& Euler);
};
inline FRotator operator*(float Scale, const FRotator& R) { return R * Scale; }

// ---------- FQuat ----------
struct FQuat
{
	float X; float Y; float Z; float W;
	static const FQuat Identity;
	FQuat(); explicit FQuat(EForceInit); FQuat(float InX, float InY, float InZ, float InW);
	explicit FQuat(const FMatrix& M); explicit FQuat(const FRotator& R); FQuat(FVector Axis, float AngleRad);
	FQuat operator+(const FQuat& Q) const; FQuat operator+=(const FQuat& Q); FQuat operator-(const FQuat& Q) const; FQuat operator-=(const FQuat& Q);
	FQuat operator*(const FQuat& Q) const; FQuat operator*=(const FQuat& Q);
	FVector operator*(const FVector& V) const; FMatrix operator*(const FMatrix& M) const;
	FQuat operator*=(const float Scale); FQuat operator*(const float Scale) const; FQuat operator/=(const float Scale); FQuat operator/(const float Scale) const;
	bool operator==(const FQuat& Q) const; bool operator!=(const FQuat& Q) const;
	float operator|(const FQuat& Q) const;
	bool Equals(const FQuat& Q, float Tolerance = KINDA_SMALL_NUMBER) const; bool IsIdentity(float Tolerance = SMALL_NUMBER) const;
	static FQuat MakeFromEuler(const FVector& Euler);
	FVector Euler() const;
	void Normalize(float Tolerance = SMALL_NUMBER); FQuat GetNormalized(float Tolerance = SMALL_NUMBER) const; bool IsNormalized() const;
	float Size() const; float SizeSquared() const;
	float GetAngle() const;
	void ToAxisAndAngle(FVector& Axis, float& Angle) const;
	void ToSwingTwist(const FVector& InTwistAxis, FQuat& OutSwing, FQuat& OutTwist) const;
	float GetTwistAngle(const FVector& TwistAxis) const;
	FVector RotateVector(FVector V) const; FVector UnrotateVector(FVector V) const;
	FQuat Log() const; FQuat Exp() const; FQuat Inverse() const;
	void EnforceShortestArcWith(const FQuat& OtherQuat);
	FVector GetAxisX() const; FVector GetAxisY() const; FVector GetAxisZ() const;
	FVector GetForwardVector() const; FVector GetRightVector() const; FVector GetUpVector() const;
	FVector Vector() const; FRotator Rotator() const; FVector GetRotationAxis() const;
	float AngularDistance(const FQuat& Q) const;
	bool ContainsNaN() const;
	FString ToString() const;
	static FQuat FindBetween(const FVector& Vector1, const FVector& Vector2);
	static FQuat FindBetweenNormals(const FVector& Normal1, const FVector& Normal2);
	static FQuat FindBetweenVectors(const FVector& Vector1, const FVector& Vector2);
	static float Error(const FQuat& Q1, const FQuat& Q2); static float ErrorAutoNormalize(const FQuat& A, const FQuat& B);
	static FQuat FastLerp(const FQuat& A, const FQuat& B, const float Alpha);
	static FQuat FastBilerp(const FQuat& P00, const FQuat& P10, const FQuat& P01, const FQuat& P11, float FracX, float FracY);
	static FQuat Slerp_NotNormalized(const FQuat& Quat1, const FQuat& Quat2, float Slerp);
	static FQuat Slerp(const FQuat& Quat1, const FQuat& Quat2, float Slerp);
	static FQuat SlerpFullPath_NotNormalized(const FQuat& quat1, const FQuat& quat2, float Alpha);
	static FQuat SlerpFullPath(const FQuat& quat1, const FQuat& quat2, float Alpha);
	static FQuat Squad(const FQuat& quat1, const FQuat& tang1, const FQuat& quat2, const FQuat& tang2, float Alpha);
};

// ---------- FPlane / FMatrix ----------
struct FPlane : public FVector
{
	float W;
	FPlane(); FPlane(const FVector4& V); FPlane(float InX, float InY, float InZ, float InW); FPlane(FVector InNormal, float InW); FPlane(FVector InBase, const FVector& InNormal); FPlane(FVector A, FVector B, FVector C); explicit FPlane(EForceInit);
	float PlaneDot(const FVector& P) const; bool Normalize(float Tolerance = SMALL_NUMBER); FPlane Flip() const; FPlane TransformBy(const FMatrix& M) const;
	FVector GetOrigin() const; const FVector& GetNormal() const;
};
struct FMatrix
{
	float M[4][4];
	static const FMatrix Identity;
	FMatrix(); explicit FMatrix(EForceInit);
	FMatrix(const FPlane& InX, const FPlane& InY, const FPlane& InZ, const FPlane& InW);
	FMatrix(const FVector& InX, const FVector& InY, const FVector& InZ, const FVector& InW);
	void SetIdentity();
	FMatrix operator*(const FMatrix& Other) const; void operator*=(const FMatrix& Other);
	FMatrix operator+(const FMatrix& Other) const; FMatrix operator*(float Other) const;
	bool operator==(const FMatrix& Other) const; bool operator!=(const FMatrix& Other) const;
	bool Equals(const FMatrix& Other, float Tolerance = KINDA_SMALL_NUMBER) const;
	FVector4 TransformFVector4(const FVector4& V) const;
	FVector4 TransformPosition(const FVector& V) const; FVector InverseTransformPosition(const FVector& V) const;
	FVector4 TransformVector(const FVector& V) const; FVector InverseTransformVector(const FVector& V) const;
	FMatrix GetTransposed() const; float Determinant() const; float RotDeterminant() const;
	FMatrix InverseFast() const; FMatrix Inverse() const; FMatrix TransposeAdjoint() const;
	void RemoveScaling(float Tolerance = SMALL_NUMBER); FMatrix GetMatrixWithoutScale(float Tolerance = SMALL_NUMBER) const;
	FVector ExtractScaling(float Tolerance = SMALL_NUMBER); FVector GetScaleVector(float Tolerance = SMALL_NUMBER) const;
	FMatrix RemoveTranslation() const; FMatrix ConcatTranslation(const FVector& Translation) const;
	bool ContainsNaN() const;
	void ScaleTranslation(const FVector& Scale3D);
	float GetMaximumAxisScale() const;
	FMatrix ApplyScale(float Scale) const;
	FVector GetOrigin() const;
	FVector GetScaledAxis(EAxis::Type Axis) const; void GetScaledAxes(FVector& X, FVector& Y, FVector& Z) const;
	FVector GetUnitAxis(EAxis::Type Axis) const; void GetUnitAxes(FVector& X, FVector& Y, FVector& Z) const;
	void SetAxis(int32 i, const FVector& Axis); void SetOrigin(const FVector& NewOrigin); void SetAxes(FVector* Axis0 = nullptr, FVector* Axis1 = nullptr, FVector* Axis2 = nullptr, FVector* Origin = nullptr);
	FVector GetColumn(int32 i) const;
	FRotator Rotator() const; FQuat ToQuat() const;
	FString ToString() const;
};
struct FRotationTranslationMatrix : public FMatrix { FRotationTranslationMatrix(const FRotator& Rot, const FVector& Origin); static FMatrix Make(const FRotator& Rot, const FVector& Origin); };
struct FRotationMatrix : public FRotationTranslationMatrix
{
	FRotationMatrix(const FRotator& Rot);
	static FMatrix Make(FRotator const& Rot); static FMatrix Make(FQuat const& Rot);
	static FMatrix MakeFromX(FVector const& XAxis); static FMatrix MakeFromY(FVector const& YAxis); static FMatrix MakeFromZ(FVector const& ZAxis);
	static FMatrix MakeFromXY(FVector const& XAxis, FVector const& YAxis); static FMatrix MakeFromXZ(FVector const& XAxis, FVector const& ZAxis);
	static FMatrix MakeFromYX(FVector const& YAxis, FVector const& XAxis); static FMatrix MakeFromYZ(FVector const& YAxis, FVector const& ZAxis);
	static FMatrix MakeFromZX(FVector const& ZAxis, FVector const& XAxis); static FMatrix MakeFromZY(FVector const& ZAxis, FVector const& YAxis);
};
struct FQuatRotationMatrix : public FMatrix { FQuatRotationMatrix(const FQuat& Q); static FMatrix Make(const FQuat& Q); };
struct FQuatRotationTranslationMatrix : public FMatrix { FQuatRotationTranslationMatrix(const FQuat& Q, const FVector& Origin); static FMatrix Make(const FQuat& Q, const FVector& Origin); };
struct FTranslationMatrix : public FMatrix { FTranslationMatrix(const FVector& Delta); static FMatrix Make(FVector const& Delta); };
struct FScaleMatrix : public FMatrix { FScaleMatrix(float Scale); FScaleMatrix(const FVector& Scale); static FMatrix Make(float Scale); static FMatrix Make(const FVector& Scale); };
struct FScaleRotationTranslationMatrix : public FMatrix { FScaleRotationTranslationMatrix(const FVector& Scale, const FRotator& Rot, const FVector& Origin); };
struct FLookAtMatrix : public FMatrix { FLookAtMatrix(const FVector& EyePosition, const FVector& LookAtPosition, const FVector& UpVector); };
struct FInverseRotationMatrix : public FMatrix { FInverseRotationMatrix(const FRotator& Rot); };

// ---------- FTransform ----------
struct FTransform
{
	static const FTransform Identity;
	FTransform(); explicit FTransform(const FVector& InTranslation); explicit FTransform(const FQuat& InRotation); explicit FTransform(const FRotator& InRotation);
	FTransform(const FQuat& InRotation, const FVector& InTranslation, const FVector& InScale3D = FVector::OneVector);
	FTransform(const FRotator& InRotation, const FVector& InTranslation, const FVector& InScale3D = FVector::OneVector);
	explicit FTransform(const FMatrix& InMatrix);
	FTransform(const FVector& InX, const FVector& InY, const FVector& InZ, const FVector& InTranslation);
	explicit FTransform(ENoInit); explicit FTransform(EForceInit);
	FTransform operator*(const FTransform& Other) const; void operator*=(const FTransform& Other);
	FTransform operator*(const FQuat& Other) const; FTransform operator+(const FTransform& Atom) const; FTransform& operator+=(const FTransform& Atom);
	FTransform operator*(float Mult) const;
	bool Equals(const FTransform& Other, float Tolerance = KINDA_SMALL_NUMBER) const;
	bool EqualsNoScale(const FTransform& Other, float Tolerance = KINDA_SMALL_NUMBER) const;
	FMatrix ToMatrixWithScale() const; FMatrix ToMatrixNoScale() const; FMatrix ToInverseMatrixWithScale() const;
	FTransform Inverse() const;
	FTransform GetRelativeTransform(const FTransform& Other) const; FTransform GetRelativeTransformReverse(const FTransform& Other) const;
	void SetToRelativeTransform(const FTransform& ParentTransform);
	void Blend(const FTransform& Atom1, const FTransform& Atom2, float Alpha); void BlendWith(const FTransform& OtherAtom, float Alpha);
	FVector4 TransformFVector4(const FVector4& V) const; FVector4 TransformFVector4NoScale(const FVector4& V) const;
	FVector TransformPosition(const FVector& V) const; FVector TransformPositionNoScale(const FVector& V) const;
	FVector InverseTransformPosition(const FVector& V) const; FVector InverseTransformPositionNoScale(const FVector& V) const;
	FVector TransformVector(const FVector& V) const; FVector TransformVectorNoScale(const FVector& V) const;
	FVector InverseTransformVector(const FVector& V) const; FVector InverseTransformVectorNoScale(const FVector& V) const;
	FQuat TransformRotation(const FQuat& Q) const; FQuat InverseTransformRotation(const FQuat& Q) const;
	FTransform GetScaled(float Scale) const; FTransform GetScaled(FVector Scale) const;
	FVector GetScaledAxis(EAxis::Type InAxis) const; FVector GetUnitAxis(EAxis::Type InAxis) const;
	void Mirror(EAxis::Type MirrorAxis, EAxis::Type FlipAxis);
	FVector GetLocation() const; FRotator Rotator() const; float GetDeterminant() const;
	void SetLocation(const FVector& Origin);
	bool ContainsNaN() const; bool IsValid() const; bool IsRotationNormalized() const;
	void NormalizeRotation();
	void SetComponents(const FQuat& InRotation, const FVector& InTranslation, const FVector& InScale3D);
	void SetIdentity();
	void MultiplyScale3D(const FVector& Scale3DMultiplier);
	void SetTranslation(const FVector& NewTranslation); void CopyTranslation(const FTransform& Other);
	void ConcatenateRotation(const FQuat& DeltaRotation);
	void AddToTranslation(const FVector& DeltaTranslation);
	void SetRotation(const FQuat& NewRotation); void CopyRotation(const FTransform& Other);
	void SetScale3D(const FVector& NewScale3D); void CopyScale3D(const FTransform& Other);
	void SetTranslationAndScale3D(const FVector& NewTranslation, const FVector& NewScale3D);
	void Accumulate(const FTransform& SourceAtom);
	FQuat GetRotation() const; FVector GetTranslation() const; FVector GetScale3D() const;
	void RemoveScaling(float Tolerance = SMALL_NUMBER);
	float GetMaximumAxisScale() const; float GetMinimumAxisScale() const;
	FString ToString() const; FString ToHumanReadableString() const;
	static void Multiply(FTransform* OutTransform, const FTransform* A, const FTransform* B);
	static bool AreRotationsEqual(const FTransform& A, const FTransform& B, float Tolerance = KINDA_SMALL_NUMBER);
	static bool AreTranslationsEqual(const FTransform& A, const FTransform& B, float Tolerance = KINDA_SMALL_NUMBER);
	static FVector AddTranslations(const FTransform& A, const FTransform& B);
	static FVector SubtractTranslations(const FTransform& A, const FTransform& B);
};

// ---------- цвета ----------
struct FColor
{
	uint8 B, G, R, A;
	static const FColor White, Black, Transparent, Red, Green, Blue, Yellow, Cyan, Magenta, Orange, Purple, Turquoise, Silver, Emerald;
	FColor(); explicit FColor(EForceInit); FColor(uint8 InR, uint8 InG, uint8 InB, uint8 InA = 255); explicit FColor(uint32 InColor);
	uint32& DWColor(); const uint32& DWColor() const;
	bool operator==(const FColor& C) const; bool operator!=(const FColor& C) const; void operator+=(const FColor& C);
	FLinearColor FromRGBE() const; FLinearColor ReinterpretAsLinear() const;
	static FColor FromHex(const FString& HexString); static FColor MakeRandomColor(); static FColor MakeRedToGreenColorFromScalar(float Scalar); static FColor MakeFromColorTemperature(float Temp);
	FColor WithAlpha(uint8 Alpha) const;
	FString ToHex() const; FString ToString() const;
	uint32 ToPackedARGB() const; uint32 ToPackedABGR() const; uint32 ToPackedRGBA() const; uint32 ToPackedBGRA() const;
};
uint32 GetTypeHash(const FColor& C);
struct FLinearColor
{
	float R, G, B, A;
	static const FLinearColor White, Gray, Black, Transparent, Red, Green, Blue, Yellow;
	FLinearColor(); explicit FLinearColor(EForceInit); FLinearColor(float InR, float InG, float InB, float InA = 1.0f);
	FLinearColor(const FColor& Color); explicit FLinearColor(const FVector& Vector); explicit FLinearColor(const FVector4& Vector);
	FColor ToRGBE() const; FColor ToFColor(const bool bSRGB) const; FColor Quantize() const; FColor QuantizeRound() const;
	static FLinearColor FromSRGBColor(const FColor& Color); static FLinearColor FromPow22Color(const FColor& Color);
	FLinearColor LinearRGBToHSV() const; FLinearColor HSVToLinearRGB() const;
	static FLinearColor MakeFromHSV8(uint8 H, uint8 S, uint8 V); static FLinearColor MakeRandomColor(); static FLinearColor MakeFromColorTemperature(float Temp);
	static FLinearColor LerpUsingHSV(const FLinearColor& From, const FLinearColor& To, const float Progress);
	static float Dist(const FLinearColor& V1, const FLinearColor& V2);
	float& Component(int32 Index); const float& Component(int32 Index) const;
	FLinearColor operator+(const FLinearColor& ColorB) const; FLinearColor& operator+=(const FLinearColor& ColorB);
	FLinearColor operator-(const FLinearColor& ColorB) const; FLinearColor& operator-=(const FLinearColor& ColorB);
	FLinearColor operator*(const FLinearColor& ColorB) const; FLinearColor& operator*=(const FLinearColor& ColorB);
	FLinearColor operator*(float Scalar) const; FLinearColor& operator*=(float Scalar);
	FLinearColor operator/(const FLinearColor& ColorB) const; FLinearColor& operator/=(const FLinearColor& ColorB);
	FLinearColor operator/(float Scalar) const; FLinearColor& operator/=(float Scalar);
	bool operator==(const FLinearColor& ColorB) const; bool operator!=(const FLinearColor& Other) const;
	bool Equals(const FLinearColor& ColorB, float Tolerance = KINDA_SMALL_NUMBER) const;
	FLinearColor GetClamped(float InMin = 0.0f, float InMax = 1.0f) const;
	FLinearColor CopyWithNewOpacity(float NewOpacicty) const;
	FLinearColor Desaturate(float Desaturation) const;
	float ComputeLuminance() const; float GetMax() const; float GetMin() const; float GetLuminance() const;
	bool IsAlmostBlack() const;
	FString ToString() const;
};
inline FLinearColor operator*(float Scalar, const FLinearColor& Color) { return Color * Scalar; }

// ---------- коробки, сферы ----------
struct FBox
{
	FVector Min; FVector Max; uint8 IsValid;
	FBox(); explicit FBox(EForceInit); FBox(const FVector& InMin, const FVector& InMax); FBox(const FVector* Points, int32 Count); explicit FBox(const TArray<FVector>& Points);
	bool operator==(const FBox& Other) const; FBox& operator+=(const FVector& Other); FBox operator+(const FVector& Other) const; FBox& operator+=(const FBox& Other); FBox operator+(const FBox& Other) const;
	FVector& operator[](int32 Index);
	float ComputeSquaredDistanceToPoint(const FVector& Point) const;
	FBox ExpandBy(float W) const; FBox ExpandBy(const FVector& V) const; FBox ExpandBy(const FVector& Neg, const FVector& Pos) const; FBox ShiftBy(const FVector& Offset) const; FBox MoveTo(const FVector& Destination) const;
	FVector GetCenter() const; void GetCenterAndExtents(FVector& center, FVector& Extents) const; FVector GetClosestPointTo(const FVector& Point) const;
	FVector GetExtent() const; FVector GetSize() const; float GetVolume() const;
	void Init();
	bool Intersect(const FBox& other) const; bool IntersectXY(const FBox& Other) const; FBox Overlap(const FBox& Other) const;
	FBox InverseTransformBy(const FTransform& M) const; FBox TransformBy(const FMatrix& M) const; FBox TransformBy(const FTransform& M) const;
	bool IsInside(const FVector& In) const; bool IsInsideOrOn(const FVector& In) const; bool IsInside(const FBox& Other) const; bool IsInsideXY(const FVector& In) const;
	static FBox BuildAABB(const FVector& Origin, const FVector& Extent);
	FString ToString() const;
};
struct FBox2D
{
	FVector2D Min; FVector2D Max; bool bIsValid;
	FBox2D(); explicit FBox2D(EForceInit); FBox2D(const FVector2D& InMin, const FVector2D& InMax);
	FBox2D& operator+=(const FVector2D& Other); FBox2D operator+(const FVector2D& Other) const; FBox2D& operator+=(const FBox2D& Other);
	float GetArea() const; FVector2D GetCenter() const; FVector2D GetExtent() const; FVector2D GetSize() const;
	void Init(); bool Intersect(const FBox2D& other) const; bool IsInside(const FVector2D& TestPoint) const; bool IsInside(const FBox2D& Other) const;
	FBox2D ShiftBy(const FVector2D& Offset) const; FBox2D ExpandBy(const float W) const;
	FVector2D GetClosestPointTo(const FVector2D& Point) const; float ComputeSquaredDistanceToPoint(const FVector2D& Point) const;
};
struct FSphere { FVector Center; float W; FSphere(); FSphere(int32); FSphere(FVector InV, float InW); explicit FSphere(EForceInit); bool IsInside(const FSphere& Other, float Tolerance = KINDA_SMALL_NUMBER) const; bool IsInside(const FVector& In, float Tolerance = KINDA_SMALL_NUMBER) const; bool Intersects(const FSphere& Other, float Tolerance = KINDA_SMALL_NUMBER) const; FSphere TransformBy(const FTransform& M) const; };
struct FBoxSphereBounds
{
	FVector Origin; FVector BoxExtent; float SphereRadius;
	FBoxSphereBounds(); explicit FBoxSphereBounds(EForceInit); FBoxSphereBounds(const FVector& InOrigin, const FVector& InBoxExtent, float InSphereRadius);
	FBoxSphereBounds(const FBox& Box, const FSphere& Sphere); FBoxSphereBounds(const FBox& Box); FBoxSphereBounds(const FSphere& Sphere);
	FBoxSphereBounds operator+(const FBoxSphereBounds& Other) const;
	FBox GetBox() const; FVector GetBoxExtrema(uint32 Extrema) const; FSphere GetSphere() const;
	FBoxSphereBounds ExpandBy(float ExpandAmount) const; FBoxSphereBounds TransformBy(const FMatrix& M) const; FBoxSphereBounds TransformBy(const FTransform& M) const;
	float ComputeSquaredDistanceFromBoxToPoint(const FVector& Point) const;
	static bool SpheresIntersect(const FBoxSphereBounds& A, const FBoxSphereBounds& B, float Tolerance = KINDA_SMALL_NUMBER);
	static bool BoxesIntersect(const FBoxSphereBounds& A, const FBoxSphereBounds& B);
};
struct FCapsuleShape { FVector Center; float Radius; FVector Orientation; float Length; };
template<typename T> struct TRange { TRange(); TRange(const T& A); TRange(const T& A, const T& B); T GetLowerBoundValue() const; T GetUpperBoundValue() const; bool Contains(const T& V) const; bool IsEmpty() const; };
typedef TRange<float> FFloatRange; typedef TRange<int32> FInt32Range;
struct FFloatInterval { float Min; float Max; FFloatInterval(); FFloatInterval(float InMin, float InMax); float Size() const; bool IsValid() const; bool Contains(const float& E) const; float Clamp(float E) const; float Interpolate(float Alpha) const; };
struct FInt32Interval { int32 Min; int32 Max; FInt32Interval(); FInt32Interval(int32 InMin, int32 InMax); };

// ---------- генератор случайных чисел ----------
struct FRandomStream
{
	FRandomStream(); FRandomStream(int32 InSeed); explicit FRandomStream(FName InName);
	void Initialize(int32 InSeed); void Initialize(FName InName); void Reset() const;
	int32 GetInitialSeed() const; void GenerateNewSeed(); int32 GetCurrentSeed() const;
	float GetFraction() const; uint32 GetUnsignedInt() const; FVector GetUnitVector() const;
	float FRand() const; FVector VRand() const;
	int32 RandHelper(int32 A) const;
	int32 RandRange(int32 Min, int32 Max) const;
	float FRandRange(float InMin, float InMax) const;
	FVector VRandCone(FVector const& Dir, float ConeHalfAngleRad);
	FVector VRandCone(FVector const& Dir, float HorizontalConeHalfAngleRad, float VerticalConeHalfAngleRad);
	FString ToString() const;
};

// ---------- кривые ----------
template<class T> struct FInterpCurvePoint { float InVal; T OutVal; T ArriveTangent; T LeaveTangent; };
template<class T> class FInterpCurve { public: TArray<FInterpCurvePoint<T>> Points; int32 AddPoint(const float InVal, const T& OutVal); T Eval(const float InVal, const T& Default = T()) const; };
typedef FInterpCurve<float> FInterpCurveFloat; typedef FInterpCurve<FVector> FInterpCurveVector;

// ---------- сериализация математических типов (FArchive) ----------
FArchive& operator<<(FArchive& Ar, FVector& V); FArchive& operator<<(FArchive& Ar, FVector2D& V); FArchive& operator<<(FArchive& Ar, FVector4& V);
FArchive& operator<<(FArchive& Ar, FRotator& R); FArchive& operator<<(FArchive& Ar, FQuat& Q); FArchive& operator<<(FArchive& Ar, FTransform& T);
FArchive& operator<<(FArchive& Ar, FColor& C); FArchive& operator<<(FArchive& Ar, FLinearColor& C); FArchive& operator<<(FArchive& Ar, FIntPoint& P);
FArchive& operator<<(FArchive& Ar, FIntVector& P); FArchive& operator<<(FArchive& Ar, FBox& B); FArchive& operator<<(FArchive& Ar, FGuid& G); FArchive& operator<<(FArchive& Ar, FDateTime& D);
template<typename T, typename K, typename A> FArchive& operator<<(FArchive& Ar, TSet<T, K, A>& S);

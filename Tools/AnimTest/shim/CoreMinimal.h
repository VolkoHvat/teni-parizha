// Тени Парижа — минимальная РАБОЧАЯ замена CoreMinimal.h для тестов математики поз вне UE.
// Формулы повторяют UE 4.27 (UnrealMath.cpp, Quat.h, Vector.h): умножение кватернионов по Гамильтону,
// A * B — сначала B, затем A; RotateVector: V + 2W(Q×V) + Q×(2Q×V); FindBetweenNormals — как FindBetween_Helper.
#pragma once
#include <cmath>
#include <cstdint>
typedef int32_t int32; typedef uint8_t uint8; typedef char TCHAR;
#define TEXT(x) x
#define PI (3.1415926535897932f)
#define HALF_PI (1.57079632679f)
#define SMALL_NUMBER (1.e-8f)
#define KINDA_SMALL_NUMBER (1.e-4f)

struct FMath {
	template<class T> static T Clamp(const T X, const T Mn, const T Mx) { return X < Mn ? Mn : X < Mx ? X : Mx; }
	template<class T> static T Max(const T A, const T B) { return A >= B ? A : B; }
	template<class T> static T Min(const T A, const T B) { return A <= B ? A : B; }
	template<class T> static T Abs(const T A) { return A >= (T)0 ? A : -A; }
	template<class T, class U> static T Lerp(const T& A, const T& B, const U& Alpha) { return (T)(A + Alpha * (B - A)); }
	static float Sqrt(float V) { return std::sqrt(V); }
	static float Sin(float V) { return std::sin(V); }
	static float Cos(float V) { return std::cos(V); }
	static float Acos(float V) { return std::acos(V); }
	static float Atan2(float Y, float X) { return std::atan2(Y, X); }
	static float Pow(float A, float B) { return std::pow(A, B); }
	static float Exp(float V) { return std::exp(V); }
	static float FloorToFloat(float V) { return std::floor(V); }
	static bool IsNaN(float V) { return std::isnan(V); }
	static bool IsFinite(float V) { return std::isfinite(V); }
};

struct FVector {
	float X, Y, Z;
	static const FVector ZeroVector;
	FVector() {}
	FVector(float InX, float InY, float InZ) : X(InX), Y(InY), Z(InZ) {}
	FVector operator+(const FVector& V) const { return FVector(X + V.X, Y + V.Y, Z + V.Z); }
	FVector operator-(const FVector& V) const { return FVector(X - V.X, Y - V.Y, Z - V.Z); }
	FVector operator-() const { return FVector(-X, -Y, -Z); }
	FVector operator*(float S) const { return FVector(X * S, Y * S, Z * S); }
	FVector operator/(float S) const { const float R = 1.f / S; return FVector(X * R, Y * R, Z * R); }
	FVector& operator+=(const FVector& V) { X += V.X; Y += V.Y; Z += V.Z; return *this; }
	FVector& operator-=(const FVector& V) { X -= V.X; Y -= V.Y; Z -= V.Z; return *this; }
	static FVector CrossProduct(const FVector& A, const FVector& B) { return FVector(A.Y * B.Z - A.Z * B.Y, A.Z * B.X - A.X * B.Z, A.X * B.Y - A.Y * B.X); }
	static float DotProduct(const FVector& A, const FVector& B) { return A.X * B.X + A.Y * B.Y + A.Z * B.Z; }
	float Size() const { return std::sqrt(X * X + Y * Y + Z * Z); }
	float SizeSquared() const { return X * X + Y * Y + Z * Z; }
	FVector GetSafeNormal(float Tolerance = SMALL_NUMBER) const {
		const float S = SizeSquared(); if (S == 1.f) return *this; if (S < Tolerance) return FVector(0, 0, 0);
		const float R = 1.f / std::sqrt(S); return FVector(X * R, Y * R, Z * R); }
	bool IsNearlyZero(float Tol = KINDA_SMALL_NUMBER) const { return std::fabs(X) <= Tol && std::fabs(Y) <= Tol && std::fabs(Z) <= Tol; }
	bool ContainsNaN() const { return !(std::isfinite(X) && std::isfinite(Y) && std::isfinite(Z)); }
};
inline FVector operator*(float S, const FVector& V) { return V * S; }
const FVector FVector::ZeroVector(0, 0, 0);   // один .cpp на тест — повторного определения нет

struct FQuat {
	float X, Y, Z, W;
	static const FQuat Identity;
	FQuat() {}
	FQuat(float InX, float InY, float InZ, float InW) : X(InX), Y(InY), Z(InZ), W(InW) {}
	FQuat(FVector Axis, float AngleRad) { const float H = .5f * AngleRad, S = std::sin(H); X = S * Axis.X; Y = S * Axis.Y; Z = S * Axis.Z; W = std::cos(H); }
	FQuat operator*(const FQuat& Q) const {
		return FQuat(W * Q.X + X * Q.W + Y * Q.Z - Z * Q.Y,
		             W * Q.Y - X * Q.Z + Y * Q.W + Z * Q.X,
		             W * Q.Z + X * Q.Y - Y * Q.X + Z * Q.W,
		             W * Q.W - X * Q.X - Y * Q.Y - Z * Q.Z); }
	FVector RotateVector(FVector V) const {
		const FVector Q(X, Y, Z); const FVector T = FVector::CrossProduct(Q, V) * 2.f;
		return V + (T * W) + FVector::CrossProduct(Q, T); }
	FQuat Inverse() const { return FQuat(-X, -Y, -Z, W); }
	float SizeSquared() const { return X * X + Y * Y + Z * Z + W * W; }
	FQuat GetNormalized(float Tolerance = SMALL_NUMBER) const {
		const float S = SizeSquared(); if (S >= Tolerance) { const float R = 1.f / std::sqrt(S); return FQuat(X * R, Y * R, Z * R, W * R); } return FQuat(0, 0, 0, 1); }
	static FQuat FindBetweenNormals(const FVector& A, const FVector& B) {
		const float NormAB = 1.f; float Wq = NormAB + FVector::DotProduct(A, B); FQuat Res;
		if (Wq >= 1e-6f * NormAB) Res = FQuat(A.Y * B.Z - A.Z * B.Y, A.Z * B.X - A.X * B.Z, A.X * B.Y - A.Y * B.X, Wq);
		else { Wq = 0.f; Res = std::fabs(A.X) > std::fabs(A.Y) ? FQuat(-A.Z, 0.f, A.X, Wq) : FQuat(0.f, -A.Z, A.Y, Wq); }
		return Res.GetNormalized(); }
	static FQuat Slerp_NotNormalized(const FQuat& Q1, const FQuat& Q2, float Slerp) {
		const float Raw = Q1.X * Q2.X + Q1.Y * Q2.Y + Q1.Z * Q2.Z + Q1.W * Q2.W; const float Cosom = Raw >= 0 ? Raw : -Raw;
		float S0, S1;
		if (Cosom < 0.9999f) { const float Om = std::acos(Cosom), InvSin = 1.f / std::sin(Om); S0 = std::sin((1.f - Slerp) * Om) * InvSin; S1 = std::sin(Slerp * Om) * InvSin; }
		else { S0 = 1.f - Slerp; S1 = Slerp; }
		S1 = Raw >= 0 ? S1 : -S1;
		return FQuat(S0 * Q1.X + S1 * Q2.X, S0 * Q1.Y + S1 * Q2.Y, S0 * Q1.Z + S1 * Q2.Z, S0 * Q1.W + S1 * Q2.W); }
	static FQuat Slerp(const FQuat& Q1, const FQuat& Q2, float T) { return Slerp_NotNormalized(Q1, Q2, T).GetNormalized(); }
};
const FQuat FQuat::Identity(0, 0, 0, 1);

struct FTransform {
	FQuat Rotation; FVector Translation;
	FTransform() : Rotation(0, 0, 0, 1), Translation(0, 0, 0) {}
	FTransform(const FQuat& R, const FVector& T) : Rotation(R), Translation(T) {}
	FQuat GetRotation() const { return Rotation; } FVector GetTranslation() const { return Translation; }
};

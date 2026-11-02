// Тени Парижа — минимальная замена CoreMinimal.h для проверки построителя квартала (TPCity*, TPTexGen) обычным g++.
// Только то, что используют чистые (без UObject) файлы города: TArray, FColor, FMath, целые типы.
#pragma once
#include <cstdint>
#include <cmath>
#include <cstring>
#include <vector>
#include <cassert>
#include <algorithm>
#include <type_traits>
#include <utility>

typedef uint8_t uint8; typedef uint16_t uint16; typedef uint32_t uint32; typedef uint64_t uint64;
typedef int8_t int8; typedef int16_t int16; typedef int32_t int32; typedef int64_t int64;
#define MIN_int32 ((int32)0x80000000)
#define FORCEINLINE inline
#ifndef PI
#define PI (3.1415926535897932f)
#endif
#define check(x) assert(x)
template<class T> inline typename std::remove_reference<T>::type&& MoveTemp(T&& X) { return static_cast<typename std::remove_reference<T>::type&&>(X); }

template<class T> class TArray
{
public:
	std::vector<T> V;
	TArray() {}
	TArray(std::initializer_list<T> L) : V(L) {}
	int32 Num() const { return (int32)V.size(); }
	int32 Add(const T& X) { V.push_back(X); return (int32)V.size() - 1; }
	int32 Add(T&& X) { V.push_back(std::move(X)); return (int32)V.size() - 1; }
	int32 AddDefaulted(int32 N = 1) { const int32 I = Num(); V.resize(V.size() + N); return I; }
	int32 AddUninitialized(int32 N = 1) { return AddDefaulted(N); }
	void SetNum(int32 N) { V.resize(N); }
	void SetNumUninitialized(int32 N) { V.resize(N); }
	void SetNumZeroed(int32 N) { V.assign(N, T()); }
	void Init(const T& X, int32 N) { V.assign(N, X); }
	void Reset(int32 N = 0) { V.clear(); V.reserve(N); }
	void Empty(int32 N = 0) { V.clear(); V.reserve(N); }
	void Reserve(int32 N) { V.reserve(N); }
	bool IsValidIndex(int32 I) const { return I >= 0 && I < Num(); }
	T& operator[](int32 I) { assert(IsValidIndex(I)); return V[I]; }
	const T& operator[](int32 I) const { assert(IsValidIndex(I)); return V[I]; }
	T& Last(int32 K = 0) { return V[V.size() - 1 - K]; }
	const T& Last(int32 K = 0) const { return V[V.size() - 1 - K]; }
	T* GetData() { return V.data(); }
	const T* GetData() const { return V.data(); }
	void Append(const TArray& O) { V.insert(V.end(), O.V.begin(), O.V.end()); }
	void RemoveAt(int32 I, int32 Count = 1) { V.erase(V.begin() + I, V.begin() + I + Count); }
	void Insert(const T& X, int32 I) { V.insert(V.begin() + I, X); }
	typename std::vector<T>::iterator begin() { return V.begin(); }
	typename std::vector<T>::iterator end() { return V.end(); }
	typename std::vector<T>::const_iterator begin() const { return V.begin(); }
	typename std::vector<T>::const_iterator end() const { return V.end(); }
};

struct FColor
{
	uint8 B = 0, G = 0, R = 0, A = 0;   // раскладка как в UE (B8G8R8A8)
	FColor() {}
	FColor(uint8 InR, uint8 InG, uint8 InB, uint8 InA = 255) : B(InB), G(InG), R(InR), A(InA) {}
	bool operator==(const FColor& O) const { return R == O.R && G == O.G && B == O.B && A == O.A; }
};

struct FMath
{
	template<class T> static T Clamp(const T X, const T Mn, const T Mx) { return X < Mn ? Mn : X < Mx ? X : Mx; }
	template<class T> static T Max(const T A, const T B) { return A >= B ? A : B; }
	template<class T> static T Min(const T A, const T B) { return A <= B ? A : B; }
	template<class T> static T Abs(const T A) { return A >= (T)0 ? A : -A; }
	template<class T> static T Square(const T A) { return A * A; }
	template<class T, class U> static T Lerp(const T& A, const T& B, const U& Alpha) { return (T)(A + Alpha * (B - A)); }
	static float Sqrt(float X) { return std::sqrt(X); }
	static float Sin(float X) { return std::sin(X); }
	static float Cos(float X) { return std::cos(X); }
	static float Tan(float X) { return std::tan(X); }
	static float Atan(float X) { return std::atan(X); }
	static float Atan2(float Y, float X) { return std::atan2(Y, X); }
	static float Acos(float X) { return std::acos(X); }
	static float Asin(float X) { return std::asin(X); }
	static float Pow(float A, float B) { return std::pow(A, B); }
	static float Exp(float X) { return std::exp(X); }
	static float Fmod(float A, float B) { return std::fmod(A, B); }
	static float Frac(float X) { return X - std::floor(X); }
	static float Floor(float X) { return std::floor(X); }
	static float FloorToFloat(float X) { return std::floor(X); }
	static int32 FloorToInt(float X) { return (int32)std::floor(X); }
	static int32 CeilToInt(float X) { return (int32)std::ceil(X); }
	static int32 RoundToInt(float X) { return (int32)std::floor(X + 0.5f); }
	static int32 TruncToInt(float X) { return (int32)X; }
	static bool IsNaN(float X) { return std::isnan(X); }
	static bool IsFinite(float X) { return std::isfinite(X); }
};

// Тени Парижа — замена CoreMinimal.h для проверки построителя одежды (TPOutfitGen.h) обычным g++:
// математика поз из AnimTest (формулы UE 4.27) + TArray, FColor, FVector2D.
#pragma once
#include "../../AnimTest/shim/CoreMinimal.h"
#include <vector>
#include <cassert>
#include <utility>
typedef uint32_t uint32; typedef uint16_t uint16;

template<class T> class TArray
{
public:
	std::vector<T> V;
	int32 Num() const { return (int32)V.size(); }
	int32 Add(const T& X) { V.push_back(X); return (int32)V.size() - 1; }
	void SetNumUninitialized(int32 N) { V.resize(N); }
	void Reset(int32 N = 0) { V.clear(); V.reserve(N); }
	void Reserve(int32 N) { V.reserve(N); }
	bool IsValidIndex(int32 I) const { return I >= 0 && I < Num(); }
	T& operator[](int32 I) { assert(IsValidIndex(I)); return V[I]; }
	const T& operator[](int32 I) const { assert(IsValidIndex(I)); return V[I]; }
	T& Last() { return V.back(); }
	typename std::vector<T>::const_iterator begin() const { return V.begin(); }
	typename std::vector<T>::const_iterator end() const { return V.end(); }
};

struct FColor
{
	uint8 B = 0, G = 0, R = 0, A = 0;   // раскладка как в UE (B8G8R8A8)
	FColor() {}
	FColor(uint8 InR, uint8 InG, uint8 InB, uint8 InA = 255) : B(InB), G(InG), R(InR), A(InA) {}
};

struct FVector2D
{
	float X, Y;
	FVector2D() {}
	FVector2D(float InX, float InY) : X(InX), Y(InY) {}
};

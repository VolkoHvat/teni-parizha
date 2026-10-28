// Макет BoneIndices.h (UE 4.27): типизированные индексы костей.
#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
typedef uint16 FBoneIndexType;
struct FBoneIndexBase
{
	FBoneIndexBase() : BoneIndex(INDEX_NONE) {}
	int32 GetInt() const { return BoneIndex; }
	bool IsRootBone() const { return BoneIndex == 0; }
	bool IsValid() const { return BoneIndex != INDEX_NONE; }
protected:
	int32 BoneIndex;
};
#define TP_BONE_INDEX_TYPE(T) \
struct T : public FBoneIndexBase \
{ \
	explicit T(int32 InBoneIndex) { BoneIndex = InBoneIndex; } \
	T& operator++() { ++BoneIndex; return *this; } \
	T& operator--() { --BoneIndex; return *this; } \
	bool operator==(const T& Rhs) const { return BoneIndex == Rhs.BoneIndex; } \
	bool operator!=(const T& Rhs) const { return BoneIndex != Rhs.BoneIndex; } \
	bool operator<(const T& Rhs) const { return BoneIndex < Rhs.BoneIndex; } \
	bool operator>(const T& Rhs) const { return BoneIndex > Rhs.BoneIndex; } \
};
TP_BONE_INDEX_TYPE(FCompactPoseBoneIndex)
TP_BONE_INDEX_TYPE(FMeshPoseBoneIndex)
TP_BONE_INDEX_TYPE(FSkeletonPoseBoneIndex)
#undef TP_BONE_INDEX_TYPE

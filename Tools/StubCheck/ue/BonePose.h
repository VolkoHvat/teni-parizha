// Макет BonePose.h (UE 4.27): FCompactPose — локальные преобразования нужных костей.
// В 4.27 это FBaseCompactPose<FAnimStackAllocator>; для проверки синтаксиса достаточно простой структуры.
#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "BoneIndices.h"
#include "BoneContainer.h"
struct FTPCompactPoseRange
{
	struct FIt { int32 I; FCompactPoseBoneIndex operator*() const { return FCompactPoseBoneIndex(I); } FIt& operator++() { ++I; return *this; } bool operator!=(const FIt& R) const { return I != R.I; } };
	int32 Num; FIt begin() const { return FIt{ 0 }; } FIt end() const { return FIt{ Num }; }
};
struct FCompactPose
{
	FCompactPose();
	FTransform& operator[](FCompactPoseBoneIndex BoneIndex);
	const FTransform& operator[](FCompactPoseBoneIndex BoneIndex) const;
	int32 GetNumBones() const;
	bool IsValid() const;
	bool IsValidIndex(const FCompactPoseBoneIndex& BoneIndex) const;
	FTPCompactPoseRange ForEachBoneIndex() const;
	const FBoneContainer& GetBoneContainer() const;
	FBoneContainer& GetBoneContainer();
	void SetBoneContainer(const FBoneContainer* InBoneContainer);
	void InitBones(int32 NumBones);
	void ResetToRefPose();
	void ResetToAdditiveIdentity();
	void NormalizeRotations();
	bool IsNormalized() const;
	bool ContainsNaN() const;
	void CopyBonesFrom(const FCompactPose& SrcPose);
	FCompactPoseBoneIndex GetParentBoneIndex(const FCompactPoseBoneIndex& BoneIndex) const;
	const FTransform& GetRefPose(const FCompactPoseBoneIndex& BoneIndex) const;
	void Empty();
};

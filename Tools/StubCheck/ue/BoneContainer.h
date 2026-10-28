// Макет BoneContainer.h (UE 4.27): набор нужных костей текущего LOD.
#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "BoneIndices.h"
#include "ReferenceSkeleton.h"
class USkeleton; class UObject;
struct FBoneContainer
{
	FBoneContainer();
	bool IsValid() const;
	UObject* GetAsset() const;
	USkeleton* GetSkeletonAsset() const;
	const FReferenceSkeleton& GetReferenceSkeleton() const;
	const TArray<FBoneIndexType>& GetBoneIndicesArray() const;
	int32 GetNumBones() const;
	int32 GetCompactPoseNumBones() const;
	int32 GetPoseBoneIndexForBoneName(const FName& BoneName) const;
	int32 GetParentBoneIndex(const int32 BoneIndex) const;
	FCompactPoseBoneIndex GetParentBoneIndex(const FCompactPoseBoneIndex& BoneIndex) const;
	FCompactPoseBoneIndex GetCompactPoseIndexFromSkeletonIndex(const int32 SkeletonIndex) const;
	FMeshPoseBoneIndex MakeMeshPoseIndex(const FCompactPoseBoneIndex& BoneIndex) const;
	FCompactPoseBoneIndex MakeCompactPoseIndex(const FMeshPoseBoneIndex& BoneIndex) const;
	const FTransform& GetRefPoseTransform(const FCompactPoseBoneIndex& BoneIndex) const;
	bool Contains(FBoneIndexType NewIndex) const;
};

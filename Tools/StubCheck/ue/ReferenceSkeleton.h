#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
struct FMeshBoneInfo { FName Name; int32 ParentIndex; FMeshBoneInfo(); FMeshBoneInfo(const FName& InName, const FString& InExportName, int32 InParentIndex); };
struct FReferenceSkeleton
{
	int32 GetNum() const; int32 GetRawBoneNum() const;
	int32 FindBoneIndex(const FName& BoneName) const; int32 FindRawBoneIndex(const FName& BoneName) const;
	FName GetBoneName(const int32 BoneIndex) const;
	int32 GetParentIndex(const int32 BoneIndex) const; int32 GetRawParentIndex(const int32 BoneIndex) const;
	const TArray<FTransform>& GetRefBonePose() const; const TArray<FTransform>& GetRawRefBonePose() const;
	const TArray<FMeshBoneInfo>& GetRefBoneInfo() const; const TArray<FMeshBoneInfo>& GetRawRefBoneInfo() const;
	bool IsValidIndex(int32 Index) const; bool IsValidRawIndex(int32 Index) const;
	int32 GetDepthBetweenBones(const int32 BoneIndex, const int32 ParentBoneIndex) const;
	bool BoneIsChildOf(const int32 ChildBoneIndex, const int32 ParentBoneIndex) const;
};
class FReferenceSkeletonModifier { public: FReferenceSkeletonModifier(FReferenceSkeleton& InRefSkel, const class USkeleton* InSkeleton); void UpdateRefPoseTransform(const int32 BoneIndex, const FTransform& BonePose); void Add(const FMeshBoneInfo& BoneInfo, const FTransform& BonePose); };

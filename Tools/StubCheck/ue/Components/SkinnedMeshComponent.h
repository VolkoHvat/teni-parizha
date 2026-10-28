// Макет Components/SkinnedMeshComponent.h (UE 4.27): SkeletalMesh — открытый член; SetMasterPoseComponent (не SetLeaderPoseComponent).
#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "Components/MeshComponent.h"
class USkeletalMesh; class UPhysicsAsset; class USkinnedMeshComponent;
namespace EBoneSpaces { enum Type { WorldSpace, ComponentSpace }; }
enum EPhysBodyOp { PBO_None, PBO_Term, PBO_MAX };
struct FAnimUpdateRateParameters { bool bInterpolateSkippedFrames; bool bShouldUseLodMap; bool bShouldUseMinLod; TMap<int32, int32> LODToFrameSkipMap; int32 BaseNonRenderedUpdateRate; TArray<float> BaseVisibleDistanceFactorThesholds; int32 MaxEvalRateForInterpolation; };
DECLARE_DELEGATE_OneParam(FOnAnimUpdateRateParamsCreated, FAnimUpdateRateParameters*);
class USkinnedMeshComponent : public UMeshComponent
{
public:
	typedef UMeshComponent Super; typedef USkinnedMeshComponent ThisClass; static UClass* StaticClass();
	USkinnedMeshComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	USkeletalMesh* SkeletalMesh;
	TWeakObjectPtr<USkinnedMeshComponent> MasterPoseComponent;
	EVisibilityBasedAnimTickOption VisibilityBasedAnimTickOption;
	uint8 bUseBoundsFromMasterPoseComponent : 1; uint8 bRenderStatic : 1; uint8 bCastCapsuleDirectShadow : 1; uint8 bCastCapsuleIndirectShadow : 1;
	uint8 bEnableUpdateRateOptimizations : 1; uint8 bDisplayDebugUpdateRateOptimizations : 1; uint8 bPerBoneMotionBlur : 1; uint8 bComponentUseFixedSkelBounds : 1;
	uint8 bConsiderAllBodiesForBounds : 1; uint8 bSyncAttachParentLOD : 1; uint8 bNoSkeletonUpdate : 1; uint8 bIgnoreMasterPoseComponentLOD : 1;
	float CapsuleIndirectShadowMinVisibility;
	FAnimUpdateRateParameters* AnimUpdateRateParams;
	FOnAnimUpdateRateParamsCreated OnAnimUpdateRateParamsCreated;
	virtual void SetSkeletalMesh(USkeletalMesh* NewMesh, bool bReinitPose = true);
	void SetMasterPoseComponent(USkinnedMeshComponent* NewMasterBoneComponent, bool bForceUpdate = false);
	const TArray<TWeakObjectPtr<USkinnedMeshComponent>>& GetSlavePoseComponents() const;
	virtual UPhysicsAsset* GetPhysicsAsset() const;
	virtual void SetPhysicsAsset(UPhysicsAsset* NewPhysicsAsset, bool bForceReInit = false);
	int32 GetBoneIndex(FName BoneName) const;
	FName GetBoneName(int32 BoneIndex) const;
	int32 GetNumBones() const;
	FName GetParentBone(FName BoneName) const;
	void GetBoneNames(TArray<FName>& BoneNames);
	FName GetSocketBoneName(FName InSocketName) const;
	FVector GetBoneLocation(FName BoneName, EBoneSpaces::Type Space = EBoneSpaces::WorldSpace) const;
	FQuat GetBoneQuaternion(FName BoneName, EBoneSpaces::Type Space = EBoneSpaces::WorldSpace) const;
	FTransform GetBoneTransform(int32 BoneIndex) const;
	FTransform GetBoneTransform(int32 BoneIndex, const FTransform& LocalToWorld) const;
	const TArray<FTransform>& GetComponentSpaceTransforms() const;
	FName FindClosestBone(FVector TestLocation, FVector* BoneLocation = nullptr, float IgnoreScale = 0.f, bool bRequirePhysicsAsset = false) const;
	void SetForcedLOD(int32 InNewForcedLOD); int32 GetForcedLOD() const; int32 GetNumLODs() const; int32 GetPredictedLODLevel() const;
	void SetMinLOD(int32 InNewMinLOD);
	void SetCastCapsuleDirectShadow(bool bNewValue); void SetCastCapsuleIndirectShadow(bool bNewValue); void SetCapsuleIndirectShadowMinVisibility(float NewValue);
	void SetRenderStatic(bool bNewValue);
	virtual void HideBoneByName(FName BoneName, EPhysBodyOp PhysBodyOption);
	virtual void UnHideBoneByName(FName BoneName);
	bool IsBoneHiddenByName(FName BoneName);
	void SetVertexColorOverride_LinearColor(int32 LODIndex, const TArray<FLinearColor>& VertexColors);
	void ClearVertexColorOverride(int32 LODIndex);
	virtual bool DoesSocketExist(FName InSocketName) const override;
	virtual FTransform GetSocketTransform(FName InSocketName, ERelativeTransformSpace TransformSpace = RTS_World) const override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void RefreshBoneTransforms(FActorComponentTickFunction* TickFunction = nullptr);
	void TransformToBoneSpace(FName BoneName, FVector InPosition, FRotator InRotation, FVector& OutPosition, FRotator& OutRotation) const;
	void TransformFromBoneSpace(FName BoneName, FVector InPosition, FRotator InRotation, FVector& OutPosition, FRotator& OutRotation);
	FVector GetRefPosePosition(int32 BoneIndex);
};

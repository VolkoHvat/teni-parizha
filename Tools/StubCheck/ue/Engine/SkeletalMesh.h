// Макет Engine/SkeletalMesh.h (UE 4.27): Skeleton, RefSkeleton, Materials, PhysicsAsset — открытые члены.
#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ReferenceSkeleton.h"
#include "Engine/StaticMesh.h"
class USkeleton; class UPhysicsAsset; class UMaterialInterface; class USkeletalMeshSocket; class UMorphTarget;
struct FSkeletalMaterial { FSkeletalMaterial(); FSkeletalMaterial(class UMaterialInterface* InMaterialInterface, bool bInEnableShadowCasting = true, bool bInRecomputeTangent = false, FName InMaterialSlotName = NAME_None, FName InImportedMaterialSlotName = NAME_None); UMaterialInterface* MaterialInterface; FName MaterialSlotName; FMeshUVChannelInfo UVChannelData; };
struct FSkeletalMeshLODInfo { FPerPlatformFloat ScreenSize; float LODHysteresis; TArray<int32> LODMaterialMap; };
class USkeletalMeshSocket : public UObject { public: typedef UObject Super; static UClass* StaticClass(); FName SocketName; FName BoneName; FVector RelativeLocation; FRotator RelativeRotation; FVector RelativeScale; FTransform GetSocketTransform(const class USkinnedMeshComponent* SkelComp) const; FVector GetSocketLocation(const class USkinnedMeshComponent* SkelComp) const; };
class USkeletalMesh : public UObject
{
public:
	typedef UObject Super; typedef USkeletalMesh ThisClass; static UClass* StaticClass();
	USkeletalMesh(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	USkeleton* Skeleton;
	FReferenceSkeleton RefSkeleton;
	TArray<FSkeletalMaterial> Materials;
	UPhysicsAsset* PhysicsAsset; UPhysicsAsset* ShadowPhysicsAsset;
	TArray<UMorphTarget*> MorphTargets;
	FPerPlatformInt MinLod;
	uint8 bHasVertexColors : 1; uint8 bEnablePerPolyCollision : 1;
	USkeleton* GetSkeleton() const; void SetSkeleton(USkeleton* InSkeleton);
	const FReferenceSkeleton& GetRefSkeleton() const; FReferenceSkeleton& GetRefSkeleton();
	TArray<FSkeletalMaterial>& GetMaterials(); const TArray<FSkeletalMaterial>& GetMaterials() const; void SetMaterials(const TArray<FSkeletalMaterial>& InMaterials);
	UPhysicsAsset* GetPhysicsAsset() const; void SetPhysicsAsset(UPhysicsAsset* InPhysicsAsset);
	FBoxSphereBounds GetBounds() const; FBoxSphereBounds GetImportedBounds() const; void SetImportedBounds(const FBoxSphereBounds& InBounds);
	USkeletalMeshSocket* FindSocket(FName InSocketName) const; int32 NumSockets() const; USkeletalMeshSocket* GetSocketByIndex(int32 Index) const;
	int32 GetLODNum() const; FSkeletalMeshLODInfo* GetLODInfo(int32 Index);
	UMorphTarget* FindMorphTarget(FName MorphTargetName) const;
	void CalculateInvRefMatrices();
};

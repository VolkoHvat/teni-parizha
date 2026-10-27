// Макет Components/InstancedStaticMeshComponent.h (UE 4.27): AddInstance без bWorldSpace (он появился в UE5).
#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
struct FInstancedStaticMeshInstanceData { FMatrix Transform; FInstancedStaticMeshInstanceData(); FInstancedStaticMeshInstanceData(const FMatrix& InTransform); };
class UInstancedStaticMeshComponent : public UStaticMeshComponent
{
public:
	typedef UStaticMeshComponent Super; typedef UInstancedStaticMeshComponent ThisClass; static UClass* StaticClass();
	UInstancedStaticMeshComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	TArray<FInstancedStaticMeshInstanceData> PerInstanceSMData;
	TArray<float> PerInstanceSMCustomData;
	int32 NumCustomDataFloats;
	int32 InstancingRandomSeed;
	int32 InstanceStartCullDistance; int32 InstanceEndCullDistance;
	TArray<int32> InstanceReorderTable;
	virtual int32 AddInstance(const FTransform& InstanceTransform);
	int32 AddInstanceWorldSpace(const FTransform& WorldTransform);
	virtual TArray<int32> AddInstances(const TArray<FTransform>& InstanceTransforms, bool bShouldReturnIndices);
	virtual bool SetCustomDataValue(int32 InstanceIndex, int32 CustomDataIndex, float CustomDataValue, bool bMarkRenderStateDirty = false);
	virtual bool SetCustomData(int32 InstanceIndex, const TArray<float>& InCustomData, bool bMarkRenderStateDirty = false);
	virtual void SetNumCustomDataFloats(int32 InNumCustomDataFloats);
	bool GetInstanceTransform(int32 InstanceIndex, FTransform& OutInstanceTransform, bool bWorldSpace = false) const;
	virtual bool UpdateInstanceTransform(int32 InstanceIndex, const FTransform& NewInstanceTransform, bool bWorldSpace = false, bool bMarkRenderStateDirty = false, bool bTeleport = false);
	virtual bool BatchUpdateInstancesTransforms(int32 StartInstanceIndex, const TArray<FTransform>& NewInstancesTransforms, bool bWorldSpace = false, bool bMarkRenderStateDirty = false, bool bTeleport = false);
	virtual bool BatchUpdateInstancesTransform(int32 StartInstanceIndex, int32 NumInstances, const FTransform& NewInstancesTransform, bool bWorldSpace = false, bool bMarkRenderStateDirty = false, bool bTeleport = false);
	virtual bool RemoveInstance(int32 InstanceIndex);
	virtual bool RemoveInstances(const TArray<int32>& InstancesToRemove);
	virtual void ClearInstances();
	int32 GetInstanceCount() const;
	void SetCullDistances(int32 StartCullDistance, int32 EndCullDistance);
	virtual TArray<int32> GetInstancesOverlappingSphere(const FVector& Center, float Radius, bool bSphereInWorldSpace = true) const;
	virtual TArray<int32> GetInstancesOverlappingBox(const FBox& Box, bool bBoxInWorldSpace = true) const;
	virtual void PreAllocateInstancesMemory(int32 AddedInstanceCount);
	void SetLODDistanceScale(float InLODDistanceScale);
	bool IsValidInstance(int32 InstanceIndex) const;
};

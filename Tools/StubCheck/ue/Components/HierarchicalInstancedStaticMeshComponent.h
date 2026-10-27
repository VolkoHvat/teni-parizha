#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "Components/InstancedStaticMeshComponent.h"
class UHierarchicalInstancedStaticMeshComponent : public UInstancedStaticMeshComponent
{
public:
	typedef UInstancedStaticMeshComponent Super; typedef UHierarchicalInstancedStaticMeshComponent ThisClass; static UClass* StaticClass();
	UHierarchicalInstancedStaticMeshComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	uint32 bDisableCollision : 1;
	uint32 bEnableDensityScaling : 1;
	int32 InstanceCountToRender;
	bool BuildTreeIfOutdated(bool Async, bool ForceUpdate);
	virtual int32 AddInstance(const FTransform& InstanceTransform) override;
	virtual TArray<int32> AddInstances(const TArray<FTransform>& InstanceTransforms, bool bShouldReturnIndices) override;
	virtual bool RemoveInstance(int32 InstanceIndex) override;
	virtual bool RemoveInstances(const TArray<int32>& InstancesToRemove) override;
	virtual void ClearInstances() override;
	virtual bool UpdateInstanceTransform(int32 InstanceIndex, const FTransform& NewInstanceTransform, bool bWorldSpace = false, bool bMarkRenderStateDirty = false, bool bTeleport = false) override;
	virtual void PreAllocateInstancesMemory(int32 AddedInstanceCount) override;
	bool IsTreeFullyBuilt() const;
};

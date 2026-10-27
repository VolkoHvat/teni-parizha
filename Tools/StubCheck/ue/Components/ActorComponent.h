// Макет Components/ActorComponent.h (UE 4.27)
#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Engine/EngineTypes.h"
class AActor; class UWorld; class UActorComponent;
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_TwoParams(FActorComponentActivatedSignature, UActorComponent, OnComponentActivated, UActorComponent*, Component, bool, bReset);
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_OneParam(FActorComponentDeactivateSignature, UActorComponent, OnComponentDeactivated, UActorComponent*, Component);
class UActorComponent : public UObject
{
public:
	typedef UObject Super; typedef UActorComponent ThisClass; static UClass* StaticClass();
	UActorComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	FActorComponentTickFunction PrimaryComponentTick;
	TArray<FName> ComponentTags;
	uint8 bAutoActivate : 1;
	uint8 bEditableWhenInherited : 1;
	uint8 bAutoRegister : 1;
	uint8 bWantsInitializeComponent : 1;
	uint8 bIsEditorOnly : 1;
	FActorComponentActivatedSignature OnComponentActivated;
	FActorComponentDeactivateSignature OnComponentDeactivated;
	AActor* GetOwner() const;
	template<class T> T* GetOwner() const { static_assert(sizeof(T) > 0, "GetOwner<T>: неполный тип"); return nullptr; }
	virtual UWorld* GetWorld() const override final;
	bool ComponentHasTag(FName Tag) const;
	virtual void Activate(bool bReset = false);
	virtual void Deactivate();
	virtual void SetActive(bool bNewActive, bool bReset = false);
	virtual void ToggleActive();
	bool IsActive() const;
	virtual void SetAutoActivate(bool bNewAutoActivate);
	virtual void SetComponentTickEnabled(bool bEnabled);
	bool IsComponentTickEnabled() const;
	void SetComponentTickInterval(float TickInterval);
	void SetComponentTickIntervalAndCooldown(float TickInterval);
	float GetComponentTickInterval() const;
	void SetTickGroup(ETickingGroup NewTickGroup);
	void SetTickableWhenPaused(bool bTickableWhenPaused);
	virtual void AddTickPrerequisiteActor(AActor* PrerequisiteActor);
	virtual void AddTickPrerequisiteComponent(UActorComponent* PrerequisiteComponent);
	virtual void RemoveTickPrerequisiteActor(AActor* PrerequisiteActor);
	virtual void RemoveTickPrerequisiteComponent(UActorComponent* PrerequisiteComponent);
	virtual void InitializeComponent();
	virtual void UninitializeComponent();
	virtual void BeginPlay();
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason);
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction);
	virtual void OnRegister();
	virtual void OnUnregister();
	virtual void OnComponentCreated();
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy);
	virtual void DestroyComponent(bool bPromoteChildren = false);
	void RegisterComponent();
	void UnregisterComponent();
	void RegisterComponentWithWorld(UWorld* InWorld);
	void ReregisterComponent();
	bool IsRegistered() const;
	bool HasBeenInitialized() const;
	bool HasBegunPlay() const;
	bool IsBeingDestroyed() const;
	bool IsOwnerRunningUserConstructionScript() const;
	void SetIsReplicated(bool ShouldReplicate);
	bool GetIsReplicated() const;
	ENetMode GetNetMode() const;
	bool IsNetSimulating() const;
	ENetRole GetOwnerRole() const;
	virtual void MarkRenderStateDirty();
	void MarkRenderTransformDirty();
	void RecreatePhysicsState();
	void RecreateRenderState_Concurrent();
	virtual void SetCanEverAffectNavigation(bool bRelevant);
	bool CanEverAffectNavigation() const;
	virtual void InvalidateLightingCacheDetailed(bool bInvalidateBuildEnqueuedLighting, bool bTranslationOnly);
	FString GetReadableName() const;
	UWorld* GetWorld_Uncached() const;
protected:
	uint8 bCanEverAffectNavigation : 1;
	uint8 bTickInEditor : 1;
	uint8 bNeverNeedsRenderUpdate : 1;
	uint8 bAllowConcurrentTick : 1;
	uint8 bAllowAnyoneToDestroyMe : 1;
};

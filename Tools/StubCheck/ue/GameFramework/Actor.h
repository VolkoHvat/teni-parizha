// Макет GameFramework/Actor.h (UE 4.27): bHidden/bCanBeDamaged/bReplicates/Owner/Instigator закрыты (Get/Set*).
#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Engine/EngineTypes.h"
#include "Components/ActorComponent.h"
#include "Components/SceneComponent.h"
class UInputComponent; class APlayerController; class APawn; class AController; class UDamageType; class ULevel; class UPrimitiveComponent; class FTimerManager; class UGameInstance; class AWorldSettings; struct FMinimalViewInfo;
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_FiveParams(FTakeAnyDamageSignature, AActor, OnTakeAnyDamage, AActor*, DamagedActor, float, Damage, const class UDamageType*, DamageType, class AController*, InstigatedBy, AActor*, DamageCauser);
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_TwoParams(FActorBeginOverlapSignature, AActor, OnActorBeginOverlap, AActor*, OverlappedActor, AActor*, OtherActor);
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_TwoParams(FActorEndOverlapSignature, AActor, OnActorEndOverlap, AActor*, OverlappedActor, AActor*, OtherActor);
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_FourParams(FActorHitSignature, AActor, OnActorHit, AActor*, SelfActor, AActor*, OtherActor, FVector, NormalImpulse, const FHitResult&, Hit);
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_OneParam(FActorDestroyedSignature, AActor, OnDestroyed, AActor*, DestroyedActor);
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_TwoParams(FActorEndPlaySignature, AActor, OnEndPlay, AActor*, Actor, EEndPlayReason::Type, EndPlayReason);

class AActor : public UObject
{
public:
	typedef UObject Super; typedef AActor ThisClass; static UClass* StaticClass();
	AActor(); AActor(const FObjectInitializer& ObjectInitializer);
	FActorTickFunction PrimaryActorTick;
	uint8 bNetTemporary : 1; uint8 bNetStartup : 1; uint8 bOnlyRelevantToOwner : 1; uint8 bAlwaysRelevant : 1; uint8 bReplayRewindable : 1;
	uint8 bAllowTickBeforeBeginPlay : 1; uint8 bBlockInput : 1; uint8 bFindCameraComponentWhenViewTarget : 1; uint8 bRelevantForNetworkReplays : 1;
	uint8 bGenerateOverlapEventsDuringLevelStreaming : 1; uint8 bCanBeInCluster : 1; uint8 bIgnoresOriginShifting : 1; uint8 bEnableAutoLODGeneration : 1;
	uint8 bIsEditorOnlyActor : 1; uint8 bActorSeamlessTraveled : 1; uint8 bAutoDestroyWhenFinished : 1; uint8 bNetLoadOnClient : 1; uint8 bNetUseOwnerRelevancy : 1;
	uint8 bAllowReceiveTickEventOnDedicatedServer : 1;
	ESpawnActorCollisionHandlingMethod SpawnCollisionHandlingMethod;
	float InitialLifeSpan; float CustomTimeDilation; float NetUpdateFrequency; float MinNetUpdateFrequency; float NetPriority; float NetCullDistanceSquared;
	TArray<FName> Tags; TArray<FName> Layers;
	TEnumAsByte<EAutoReceiveInput::Type> AutoReceiveInput; int32 InputPriority;
	UInputComponent* InputComponent;
	FTakeAnyDamageSignature OnTakeAnyDamage;
	FActorBeginOverlapSignature OnActorBeginOverlap; FActorEndOverlapSignature OnActorEndOverlap;
	FActorHitSignature OnActorHit; FActorDestroyedSignature OnDestroyed; FActorEndPlaySignature OnEndPlay;

	virtual UWorld* GetWorld() const override final;
	virtual void PostInitializeComponents();
	virtual void PreInitializeComponents();
	virtual void OnConstruction(const FTransform& Transform);
	virtual void PostActorCreated();
	virtual void PostLoad() override;
	virtual void BeginDestroy() override;
	virtual void Destroyed();
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason);
	virtual void Tick(float DeltaSeconds);
	virtual void TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction);
	virtual bool ShouldTickIfViewportsOnly() const;
	virtual void NotifyActorBeginOverlap(AActor* OtherActor);
	virtual void NotifyActorEndOverlap(AActor* OtherActor);
	virtual void NotifyHit(class UPrimitiveComponent* MyComp, AActor* Other, class UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit);
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser);
	virtual void SetLifeSpan(float InLifespan);
	virtual float GetLifeSpan() const;
	virtual void LifeSpanExpired();
	virtual void Reset();
	virtual void FellOutOfWorld(const class UDamageType& dmgType);
	virtual void OutsideWorldBounds();
	virtual void GetActorEyesViewPoint(FVector& OutLocation, FRotator& OutRotation) const;
	virtual FVector GetVelocity() const;
	virtual void CalcCamera(float DeltaTime, struct FMinimalViewInfo& OutResult);
	virtual bool HasActiveCameraComponent() const;
	virtual void EnableInput(class APlayerController* PlayerController);
	virtual void DisableInput(class APlayerController* PlayerController);
	virtual void SetActorHiddenInGame(bool bNewHidden);
	virtual void SetOwner(AActor* NewOwner);
	virtual void SetReplicateMovement(bool bInReplicateMovement);
	virtual bool TeleportTo(const FVector& DestLocation, const FRotator& DestRotation, bool bIsATest = false, bool bNoCheck = false);
	virtual void GetActorBounds(bool bOnlyCollidingComponents, FVector& Origin, FVector& BoxExtent, bool bIncludeFromChildActors = false) const;
	virtual void GetSimpleCollisionCylinder(float& CollisionRadius, float& CollisionHalfHeight) const;
	virtual void AddTickPrerequisiteActor(AActor* PrerequisiteActor);
	virtual void AddTickPrerequisiteComponent(UActorComponent* PrerequisiteComponent);
	virtual void RemoveTickPrerequisiteActor(AActor* PrerequisiteActor);
	virtual void RemoveTickPrerequisiteComponent(UActorComponent* PrerequisiteComponent);
	virtual bool IsLevelBoundsRelevant() const;
	virtual void K2_DestroyActor();
	virtual bool CanBeBaseForCharacter(class APawn* Pawn) const;
	virtual void BecomeViewTarget(class APlayerController* PC);
	virtual void EndViewTarget(class APlayerController* PC);
	virtual FString GetHumanReadableName() const;
	virtual void ClearCrossLevelReferences();
	virtual void PostRegisterAllComponents();
	virtual void PostUnregisterAllComponents();
	virtual void RegisterActorTickFunctions(bool bRegister);
#if WITH_EDITORONLY_DATA
	uint8 bRunConstructionScriptOnDrag : 1; uint8 bIsEditorPreviewActor : 1; uint8 bHiddenEd : 1;
#endif
#if WITH_EDITOR
	virtual void PostEditMove(bool bFinished);
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	void SetActorLabel(const FString& NewActorLabel, bool bMarkDirty = true);
	const FString& GetActorLabel() const;
	void SetFolderPath(const FName& NewFolderPath);
#endif

	FVector GetActorLocation() const; FRotator GetActorRotation() const; FQuat GetActorQuat() const;
	FVector GetActorScale3D() const; FVector GetActorScale() const; const FTransform& GetActorTransform() const; const FTransform& GetTransform() const; const FTransform& ActorToWorld() const;
	FVector GetActorForwardVector() const; FVector GetActorRightVector() const; FVector GetActorUpVector() const;
	FVector GetActorRelativeScale3D() const; FVector GetTargetLocation(AActor* RequestedBy = nullptr) const;
	bool SetActorLocation(const FVector& NewLocation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	bool SetActorRotation(FRotator NewRotation, ETeleportType Teleport = ETeleportType::None);
	bool SetActorRotation(const FQuat& NewRotation, ETeleportType Teleport = ETeleportType::None);
	bool SetActorLocationAndRotation(FVector NewLocation, FRotator NewRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	bool SetActorLocationAndRotation(FVector NewLocation, const FQuat& NewRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void SetActorScale3D(FVector NewScale3D);
	void SetActorRelativeScale3D(FVector NewRelativeScale);
	bool SetActorTransform(const FTransform& NewTransform, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void SetActorRelativeLocation(FVector NewRelativeLocation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void SetActorRelativeRotation(FRotator NewRelativeRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void SetActorRelativeRotation(const FQuat& NewRelativeRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void SetActorRelativeTransform(const FTransform& NewRelativeTransform, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void AddActorWorldOffset(FVector DeltaLocation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void AddActorWorldRotation(FRotator DeltaRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void AddActorWorldRotation(const FQuat& DeltaRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void AddActorWorldTransform(const FTransform& DeltaTransform, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void AddActorLocalOffset(FVector DeltaLocation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void AddActorLocalRotation(FRotator DeltaRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void AddActorLocalRotation(const FQuat& DeltaRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void AddActorLocalTransform(const FTransform& NewTransform, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	float GetDistanceTo(const AActor* OtherActor) const; float GetSquaredDistanceTo(const AActor* OtherActor) const;
	float GetHorizontalDistanceTo(const AActor* OtherActor) const; float GetVerticalDistanceTo(const AActor* OtherActor) const;
	float GetDotProductTo(const AActor* OtherActor) const; float GetHorizontalDotProductTo(const AActor* OtherActor) const;
	float GetSimpleCollisionRadius() const; float GetSimpleCollisionHalfHeight() const;
	FBox GetComponentsBoundingBox(bool bNonColliding = false, bool bIncludeFromChildActors = false) const;

	USceneComponent* GetRootComponent() const; USceneComponent* K2_GetRootComponent() const;
	bool SetRootComponent(USceneComponent* NewRootComponent);
	AActor* GetOwner() const;
	template<class T> T* GetOwner() const { static_assert(sizeof(T) > 0, "GetOwner<T>: неполный тип"); return nullptr; }
	APawn* GetInstigator() const;
	template<class T> T* GetInstigator() const { return nullptr; }
	AController* GetInstigatorController() const;
	template<class T> T* GetInstigatorController() const { return nullptr; }
	void SetInstigator(APawn* InInstigator);
	void GetComponents(TArray<UActorComponent*>& OutComponents, bool bIncludeFromChildActors = false) const;
	template<class T, class AllocatorType> void GetComponents(TArray<T*, AllocatorType>& OutComponents, bool bIncludeFromChildActors = false) const { static_assert(std::is_base_of<UActorComponent, T>::value, "GetComponents: T должен быть компонентом (нужен #include)"); }
	template<class T, class AllocatorType> void GetComponents(TSubclassOf<UActorComponent> ComponentClass, TArray<T*, AllocatorType>& OutComponents, bool bIncludeFromChildActors = false) const;
	template<class T> T* FindComponentByClass() const { static_assert(std::is_base_of<UActorComponent, T>::value, "FindComponentByClass: T должен быть компонентом (нужен #include)"); return nullptr; }
	UActorComponent* FindComponentByClass(const TSubclassOf<UActorComponent> ComponentClass) const;
	UActorComponent* GetComponentByClass(TSubclassOf<UActorComponent> ComponentClass) const;
	TArray<UActorComponent*> GetComponentsByClass(TSubclassOf<UActorComponent> ComponentClass) const;
	TArray<UActorComponent*> GetComponentsByTag(TSubclassOf<UActorComponent> ComponentClass, FName Tag) const;
	TArray<UActorComponent*> GetComponentsByInterface(TSubclassOf<UInterface> Interface) const;
	const TSet<UActorComponent*>& GetComponents() const;
	void AddInstanceComponent(UActorComponent* Component);
	void RemoveInstanceComponent(UActorComponent* Component);
	void ClearInstanceComponents(bool bDestroyComponents);
	const TArray<UActorComponent*>& GetInstanceComponents() const;
	void AddOwnedComponent(UActorComponent* Component);
	void RemoveOwnedComponent(UActorComponent* Component);
	void RegisterAllComponents(); void ReregisterAllComponents(); void UnregisterAllComponents(bool bForReregister = false);
	void UpdateComponentTransforms(); void MarkComponentsRenderStateDirty();
	void AttachToActor(AActor* ParentActor, const FAttachmentTransformRules& AttachmentRules, FName SocketName = NAME_None);
	void AttachToComponent(USceneComponent* Parent, const FAttachmentTransformRules& AttachmentRules, FName SocketName = NAME_None);
	void DetachFromActor(const FDetachmentTransformRules& DetachmentRules);
	void DetachAllSceneComponents(class USceneComponent* InParentComponent, const FDetachmentTransformRules& DetachmentRules);
	AActor* GetAttachParentActor() const; FName GetAttachParentSocketName() const;
	void GetAttachedActors(TArray<AActor*>& OutActors, bool bResetArray = true) const;
	AActor* GetParentActor() const; class UChildActorComponent* GetParentComponent() const;
	bool Destroy(bool bNetForce = false, bool bShouldModifyLevel = true);
	bool IsActorBeingDestroyed() const; bool IsPendingKillPending() const; bool HasActorBegunPlay() const; bool IsActorInitialized() const; bool IsActorTickEnabled() const;
	bool ActorHasTag(FName Tag) const;
	void SetActorTickEnabled(bool bEnabled); void SetActorTickInterval(float TickInterval); float GetActorTickInterval() const;
	void SetTickGroup(ETickingGroup NewTickGroup); void SetTickableWhenPaused(bool bTickableWhenPaused); bool GetTickableWhenPaused();
	void SetActorEnableCollision(bool bNewActorEnableCollision); bool GetActorEnableCollision() const;
	bool IsHidden() const; void SetHidden(bool bInHidden);
	bool CanBeDamaged() const; void SetCanBeDamaged(bool bInCanBeDamaged);
	void SetReplicates(bool bInReplicates); bool GetIsReplicated() const; bool IsReplicatingMovement() const;
	bool HasAuthority() const; ENetRole GetLocalRole() const; ENetRole GetRemoteRole() const; ENetMode GetNetMode() const; bool IsNetMode(ENetMode Mode) const;
	void ForceNetUpdate(); void FlushNetDormancy();
	FTimerManager& GetWorldTimerManager() const;
	UGameInstance* GetGameInstance() const;
	template<class T> T* GetGameInstance() const { return nullptr; }
	ULevel* GetLevel() const; AWorldSettings* GetWorldSettings() const;
	float GetGameTimeSinceCreation() const; float GetActorTimeDilation() const;
	bool WasRecentlyRendered(float Tolerance = 0.2f) const; float GetLastRenderTime() const;
	bool IsOverlappingActor(const AActor* Other) const;
	void GetOverlappingActors(TArray<AActor*>& OverlappingActors, TSubclassOf<AActor> ClassFilter = nullptr) const;
	void GetOverlappingComponents(TArray<UPrimitiveComponent*>& OverlappingComponents) const;
	void UpdateOverlaps(bool bDoNotifies = true);
	bool IsRootComponentMovable() const; bool IsRootComponentStatic() const;
	void DispatchBeginPlay(bool bFromLevelStreaming = false);
	bool IsSelectable() const;
	void FinishSpawning(const FTransform& Transform, bool bIsDefaultTransform = false, const struct FComponentInstanceDataCache* InstanceDataCache = nullptr);
	bool IsOwnedBy(const AActor* TestOwner) const;
	bool IsChildActor() const;
	void SetNetDormancy(enum ENetDormancy NewDormancy);
	void MakeNoise(float Loudness = 1.f, APawn* NoiseInstigator = nullptr, FVector NoiseLocation = FVector::ZeroVector, float MaxRange = 0.f, FName Tag = NAME_None);
	USceneComponent* GetDefaultAttachComponent() const;
protected:
	USceneComponent* RootComponent;
	virtual void BeginPlay();
};


enum { NumInlinedActorComponents = 24 };
template<class T, uint32 NumElements = NumInlinedActorComponents>
class TInlineComponentArray : public TArray<T, TInlineAllocator<NumElements>>
{
public:
	TInlineComponentArray();
	explicit TInlineComponentArray(const AActor* Actor, bool bIncludeFromChildActors = false);
};

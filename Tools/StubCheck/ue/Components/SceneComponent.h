// Макет Components/SceneComponent.h (UE 4.27): RelativeLocation и пр. закрыты, доступ через Get/SetRelative*.
#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
class USceneComponent;
enum class EUpdateTransformFlags : int32 { None = 0x0, SkipPhysicsUpdate = 0x1, PropagateFromParent = 0x2, OnlyUpdateIfUsingSocket = 0x4 };
ENUM_CLASS_FLAGS(EUpdateTransformFlags)
enum EMoveComponentFlags { MOVECOMP_NoFlags = 0x0, MOVECOMP_IgnoreBases = 0x1, MOVECOMP_SkipPhysicsMove = 0x2, MOVECOMP_NeverIgnoreBlockingOverlaps = 0x4, MOVECOMP_DisableBlockingOverlapDispatch = 0x8 };
inline EMoveComponentFlags operator|(EMoveComponentFlags A, EMoveComponentFlags B) { return (EMoveComponentFlags)((int)A | (int)B); }
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_OneParam(FPhysicsVolumeChanged, USceneComponent, PhysicsVolumeChangedDelegate, class APhysicsVolume*, NewVolume);
class USceneComponent : public UActorComponent
{
public:
	typedef UActorComponent Super; typedef USceneComponent ThisClass; static UClass* StaticClass();
	USceneComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	FBoxSphereBounds Bounds;
	TEnumAsByte<EComponentMobility::Type> Mobility;
	FVector ComponentVelocity;
	uint8 bShouldUpdatePhysicsVolume : 1;
	uint8 bUseAttachParentBound : 1;
	void SetupAttachment(USceneComponent* InParent, FName InSocketName = NAME_None);
	bool AttachToComponent(USceneComponent* InParent, const FAttachmentTransformRules& AttachmentRules, FName InSocketName = NAME_None);
	virtual void DetachFromComponent(const FDetachmentTransformRules& DetachmentRules);
	USceneComponent* GetAttachParent() const;
	AActor* GetAttachParentActor() const;
	FName GetAttachSocketName() const;
	const TArray<USceneComponent*>& GetAttachChildren() const;
	void GetParentComponents(TArray<USceneComponent*>& Parents) const;
	int32 GetNumChildrenComponents() const;
	USceneComponent* GetChildComponent(int32 ChildIndex) const;
	void GetChildrenComponents(bool bIncludeAllDescendants, TArray<USceneComponent*>& Children) const;
	bool IsAttachedTo(const USceneComponent* TestComp) const;

	void SetRelativeLocation(FVector NewLocation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void SetRelativeRotation(FRotator NewRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void SetRelativeRotation(const FQuat& NewRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void SetRelativeTransform(const FTransform& NewTransform, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void SetRelativeLocationAndRotation(FVector NewLocation, FRotator NewRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void SetRelativeLocationAndRotation(FVector NewLocation, const FQuat& NewRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	virtual void SetRelativeScale3D(FVector NewScale3D);
	void AddRelativeLocation(FVector DeltaLocation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void AddRelativeRotation(FRotator DeltaRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void AddRelativeRotation(const FQuat& DeltaRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void AddLocalOffset(FVector DeltaLocation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void AddLocalRotation(FRotator DeltaRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void AddLocalRotation(const FQuat& DeltaRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void AddLocalTransform(const FTransform& DeltaTransform, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void SetWorldLocation(FVector NewLocation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void SetWorldRotation(FRotator NewRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void SetWorldRotation(const FQuat& NewRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void SetWorldScale3D(FVector NewScale);
	void SetWorldTransform(const FTransform& NewTransform, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void SetWorldLocationAndRotation(FVector NewLocation, FRotator NewRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void SetWorldLocationAndRotation(FVector NewLocation, const FQuat& NewRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void SetWorldLocationAndRotationNoPhysics(const FVector& NewLocation, const FRotator& NewRotation);
	void AddWorldOffset(FVector DeltaLocation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void AddWorldRotation(FRotator DeltaRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void AddWorldRotation(const FQuat& DeltaRotation, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void AddWorldTransform(const FTransform& DeltaTransform, bool bSweep = false, FHitResult* OutSweepHitResult = nullptr, ETeleportType Teleport = ETeleportType::None);
	void ResetRelativeTransform();
	FVector GetRelativeLocation() const; FRotator GetRelativeRotation() const; FVector GetRelativeScale3D() const; FTransform GetRelativeTransform() const;
	void SetRelativeLocation_Direct(const FVector NewRelativeLocation); void SetRelativeRotation_Direct(const FRotator NewRelativeRotation); void SetRelativeScale3D_Direct(const FVector NewRelativeScale3D);
	FVector GetComponentLocation() const; FRotator GetComponentRotation() const; FQuat GetComponentQuat() const; FVector GetComponentScale() const;
	const FTransform& GetComponentTransform() const; const FTransform& GetComponentToWorld() const;
	FVector GetForwardVector() const; FVector GetRightVector() const; FVector GetUpVector() const;
	virtual FVector GetComponentVelocity() const;
	virtual FTransform GetSocketTransform(FName InSocketName, ERelativeTransformSpace TransformSpace = RTS_World) const;
	FVector GetSocketLocation(FName InSocketName) const;
	FRotator GetSocketRotation(FName InSocketName) const;
	FQuat GetSocketQuaternion(FName InSocketName) const;
	virtual bool DoesSocketExist(FName InSocketName) const;
	virtual bool HasAnySockets() const;
	void SetVisibility(bool bNewVisibility, bool bPropagateToChildren = false);
	void ToggleVisibility(bool bPropagateToChildren = false);
	bool IsVisible() const; bool GetVisibleFlag() const;
	void SetHiddenInGame(bool NewHidden, bool bPropagateToChildren = false);
	virtual void SetMobility(EComponentMobility::Type NewMobility);
	bool IsSimulatingPhysics(FName BoneName = NAME_None) const;
	bool IsAnySimulatingPhysics() const;
	void SetAbsolute(bool bNewAbsoluteLocation = false, bool bNewAbsoluteRotation = false, bool bNewAbsoluteScale = false);
	void SetUsingAbsoluteLocation(bool bInAbsoluteLocation); void SetUsingAbsoluteRotation(bool bInAbsoluteRotation); void SetUsingAbsoluteScale(bool bInAbsoluteScale);
	bool IsUsingAbsoluteLocation() const; bool IsUsingAbsoluteRotation() const; bool IsUsingAbsoluteScale() const;
	void UpdateComponentToWorld(EUpdateTransformFlags UpdateTransformFlags = EUpdateTransformFlags::None, ETeleportType Teleport = ETeleportType::None);
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const;
	virtual void UpdateBounds();
	bool MoveComponent(const FVector& Delta, const FQuat& NewRotation, bool bSweep, FHitResult* OutHit = nullptr, EMoveComponentFlags MoveFlags = MOVECOMP_NoFlags, ETeleportType Teleport = ETeleportType::None);
	bool MoveComponent(const FVector& Delta, const FRotator& NewRotation, bool bSweep, FHitResult* OutHit = nullptr, EMoveComponentFlags MoveFlags = MOVECOMP_NoFlags, ETeleportType Teleport = ETeleportType::None);
	virtual bool IsWorldGeometry() const;
	virtual ECollisionEnabled::Type GetCollisionEnabled() const;
	virtual ECollisionResponse GetCollisionResponseToChannel(ECollisionChannel Channel) const;
	virtual ECollisionChannel GetCollisionObjectType() const;
	virtual void OnAttachmentChanged();
	virtual void OnChildAttached(USceneComponent* ChildComponent);
	virtual void OnChildDetached(USceneComponent* ChildComponent);
	FPhysicsVolumeChanged PhysicsVolumeChangedDelegate;
};

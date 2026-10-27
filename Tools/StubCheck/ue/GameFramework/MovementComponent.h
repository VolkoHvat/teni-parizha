#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
class UPrimitiveComponent; class APhysicsVolume;
enum class EPlaneConstraintAxisSetting : uint8 { Custom, X, Y, Z, UseGlobalPhysicsSetting };
class UMovementComponent : public UActorComponent
{
public:
	typedef UActorComponent Super; typedef UMovementComponent ThisClass; static UClass* StaticClass();
	UMovementComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	USceneComponent* UpdatedComponent; UPrimitiveComponent* UpdatedPrimitive;
	FVector Velocity;
	uint8 bConstrainToPlane : 1; uint8 bSnapToPlaneAtStart : 1; uint8 bUpdateOnlyIfRendered : 1; uint8 bAutoUpdateTickRegistration : 1; uint8 bTickBeforeOwner : 1; uint8 bAutoRegisterUpdatedComponent : 1;
	EPlaneConstraintAxisSetting PlaneConstraintAxisSetting; FVector PlaneConstraintNormal; FVector PlaneConstraintOrigin;
	virtual void SetUpdatedComponent(USceneComponent* NewUpdatedComponent);
	virtual float GetGravityZ() const;
	virtual float GetMaxSpeed() const;
	virtual void StopMovementImmediately();
	virtual void UpdateComponentVelocity();
	virtual bool IsExceedingMaxSpeed(float MaxSpeed) const;
	virtual bool IsInWater() const;
	virtual void PhysicsVolumeChanged(APhysicsVolume* NewVolume);
	APhysicsVolume* GetPhysicsVolume() const;
	bool MoveUpdatedComponent(const FVector& Delta, const FQuat& NewRotation, bool bSweep, FHitResult* OutHit = nullptr, ETeleportType Teleport = ETeleportType::None);
	bool MoveUpdatedComponent(const FVector& Delta, const FRotator& NewRotation, bool bSweep, FHitResult* OutHit = nullptr, ETeleportType Teleport = ETeleportType::None);
	bool SafeMoveUpdatedComponent(const FVector& Delta, const FQuat& NewRotation, bool bSweep, FHitResult& OutHit, ETeleportType Teleport = ETeleportType::None);
	bool SafeMoveUpdatedComponent(const FVector& Delta, const FRotator& NewRotation, bool bSweep, FHitResult& OutHit, ETeleportType Teleport = ETeleportType::None);
	virtual float SlideAlongSurface(const FVector& Delta, float Time, const FVector& Normal, FHitResult& Hit, bool bHandleImpact = false);
	virtual FVector ComputeSlideVector(const FVector& Delta, const float Time, const FVector& Normal, const FHitResult& Hit) const;
	virtual bool ResolvePenetration(const FVector& Adjustment, const FHitResult& Hit, const FQuat& NewRotation);
	virtual void HandleImpact(const FHitResult& Hit, float TimeSlice = 0.f, const FVector& MoveDelta = FVector::ZeroVector);
	virtual void SetPlaneConstraintAxisSetting(EPlaneConstraintAxisSetting NewAxisSetting);
	virtual void SetPlaneConstraintNormal(FVector PlaneNormal);
	virtual void SetPlaneConstraintFromVectors(FVector Forward, FVector Up);
	virtual void SetPlaneConstraintOrigin(FVector PlaneOrigin);
	virtual void SetPlaneConstraintEnabled(bool bEnabled);
	const FVector& GetPlaneConstraintNormal() const;
	virtual FVector ConstrainDirectionToPlane(FVector Direction) const;
	virtual FVector ConstrainLocationToPlane(FVector Location) const;
	virtual FVector ConstrainNormalToPlane(FVector Normal) const;
	virtual void SnapUpdatedComponentToPlane();
	virtual float GetMaxSpeedModifier() const;
	float K2_GetModifiedMaxSpeed() const;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};

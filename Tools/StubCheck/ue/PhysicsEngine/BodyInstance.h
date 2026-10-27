#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
struct FBodyInstance
{
	FBodyInstance();
	uint8 bUseCCD : 1; uint8 bNotifyRigidBodyCollision : 1; uint8 bSimulatePhysics : 1; uint8 bOverrideMass : 1; uint8 bEnableGravity : 1; uint8 bLockXRotation : 1; uint8 bLockYRotation : 1; uint8 bLockZRotation : 1;
	float LinearDamping; float AngularDamping; float MassScale; FVector COMNudge; float MaxAngularVelocity;
	void SetCollisionProfileName(FName InCollisionProfileName); FName GetCollisionProfileName() const;
	void SetCollisionEnabled(ECollisionEnabled::Type NewType, bool bUpdatePhysicsFilterData = true); ECollisionEnabled::Type GetCollisionEnabled() const;
	void SetResponseToChannel(ECollisionChannel Channel, ECollisionResponse NewResponse); void SetResponseToAllChannels(ECollisionResponse NewResponse);
	void SetObjectType(ECollisionChannel Channel); ECollisionChannel GetObjectType() const;
	void SetInstanceSimulatePhysics(bool bSimulate, bool bMaintainPhysicsBlending = false); bool IsInstanceSimulatingPhysics() const;
	void SetMassOverride(float MassInKG, bool bNewOverrideMass = true); float GetBodyMass() const;
	void SetLinearVelocity(const FVector& NewVel, bool bAddToCurrent, bool bAutoWake = true); FVector GetUnrealWorldVelocity() const;
	void AddImpulse(const FVector& Impulse, bool bVelChange); void AddForce(const FVector& Force, bool bAllowSubstepping = true, bool bAccelChange = false);
	void SetEnableGravity(bool bGravityEnabled); void WakeInstance(); void PutInstanceToSleep(); bool IsValidBodyInstance() const;
	void SetUseCCD(bool bInUseCCD);
};

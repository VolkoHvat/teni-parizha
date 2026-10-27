#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "GameFramework/MovementComponent.h"
struct FMovementProperties { uint8 bCanCrouch : 1; uint8 bCanJump : 1; uint8 bCanWalk : 1; uint8 bCanSwim : 1; uint8 bCanFly : 1; };
struct FNavAgentProperties : public FMovementProperties { float AgentRadius; float AgentHeight; float AgentStepHeight; float NavWalkingSearchHeightScale; FNavAgentProperties(float Radius = -1.f, float Height = -1.f); bool IsValid() const; };
class UNavMovementComponent : public UMovementComponent
{
public:
	typedef UMovementComponent Super; typedef UNavMovementComponent ThisClass; static UClass* StaticClass();
	UNavMovementComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	FNavAgentProperties NavAgentProps; float FixedPathBrakingDistance;
	uint8 bUpdateNavAgentWithOwnersCollision : 1; uint8 bUseAccelerationForPaths : 1; uint8 bUseFixedBrakingDistanceForPaths : 1; uint8 bStopMovementAbortPaths : 1;
	FMovementProperties MovementState;
	virtual void StopActiveMovement();
	virtual FVector GetActorFeetLocation() const;
	virtual FVector GetActorNavLocation() const;
	virtual bool IsCrouching() const; virtual bool IsFalling() const; virtual bool IsMovingOnGround() const; virtual bool IsSwimming() const; virtual bool IsFlying() const;
	bool CanEverCrouch() const; bool CanEverJump() const; bool CanEverMoveOnGround() const; bool CanEverSwim() const; bool CanEverFly() const;
	virtual void RequestDirectMove(const FVector& MoveVelocity, bool bForceMaxSpeed);
	virtual void RequestPathMove(const FVector& MoveInput);
	virtual bool CanStartPathFollowing() const; virtual bool CanStopPathFollowing() const;
	const FNavAgentProperties& GetNavAgentPropertiesRef() const;
	virtual void UpdateNavAgent(const AActor& Owner);
	virtual float GetPathFollowingBrakingDistance(float MaxSpeed) const;
	void SetFixedBrakingDistance(float DistanceToEndOfPath); void ClearFixedBrakingDistance();
};

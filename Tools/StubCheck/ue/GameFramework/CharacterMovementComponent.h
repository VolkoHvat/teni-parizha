// Макет GameFramework/CharacterMovementComponent.h (UE 4.27): WalkableFloorAngle закрыт — SetWalkableFloorAngle().
#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "GameFramework/PawnMovementComponent.h"
class ACharacter;
struct FFindFloorResult
{
	uint32 bBlockingHit : 1; uint32 bWalkableFloor : 1; uint32 bLineTrace : 1;
	float FloorDist; float LineDist; FHitResult HitResult;
	FFindFloorResult(); bool IsWalkableFloor() const; void Clear(); float GetDistanceToFloor() const;
	void SetFromSweep(const FHitResult& InHit, const float InSweepFloorDist, const bool bIsWalkableFloor);
	void SetFromLineTrace(const FHitResult& InHit, const float InSweepFloorDist, const float InLineDist, const bool bIsWalkableFloor);
};
enum class ENetworkSmoothingMode : uint8 { Disabled, Linear, Exponential, Replay };
class UCharacterMovementComponent : public UPawnMovementComponent
{
public:
	typedef UPawnMovementComponent Super; typedef UCharacterMovementComponent ThisClass; static UClass* StaticClass();
	UCharacterMovementComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	float GravityScale; float MaxStepHeight; float JumpZVelocity; float JumpOffJumpZFactor;
	TEnumAsByte<enum EMovementMode> MovementMode; uint8 CustomMovementMode;
	ENetworkSmoothingMode NetworkSmoothingMode;
	float GroundFriction; float MaxWalkSpeed; float MaxWalkSpeedCrouched; float MaxSwimSpeed; float MaxFlySpeed; float MaxCustomMovementSpeed; float MaxAcceleration; float MinAnalogWalkSpeed;
	float BrakingFrictionFactor; float BrakingFriction; float BrakingSubStepTime;
	float BrakingDecelerationWalking; float BrakingDecelerationFalling; float BrakingDecelerationSwimming; float BrakingDecelerationFlying;
	float AirControl; float AirControlBoostMultiplier; float AirControlBoostVelocityThreshold; float FallingLateralFriction;
	float CrouchedHalfHeight; float Buoyancy; float PerchRadiusThreshold; float PerchAdditionalHeight;
	FRotator RotationRate;
	uint8 bUseSeparateBrakingFriction : 1; uint8 bApplyGravityWhileJumping : 1; uint8 bUseControllerDesiredRotation : 1; uint8 bOrientRotationToMovement : 1;
	uint8 bSweepWhileNavWalking : 1; uint8 bMovementInProgress : 1; uint8 bEnableScopedMovementUpdates : 1; uint8 bEnableServerDualMoveScopedMovementUpdates : 1;
	uint8 bForceMaxAccel : 1; uint8 bRunPhysicsWithNoController : 1; uint8 bForceNextFloorCheck : 1; uint8 bShrinkProxyCapsule : 1;
	uint8 bCanWalkOffLedges : 1; uint8 bCanWalkOffLedgesWhenCrouching : 1; uint8 bNetworkSkipProxyPredictionOnNetUpdate : 1; uint8 bDeferUpdateMoveComponent : 1;
	uint8 bEnablePhysicsInteraction : 1; uint8 bTouchForceScaledToMass : 1; uint8 bPushForceScaledToMass : 1; uint8 bPushForceUsingZOffset : 1; uint8 bScalePushForceToVelocity : 1;
	uint8 bImpartBaseVelocityX : 1; uint8 bImpartBaseVelocityY : 1; uint8 bImpartBaseVelocityZ : 1; uint8 bImpartBaseAngularVelocity : 1; uint8 bJustTeleported : 1;
	uint8 bNetworkUpdateReceived : 1; uint8 bNetworkMovementModeChanged : 1; uint8 bIgnoreClientMovementErrorChecksAndCorrection : 1; uint8 bServerAcceptClientAuthoritativePosition : 1;
	uint8 bNotifyApex : 1; uint8 bCheatFlying : 1; uint8 bWantsToCrouch : 1; uint8 bCrouchMaintainsBaseLocation : 1; uint8 bIgnoreBaseRotation : 1; uint8 bFastAttachedMove : 1;
	uint8 bAlwaysCheckFloor : 1; uint8 bUseFlatBaseForFloorChecks : 1; uint8 bPerformingJumpOff : 1; uint8 bWantsToLeaveNavWalking : 1; uint8 bUseRVOAvoidance : 1;
	uint8 bRequestedMoveUseAcceleration : 1; uint8 bWasSimulatingRootMotion : 1; uint8 bAllowPhysicsRotationDuringAnimRootMotion : 1; uint8 bMaintainHorizontalGroundVelocity : 1;
	uint8 bProjectNavMeshWalking : 1; uint8 bProjectNavMeshOnBothWorldChannels : 1;
	float LedgeCheckThreshold; float JumpOutOfWaterPitch; float Mass; float StandingDownwardForceScale; float InitialPushForceFactor; float PushForceFactor;
	float PushForcePointZOffsetFactor; float TouchForceFactor; float MinTouchForce; float MaxTouchForce; float RepulsionForce;
	float MaxOutOfWaterStepHeight; float OutofWaterZ; float MaxSimulationTimeStep; int32 MaxSimulationIterations; int32 MaxJumpApexAttemptsPerSimulation;
	float MaxDepenetrationWithGeometry; float MaxDepenetrationWithGeometryAsProxy; float MaxDepenetrationWithPawn; float MaxDepenetrationWithPawnAsProxy;
	float AvoidanceConsiderationRadius; float AvoidanceWeight;
	FFindFloorResult CurrentFloor;
	TEnumAsByte<enum EMovementMode> DefaultLandMovementMode; TEnumAsByte<enum EMovementMode> DefaultWaterMovementMode;
	FVector LastUpdateLocation; FQuat LastUpdateRotation; FVector LastUpdateVelocity;
	FVector PendingLaunchVelocity;
	virtual void SetMovementMode(EMovementMode NewMovementMode, uint8 NewCustomMode = 0);
	virtual void SetDefaultMovementMode();
	void SetGroundMovementMode(EMovementMode NewGroundMovementMode); EMovementMode GetGroundMovementMode() const;
	virtual bool IsMovingOnGround() const override; virtual bool IsFalling() const override; virtual bool IsCrouching() const override;
	virtual bool IsSwimming() const override; virtual bool IsFlying() const override;
	bool IsWalking() const;
	virtual bool IsWalkable(const FHitResult& Hit) const;
	float GetWalkableFloorAngle() const; void SetWalkableFloorAngle(float InWalkableFloorAngle);
	float GetWalkableFloorZ() const; void SetWalkableFloorZ(float InWalkableFloorZ);
	virtual void DisableMovement();
	virtual void StopMovementImmediately() override;
	virtual void StopActiveMovement() override;
	virtual void AddImpulse(FVector Impulse, bool bVelocityChange = false);
	virtual void AddForce(FVector Force);
	virtual void Launch(FVector const& LaunchVel);
	virtual bool DoJump(bool bReplayingMoves);
	virtual bool CanAttemptJump() const;
	virtual float GetMaxSpeed() const override;
	virtual float GetMaxAcceleration() const;
	virtual float GetMaxBrakingDeceleration() const;
	virtual float GetMinAnalogSpeed() const;
	virtual float GetGravityZ() const override;
	FVector GetCurrentAcceleration() const;
	FVector GetLastUpdateVelocity() const; FVector GetLastUpdateLocation() const; FRotator GetLastUpdateRotation() const;
	ACharacter* GetCharacterOwner() const;
	UPrimitiveComponent* GetMovementBase() const;
	float GetPerchRadiusThreshold() const; float GetValidPerchRadius() const;
	float GetMaxJumpHeight() const; float GetMaxJumpHeightWithJumpTime() const;
	float GetAnalogInputModifier() const;
	virtual void CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration);
	virtual void FindFloor(const FVector& CapsuleLocation, FFindFloorResult& OutFloorResult, bool bCanUseCachedLocation, const FHitResult* DownwardSweepResult = nullptr) const;
	virtual void ComputeFloorDist(const FVector& CapsuleLocation, float LineDistance, float SweepDistance, FFindFloorResult& OutFloorResult, float SweepRadius, const FHitResult* DownwardSweepResult = nullptr) const;
	virtual bool IsValidLandingSpot(const FVector& CapsuleLocation, const FHitResult& Hit) const;
	virtual void SetUpdatedComponent(USceneComponent* NewUpdatedComponent) override;
	bool HasValidData() const;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void RequestDirectMove(const FVector& MoveVelocity, bool bForceMaxSpeed) override;
	void ClearAccumulatedForces();
	virtual FVector GetImpartedMovementBaseVelocity() const;
	void SetAvoidanceEnabled(bool bEnable);
	virtual void Crouch(bool bClientSimulation = false);
	virtual void UnCrouch(bool bClientSimulation = false);
	virtual bool CanCrouchInCurrentState() const;
	virtual FVector ConstrainInputAcceleration(const FVector& InputAcceleration) const;
	virtual FVector ScaleInputAcceleration(const FVector& InputAcceleration) const;
	virtual void MaybeUpdateBasedMovement(float DeltaSeconds);
	virtual FRotator ComputeOrientToMovementRotation(const FRotator& CurrentRotation, float DeltaTime, FRotator& DeltaRotation) const;
	virtual void PhysicsRotation(float DeltaTime);
	virtual FRotator GetDeltaRotation(float DeltaTime) const;
protected:
	virtual void PhysCustom(float deltaTime, int32 Iterations);
	virtual void PhysWalking(float deltaTime, int32 Iterations);
	virtual void PhysFalling(float deltaTime, int32 Iterations);
	virtual void PhysFlying(float deltaTime, int32 Iterations);
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode);
	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity);
	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds);
	virtual void UpdateCharacterStateAfterMovement(float DeltaSeconds);
	virtual void ProcessLanded(const FHitResult& Hit, float remainingTime, int32 Iterations);
	virtual void SetPostLandedPhysics(const FHitResult& Hit);
	ACharacter* CharacterOwner;
	FVector Acceleration; FVector PendingImpulseToApply; FVector PendingForceToApply; float AnalogInputModifier;
};

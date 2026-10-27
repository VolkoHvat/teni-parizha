// Макет GameFramework/Character.h (UE 4.27): Mesh/CapsuleComponent/CharacterMovement закрыты — GetMesh() и т. д.
#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
class USkeletalMeshComponent; class UCapsuleComponent; class UCharacterMovementComponent; class UAnimMontage; class UArrowComponent;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLandedSignature, const FHitResult&, Hit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FMovementModeChangedSignature, class ACharacter*, Character, EMovementMode, PrevMovementMode, uint8, PreviousCustomMode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FCharacterMovementUpdatedSignature, float, DeltaSeconds, FVector, OldLocation, FVector, OldVelocity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCharacterReachedApexSignature);
class ACharacter : public APawn
{
public:
	typedef APawn Super; typedef ACharacter ThisClass; static UClass* StaticClass();
	ACharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	static FName MeshComponentName; static FName CharacterMovementComponentName; static FName CapsuleComponentName;
	USkeletalMeshComponent* GetMesh() const;
	UCapsuleComponent* GetCapsuleComponent() const;
	UCharacterMovementComponent* GetCharacterMovement() const;
	UArrowComponent* GetArrowComponent() const;
	float CrouchedEyeHeight;
	uint32 bIsCrouched : 1; uint32 bPressedJump : 1; uint32 bClientUpdating : 1; uint32 bClientWasFalling : 1; uint32 bClientResimulateRootMotion : 1;
	uint32 bClientResimulateRootMotionSources : 1; uint32 bSimGravityDisabled : 1; uint32 bClientCheckEncroachmentOnNetUpdate : 1; uint32 bServerMoveIgnoreRootMotion : 1; uint32 bWasJumping : 1;
	float JumpKeyHoldTime; float JumpForceTimeRemaining; float ProxyJumpForceStartedTime; float JumpMaxHoldTime; int32 JumpMaxCount; int32 JumpCurrentCount;
	FLandedSignature LandedDelegate;
	FMovementModeChangedSignature MovementModeChangedDelegate;
	FCharacterMovementUpdatedSignature OnCharacterMovementUpdated;
	FCharacterReachedApexSignature OnReachedJumpApex;
	virtual void Jump();
	virtual void StopJumping();
	bool CanJump() const;
	virtual void CheckJumpInput(float DeltaTime);
	virtual void ClearJumpInput(float DeltaTime);
	virtual bool IsJumpProvidingForce() const;
	virtual float GetJumpMaxHoldTime() const;
	virtual void ResetJumpState();
	virtual void LaunchCharacter(FVector LaunchVelocity, bool bXYOverride, bool bZOverride);
	virtual void Crouch(bool bClientSimulation = false);
	virtual void UnCrouch(bool bClientSimulation = false);
	virtual bool CanCrouch() const;
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust);
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust);
	virtual void Landed(const FHitResult& Hit);
	virtual void OnJumped_Implementation();
	virtual void NotifyJumpApex();
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0);
	virtual void OnWalkingOffLedge_Implementation(const FVector& PreviousFloorImpactNormal, const FVector& PreviousFloorContactNormal, const FVector& PreviousLocation, float TimeDelta);
	virtual void FellOutOfWorld(const class UDamageType& dmgType) override;
	virtual float PlayAnimMontage(class UAnimMontage* AnimMontage, float InPlayRate = 1.f, FName StartSectionName = NAME_None);
	virtual void StopAnimMontage(class UAnimMontage* AnimMontage = nullptr);
	class UAnimMontage* GetCurrentMontage() const;
	bool IsPlayingRootMotion() const; bool IsPlayingNetworkedRootMotionMontage() const; bool HasAnyRootMotion() const;
	FVector GetBaseTranslationOffset() const; virtual FQuat GetBaseRotationOffset() const; FRotator GetBaseRotationOffsetRotator() const;
	void CacheInitialMeshOffset(FVector MeshRelativeLocation, FRotator MeshRelativeRotation);
	virtual void PostInitializeComponents() override;
	virtual void GetSimpleCollisionCylinder(float& CollisionRadius, float& CollisionHalfHeight) const override;
	virtual FVector GetNavAgentLocation() const override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;
	virtual void ApplyWorldOffset(const FVector& InOffset, bool bWorldShift);
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void Restart() override;
	virtual void PawnClientRestart() override;
	virtual void FaceRotation(FRotator NewControlRotation, float DeltaTime = 0.f) override;
	virtual void SetBase(UPrimitiveComponent* NewBase, const FName BoneName = NAME_None, bool bNotifyActor = true);
	UPrimitiveComponent* GetMovementBase() const;
protected:
	virtual bool CanJumpInternal_Implementation() const;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	FVector BaseTranslationOffset; FQuat BaseRotationOffset;
};

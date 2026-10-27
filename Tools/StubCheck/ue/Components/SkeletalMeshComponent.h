// Макет Components/SkeletalMeshComponent.h (UE 4.27): без GetSkeletalMeshAsset/SetSkeletalMeshAsset (UE5).
#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "Components/SkinnedMeshComponent.h"
class UAnimInstance; class UAnimationAsset; class UAnimSequenceBase;
enum class EKinematicBonesUpdateToPhysics : uint8 { SkipSimulatingBones, SkipAllBones };
enum class EPhysicsTransformUpdateMode : uint8 { SimulationUpatesComponentTransform, ComponentTransformIsKinematic };
class USkeletalMeshComponent : public USkinnedMeshComponent
{
public:
	typedef USkinnedMeshComponent Super; typedef USkeletalMeshComponent ThisClass; static UClass* StaticClass();
	USkeletalMeshComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	TSubclassOf<UAnimInstance> AnimClass;
	UAnimInstance* AnimScriptInstance;
	EPhysicsTransformUpdateMode PhysicsTransformUpdateMode;
	float GlobalAnimRateScale;
	uint8 bEnablePhysicsOnDedicatedServer : 1; uint8 bUpdateJointsFromAnimation : 1; uint8 bDisableClothSimulation : 1; uint8 bDisablePostProcessBlueprint : 1;
	uint8 bAllowAnimCurveEvaluation : 1; uint8 bPauseAnims : 1; uint8 bUseRefPoseOnInitAnim : 1; uint8 bOnlyAllowAutonomousTickPose : 1;
	uint8 bIsAutonomousTickPose : 1; uint8 bForceRefpose : 1; uint8 bBlendPhysics : 1; uint8 bEnablePerPolyCollision : 1; uint8 bPropagateCurvesToSlaves : 1;
	uint8 bUpdateAnimationInEditor : 1; uint8 bLocalSpaceSimulation : 1; uint8 bResetAfterTeleport : 1;
	virtual void SetSkeletalMesh(USkeletalMesh* NewMesh, bool bReinitPose = true) override;
	virtual void SetAnimInstanceClass(UClass* NewClass);
	void SetAnimClass(UClass* NewClass);
	UClass* GetAnimClass();
	UAnimInstance* GetAnimInstance() const;
	UAnimInstance* GetPostProcessInstance() const;
	UAnimInstance* GetLinkedAnimGraphInstanceByTag(FName InTag) const;
	void SetAnimationMode(EAnimationMode::Type InAnimationMode);
	EAnimationMode::Type GetAnimationMode() const;
	void PlayAnimation(UAnimationAsset* NewAnimToPlay, bool bLooping);
	void SetAnimation(UAnimationAsset* NewAnimToPlay);
	void Play(bool bLooping); void Stop(); bool IsPlaying() const;
	void SetPosition(float InPos, bool bFireNotifies = true); float GetPosition() const;
	void SetPlayRate(float Rate); float GetPlayRate() const;
	void SetUpdateAnimationInEditor(const bool NewUpdateState);
	void SetDisablePostProcessBlueprint(bool bInDisablePostProcess);
	void SetAllBodiesSimulatePhysics(bool bNewSimulate);
	void SetAllBodiesBelowSimulatePhysics(const FName& InBoneName, bool bNewSimulate, bool bIncludeSelf = true);
	void SetAllBodiesPhysicsBlendWeight(float PhysicsBlendWeight, bool bSkipCustomPhysicsType = false);
	void SetAllBodiesBelowPhysicsBlendWeight(const FName& InBoneName, float PhysicsBlendWeight, bool bSkipCustomPhysicsType = false, bool bIncludeSelf = true);
	void ResetAllBodiesSimulatePhysics();
	void SetPhysicsBlendWeight(float PhysicsBlendWeight);
	void SetEnableBodyGravity(bool bEnableGravity, FName BoneName);
	bool IsBodyGravityEnabled(FName BoneName);
	void SetEnablePhysicsBlending(bool bNewBlendPhysics);
	void AddImpulseToAllBodiesBelow(FVector Impulse, FName BoneName = NAME_None, bool bVelChange = false, bool bIncludeSelf = true);
	void AddForceToAllBodiesBelow(FVector Force, FName BoneName = NAME_None, bool bAccelChange = false, bool bIncludeSelf = true);
	void BreakConstraint(FVector Impulse, FVector HitLocation, FName InBoneName);
	void SetMorphTarget(FName MorphTargetName, float Value, bool bRemoveZeroWeight = true);
	void ClearMorphTargets();
	float GetMorphTarget(FName MorphTargetName) const;
	float GetBoneMass(FName BoneName = NAME_None, bool bScaleMass = true) const;
	FVector GetSkeletalCenterOfMass() const;
	virtual void SetSimulatePhysics(bool bEnabled) override;
	virtual void SetAllPhysicsLinearVelocity(FVector NewVel, bool bAddToCurrent = false) override;
	virtual void WakeAllRigidBodies() override;
	virtual void PutAllRigidBodiesToSleep() override;
	virtual FBodyInstance* GetBodyInstance(FName BoneName = NAME_None, bool bGetWelded = true) const override;
	virtual void TickAnimation(float DeltaTime, bool bNeedsValidRootMotion);
	void ResetAnimInstanceDynamics(ETeleportType InTeleportType = ETeleportType::ResetPhysics);
	void InitAnim(bool bForceReinit);
	bool IsPlayingRootMotion() const;
	bool IsPlayingNetworkedRootMotionMontage() const;
	float GetTeleportRotationThreshold() const; void SetTeleportRotationThreshold(float Threshold);
	float GetTeleportDistanceThreshold() const; void SetTeleportDistanceThreshold(float Threshold);
	void SnapshotPose(struct FPoseSnapshot& Snapshot);
	void OverrideAnimationData(UAnimationAsset* InAnimToPlay, bool bIsLooping = true, bool bIsPlaying = true, float Position = 0.f, float PlayRate = 1.f);
	void SetConstraintProfileForAll(FName ProfileName, bool bDefaultIfNotFound = false);
	EKinematicBonesUpdateToPhysics KinematicBonesUpdateType;
};


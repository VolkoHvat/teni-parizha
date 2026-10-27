// Макет Animation/AnimInstance.h (UE 4.27): NativeThreadSafeUpdateAnimation отсутствует (UE5).
#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Animation/AnimationAsset.h"
class APawn; class AActor; class USkeletalMeshComponent; class UAnimMontage; class UAnimSequenceBase; struct FAnimInstanceProxy; class USkeleton;
enum class EMontagePlayReturnType : uint8 { MontageLength, Duration };
DECLARE_DELEGATE_TwoParams(FOnMontageEnded, class UAnimMontage*, bool);
DECLARE_DELEGATE_TwoParams(FOnMontageBlendingOutStarted, class UAnimMontage*, bool);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMontageBlendingOutStartedMCDelegate, UAnimMontage*, Montage, bool, bInterrupted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMontageEndedMCDelegate, UAnimMontage*, Montage, bool, bInterrupted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMontageStartedMCDelegate, UAnimMontage*, Montage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAllMontageInstancesEndedMCDelegate);
class UAnimInstance : public UObject
{
public:
	typedef UObject Super; typedef UAnimInstance ThisClass; static UClass* StaticClass();
	UAnimInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	USkeleton* CurrentSkeleton;
	TEnumAsByte<ERootMotionMode::Type> RootMotionMode;
	uint8 bUseMultiThreadedAnimationUpdate : 1; uint8 bUsingCopyPoseFromMesh : 1; uint8 bQueueMontageEvents : 1;
	FOnMontageBlendingOutStartedMCDelegate OnMontageBlendingOut; FOnMontageStartedMCDelegate OnMontageStarted; FOnMontageEndedMCDelegate OnMontageEnded; FOnAllMontageInstancesEndedMCDelegate OnAllMontageInstancesEnded;
	virtual void NativeInitializeAnimation();
	virtual void NativeUpdateAnimation(float DeltaSeconds);
	virtual void NativePostEvaluateAnimation();
	virtual void NativeUninitializeAnimation();
	virtual void NativeBeginPlay();
	APawn* TryGetPawnOwner() const;
	AActor* GetOwningActor() const;
	USkeletalMeshComponent* GetOwningComponent() const;
	USkeletalMeshComponent* GetSkelMeshComponent() const;
	virtual UWorld* GetWorld() const override;
	float Montage_Play(UAnimMontage* MontageToPlay, float InPlayRate = 1.f, EMontagePlayReturnType ReturnValueType = EMontagePlayReturnType::MontageLength, float InTimeToStartMontageAt = 0.f, bool bStopAllMontages = true);
	void Montage_Stop(float InBlendOutTime, const UAnimMontage* Montage = nullptr);
	void Montage_StopGroupByName(float InBlendOutTime, FName GroupName);
	void Montage_Pause(const UAnimMontage* Montage = nullptr); void Montage_Resume(const UAnimMontage* Montage);
	void Montage_JumpToSection(FName SectionName, const UAnimMontage* Montage = nullptr);
	void Montage_JumpToSectionsEnd(FName SectionName, const UAnimMontage* Montage = nullptr);
	void Montage_SetNextSection(FName SectionNameToChange, FName NextSection, const UAnimMontage* Montage = nullptr);
	void Montage_SetPlayRate(const UAnimMontage* Montage, float NewPlayRate = 1.f);
	bool Montage_IsActive(const UAnimMontage* Montage) const; bool Montage_IsPlaying(const UAnimMontage* Montage) const;
	FName Montage_GetCurrentSection(const UAnimMontage* Montage = nullptr) const;
	float Montage_GetPosition(const UAnimMontage* Montage) const; void Montage_SetPosition(const UAnimMontage* Montage, float NewPosition);
	float Montage_GetPlayRate(const UAnimMontage* Montage) const; bool Montage_GetIsStopped(const UAnimMontage* Montage) const; float Montage_GetBlendTime(const UAnimMontage* Montage) const;
	void Montage_SetEndDelegate(FOnMontageEnded& InOnMontageEnded, UAnimMontage* Montage = nullptr);
	void Montage_SetBlendingOutDelegate(FOnMontageBlendingOutStarted& InOnMontageBlendingOut, UAnimMontage* Montage = nullptr);
	bool IsAnyMontagePlaying() const; UAnimMontage* GetCurrentActiveMontage() const;
	void StopAllMontages(float BlendOut);
	UAnimMontage* PlaySlotAnimationAsDynamicMontage(UAnimSequenceBase* Asset, FName SlotNodeName, float BlendInTime = 0.25f, float BlendOutTime = 0.25f, float InPlayRate = 1.f, int32 LoopCount = 1, float BlendOutTriggerTime = -1.f, float InTimeToStartMontageAt = 0.f);
	void StopSlotAnimation(float InBlendOutTime = 0.25f, FName SlotNodeName = NAME_None);
	bool IsPlayingSlotAnimation(const UAnimSequenceBase* Asset, FName SlotNodeName) const;
	float GetCurveValue(FName CurveName) const;
	bool GetCurveValue(FName CurveName, float& OutValue) const;
	void SetRootMotionMode(TEnumAsByte<ERootMotionMode::Type> Value);
	float CalculateDirection(const FVector& Velocity, const FRotator& BaseRotation) const;
	void SetMorphTarget(FName MorphTargetName, float Value);
	void ClearMorphTargets();
	void SavePoseSnapshot(FName SnapshotName);
	template<typename T> T& GetProxyOnGameThread();
	template<typename T> const T& GetProxyOnGameThread() const;
	template<typename T> T& GetProxyOnAnyThread();
	void InitializeAnimation(bool bInDeferRootNodeInitialization = false);
	void UpdateAnimation(float DeltaSeconds, bool bNeedsValidRootMotion, int32 UpdateMode = 0);
	virtual void PreUpdateAnimation(float DeltaSeconds);
protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy();
	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy);
	virtual bool NativeEvaluateAnimation(struct FPoseContext& Output);
};

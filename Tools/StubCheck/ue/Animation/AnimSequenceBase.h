#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "Animation/AnimationAsset.h"
struct FAnimNotifyEvent { float TriggerTimeOffset; FName NotifyName; float GetTime() const; float GetDuration() const; };
class UAnimSequenceBase : public UAnimationAsset
{
public:
	typedef UAnimationAsset Super; typedef UAnimSequenceBase ThisClass; static UClass* StaticClass();
	UAnimSequenceBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	TArray<FAnimNotifyEvent> Notifies; float SequenceLength; float RateScale; bool bLoop;
	virtual float GetPlayLength() const override;
	virtual int32 GetNumberOfFrames() const;
	virtual bool IsValidAdditive() const;
	virtual bool HasRootMotion() const;
};

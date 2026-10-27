#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "Animation/AnimSequenceBase.h"
struct FAlphaBlend { FAlphaBlend(float NewBlendTime = 0.2f); float GetBlendTime() const; void SetBlendTime(float InBlendTime); float GetAlpha() const; };
struct FAnimSegment { UAnimSequenceBase* AnimReference; float StartPos; float AnimStartTime; float AnimEndTime; float AnimPlayRate; int32 LoopingCount; };
struct FAnimTrack { TArray<FAnimSegment> AnimSegments; float GetLength() const; };
struct FSlotAnimationTrack { FName SlotName; FAnimTrack AnimTrack; };
struct FCompositeSection { FName SectionName; FName NextSectionName; float GetTime() const; };
class UAnimMontage : public UAnimSequenceBase
{
public:
	typedef UAnimSequenceBase Super; typedef UAnimMontage ThisClass; static UClass* StaticClass();
	UAnimMontage(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	FAlphaBlend BlendIn; FAlphaBlend BlendOut; float BlendOutTriggerTime; bool bEnableAutoBlendOut; FName SyncGroup;
	TArray<FCompositeSection> CompositeSections; TArray<FSlotAnimationTrack> SlotAnimTracks;
	int32 GetSectionIndex(FName InSectionName) const; FName GetSectionName(int32 SectionIndex) const; int32 GetNumSections() const; bool IsValidSectionName(FName InSectionName) const;
	float GetSectionLength(int32 SectionIndex) const; void GetSectionStartAndEndTime(int32 SectionIndex, float& OutStartTime, float& OutEndTime) const;
	float GetDefaultBlendOutTime() const;
	static UAnimMontage* CreateSlotAnimationAsDynamicMontage(UAnimSequenceBase* Asset, FName SlotNodeName, float BlendInTime = 0.25f, float BlendOutTime = 0.25f, float InPlayRate = 1.f, int32 LoopCount = 1, float BlendOutTriggerTime = -1.f, float InTimeToStartMontageAt = 0.f);
};

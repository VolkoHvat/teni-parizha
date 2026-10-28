#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "UObject/Object.h"
class USkeleton;
namespace ERootMotionMode { enum Type { NoRootMotionExtraction, IgnoreRootMotion, RootMotionFromEverything, RootMotionFromMontagesOnly }; }
// В 4.27 объявлен в AnimationAsset.h.
struct FAnimExtractContext { float CurrentTime; bool bExtractRootMotion; FAnimExtractContext(float InCurrentTime = 0.f, bool InbExtractRootMotion = false); };
class UAnimationAsset : public UObject
{
public:
	typedef UObject Super; typedef UAnimationAsset ThisClass; static UClass* StaticClass();
	UAnimationAsset(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	USkeleton* GetSkeleton() const; void SetSkeleton(USkeleton* NewSkeleton);
	virtual float GetPlayLength() const;
};

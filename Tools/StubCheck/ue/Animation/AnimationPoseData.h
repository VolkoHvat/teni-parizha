// Макет Animation/AnimationPoseData.h (UE 4.26–4.27).
#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
struct FPoseContext; struct FCompactPose; struct FBlendedCurve; struct FStackCustomAttributes;
struct FAnimationPoseData
{
	FAnimationPoseData(FPoseContext& InPoseContext);
	FAnimationPoseData(FCompactPose& InPose, FBlendedCurve& InCurve, FStackCustomAttributes& InAttributes);
	const FCompactPose& GetPose() const; FCompactPose& GetPose();
	const FBlendedCurve& GetCurve() const; FBlendedCurve& GetCurve();
	const FStackCustomAttributes& GetAttributes() const; FStackCustomAttributes& GetAttributes();
};

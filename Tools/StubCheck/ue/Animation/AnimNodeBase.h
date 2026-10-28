// Макет Animation/AnimNodeBase.h (UE 4.27): контексты обновления и вычисления позы.
#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "BonePose.h"
#include "Animation/AnimCurveTypes.h"
#include "Animation/CustomAttributesRuntime.h"
struct FAnimInstanceProxy;
struct FAnimationBaseContext
{
	FAnimInstanceProxy* AnimInstanceProxy;
	FAnimInstanceProxy* GetAnimInstanceObject() const;
};
struct FAnimationUpdateContext : public FAnimationBaseContext
{
	float GetDeltaTime() const; float GetFinalBlendWeight() const;
};
struct FPoseContext : public FAnimationBaseContext
{
	FCompactPose Pose;
	FBlendedCurve Curve;
	FStackCustomAttributes CustomAttributes;
	FPoseContext(FAnimInstanceProxy* InAnimInstanceProxy, bool bInExpectsAdditivePose = false);
	FPoseContext(const FPoseContext& SourceContext, bool bInOverrideExpectsAdditivePose = false);
	void ResetToRefPose();
	void ResetToAdditiveIdentity();
	bool ContainsNaN() const;
	bool IsNormalized() const;
	bool ExpectsAdditivePose() const;
};
struct FAnimNode_Base
{
	virtual ~FAnimNode_Base() {}
};

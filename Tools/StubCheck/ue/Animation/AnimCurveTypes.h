// Макет Animation/AnimCurveTypes.h (UE 4.27): FBlendedCurve.
#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
struct FBoneContainer;
struct FBlendedCurve
{
	FBlendedCurve();
	void InitFrom(const FBoneContainer& RequiredBones);
	void InitFrom(const FBlendedCurve& InCurveToInitFrom);
	void Empty();
	bool IsValid() const;
	void Lerp(const FBlendedCurve& A, const FBlendedCurve& B, float Alpha);
	void LerpTo(const FBlendedCurve& Other, float Alpha);
	void CopyFrom(const FBlendedCurve& CurveToCopyFrom);
	void Override(const FBlendedCurve& CurveToOverrideFrom, float WeightToOverrideFrom);
};

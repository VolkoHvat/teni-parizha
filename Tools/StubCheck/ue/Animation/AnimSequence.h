#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "Animation/AnimSequenceBase.h"
struct FCompactPose; struct FBlendedCurve; struct FAnimExtractContext; struct FAnimationPoseData;
enum EAdditiveAnimationType { AAT_None, AAT_LocalSpaceBase, AAT_RotationOffsetMeshSpace, AAT_MAX };
enum class EAnimInterpolationType : uint8 { Linear, Step };
class UAnimSequence : public UAnimSequenceBase
{
public:
	typedef UAnimSequenceBase Super; typedef UAnimSequence ThisClass; static UClass* StaticClass();
	UAnimSequence(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	int32 NumFrames; EAnimInterpolationType Interpolation; FName RetargetSource;
	TEnumAsByte<enum EAdditiveAnimationType> AdditiveAnimType;
	bool bEnableRootMotion; bool bForceRootLock; bool bUseNormalizedRootMotionScale;
	virtual int32 GetNumberOfFrames() const override;
	int32 GetRawNumberOfFrames() const;
	void GetBoneTransform(FTransform& OutAtom, int32 TrackIndex, float Time, bool bUseRawData) const;
	void GetAnimationPose(FCompactPose& OutPose, FBlendedCurve& OutCurve, const FAnimExtractContext& ExtractionContext) const;
	void GetAnimationPose(FAnimationPoseData& OutAnimationPoseData, const FAnimExtractContext& ExtractionContext) const;
	void GetBonePose(FCompactPose& OutPose, FBlendedCurve& OutCurve, const FAnimExtractContext& ExtractionContext, bool bForceUseRawData = false) const;
	FTransform ExtractRootMotion(float StartTime, float DeltaTime, bool bAllowLooping) const;
	FTransform ExtractRootTrackTransform(float Pos, const struct FBoneContainer* RequiredBones) const;
	const TArray<FName>& GetAnimationTrackNames() const;
	float GetTimeAtFrame(const int32 Frame) const; int32 GetFrameAtTime(const float Time) const;
};

// Макет Animation/AnimInstanceProxy.h (UE 4.27): прокси анимации, работает в рабочем потоке.
// Защищённые виртуальные методы — как в 4.27 (Evaluate_WithRoot/UpdateAnimationNode_WithRoot не используются проектом).
#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "BoneContainer.h"
#include "Animation/AnimNodeBase.h"
class UAnimInstance; class USkeleton; class USkeletalMeshComponent; class UObject;
struct FAnimInstanceProxy
{
public:
	TP_GENERATED_USTRUCT(FAnimInstanceProxy)
	FAnimInstanceProxy();
	FAnimInstanceProxy(UAnimInstance* Instance);
	virtual ~FAnimInstanceProxy();
	UObject* GetAnimInstanceObject(); const UObject* GetAnimInstanceObject() const;
	FBoneContainer& GetRequiredBones(); const FBoneContainer& GetRequiredBones() const;
	USkeleton* GetSkeleton();
	USkeletalMeshComponent* GetSkelMeshComponent() const;
	float GetDeltaSeconds() const;
	const FTransform& GetComponentTransform() const;
	const FTransform& GetActorTransform() const;
	bool IsSkeletonCompatible(USkeleton const* InSkeleton) const;
protected:
	virtual void Initialize(UAnimInstance* InAnimInstance);
	virtual void Uninitialize(UAnimInstance* InAnimInstance);
	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds);
	virtual void Update(float DeltaSeconds);
	virtual void PostUpdate(UAnimInstance* InAnimInstance) const;
	virtual void PreEvaluateAnimation(UAnimInstance* InAnimInstance);
	virtual bool Evaluate(FPoseContext& Output);
	virtual void CacheBones();
	virtual void InitializeObjects(UAnimInstance* InAnimInstance);
	virtual void ClearObjects();
	virtual void PostEvaluate(UAnimInstance* InAnimInstance);
};

// Тени Парижа — анимация без AnimBP: прокси со своей Evaluate (клипы манекена или процедурная походка).
#include "TPAnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimationAsset.h"
#include "Animation/AnimNodeBase.h"
#include "Animation/AnimationPoseData.h"
#include "Animation/CustomAttributesRuntime.h"
#include "Animation/Skeleton.h"
#include "BonePose.h"
#include "BoneContainer.h"
#include "Components/SkeletalMeshComponent.h"

using namespace TPAnim;

namespace
{
	// Клипы пакета Third Person UE4 (порядок — как в TPAnim::ELocoClip).
	const TCHAR* const GDefaultClipPaths[C_Count] = {
		TEXT("/Game/Mannequin/Animations/ThirdPersonIdle.ThirdPersonIdle"),
		TEXT("/Game/Mannequin/Animations/ThirdPersonWalk.ThirdPersonWalk"),
		TEXT("/Game/Mannequin/Animations/ThirdPersonRun.ThirdPersonRun"),
		TEXT("/Game/Mannequin/Animations/ThirdPersonJump_Start.ThirdPersonJump_Start"),
		TEXT("/Game/Mannequin/Animations/ThirdPersonJump_Loop.ThirdPersonJump_Loop"),
		TEXT("/Game/Mannequin/Animations/ThirdPersonJump_End.ThirdPersonJump_End") };
}

// ---------------- экземпляр (игровой поток) ----------------

FAnimInstanceProxy* UTPAnimInstance::CreateAnimInstanceProxy()
{
	return new FTPAnimInstanceProxy(this);
}

UAnimSequence* UTPAnimInstance::GetClip(int32 Clip) const
{
	switch (Clip)
	{
	case C_Idle: return IdleAnim;
	case C_Walk: return WalkAnim;
	case C_Run: return RunAnim;
	case C_JumpStart: return JumpStartAnim;
	case C_JumpLoop: return JumpLoopAnim;
	case C_JumpEnd: return JumpEndAnim;
	default: return nullptr;
	}
}

void UTPAnimInstance::LoadDefaultClips()
{
	UAnimSequence** Slots[C_Count] = { &IdleAnim, &WalkAnim, &RunAnim, &JumpStartAnim, &JumpLoopAnim, &JumpEndAnim };
	for (int32 i = 0; i < C_Count; ++i)
	{
		if (!*Slots[i])
		{
			*Slots[i] = LoadObject<UAnimSequence>(nullptr, GDefaultClipPaths[i], nullptr, LOAD_NoWarn | LOAD_Quiet);
		}
	}
}

void UTPAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	if (!bForceProcedural)
	{
		LoadDefaultClips();
	}
}

void UTPAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);   // Speed, bInAir, State… из движения и паркура
	GetProxyOnGameThread<FTPAnimInstanceProxy>().GatherInput(this);
}

// ---------------- прокси ----------------

void FTPAnimInstanceProxy::Initialize(UAnimInstance* InAnimInstance)
{
	Super::Initialize(InAnimInstance);
	for (int32 b = 0; b < B_Count; ++b)
	{
		BoneFNames[b] = FName(BoneName(b));
	}
	Loco.Reset();
	Clips.Reset();
	Clips.SetNumZeroed(C_Count);
	bClipsReady = false;
	bProcedural = true;
	bRigBuilt = false;
	bRigCompatible = false;
}

void FTPAnimInstanceProxy::GatherInput(const UTPAnimInstance* Inst)
{
	if (!Inst)
	{
		return;
	}
	Input.Speed = Inst->Speed;
	Input.VerticalSpeed = Inst->VerticalSpeed;
	// вис и лазание — не полёт (их позы — этап 7); здесь только не даём включиться падению
	Input.bInAir = Inst->bInAir && !Inst->bHanging && !Inst->bClimbing;
	Input.bCrouching = Inst->bCrouching;
	Params.WalkAnimSpeed = FMath::Max(Inst->WalkAnimSpeed, 10.f);
	Params.RunAnimSpeed = FMath::Max(Inst->RunAnimSpeed, Params.WalkAnimSpeed + 10.f);
	Params.MaxPlayRate = FMath::Clamp(Inst->MaxPlayRate, 1.f, 3.f);
	bForceProcedural = Inst->bForceProcedural;

	if (const USkeletalMeshComponent* Mesh = Inst->GetSkelMeshComponent())
	{
		MeshRot = Mesh->GetRelativeRotation().Quaternion();
	}

	// клипы годятся, только если все есть и сняты на скелете этого меша
	Clips.SetNumZeroed(C_Count);
	bool bAll = true;
	for (int32 c = 0; c < C_Count; ++c)
	{
		UAnimSequence* Seq = Inst->GetClip(c);
		if (Seq && Inst->CurrentSkeleton && Seq->GetSkeleton() != Inst->CurrentSkeleton)
		{
			Seq = nullptr;
		}
		Clips[c] = Seq;
		if (Seq)
		{
			Params.ClipLength[c] = Seq->GetPlayLength();
		}
		else
		{
			bAll = false;
		}
	}
	bClipsReady = bAll;
}

void FTPAnimInstanceProxy::Update(float DeltaSeconds)
{
	Super::Update(DeltaSeconds);
	bProcedural = bForceProcedural || !bClipsReady;
	Loco.Update(Params, Input, DeltaSeconds);
	if (bRigBuilt && bRigCompatible)
	{
		Loco.UpdateProcedural(Rig, DeltaSeconds);   // нужен и при клипах — для слоя приседа
	}
}

void FTPAnimInstanceProxy::MapBones(const FCompactPose& Pose, int32* OutIdx) const
{
	const FBoneContainer& Bones = Pose.GetBoneContainer();
	for (int32 b = 0; b < B_Count; ++b)
	{
		const int32 MeshIdx = Bones.GetPoseBoneIndexForBoneName(BoneFNames[b]);
		OutIdx[b] = MeshIdx == INDEX_NONE ? INDEX_NONE : Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(MeshIdx)).GetInt();
	}
}

void FTPAnimInstanceProxy::BuildRig(const FCompactPose& Pose, const int32* Idx)
{
	for (int32 b = 0; b < B_Count; ++b)
	{
		if (Idx[b] == INDEX_NONE)
		{
			return;   // кость отсечена этим LOD — скелет соберём на полном LOD
		}
	}
	FVector CompPos[B_Count];
	FQuat CompRot[B_Count];
	bool bParentsMatch = true;
	for (int32 b = 0; b < B_Count; ++b)
	{
		const FCompactPoseBoneIndex CI(Idx[b]);
		FTransform CS = Pose.GetRefPose(CI);
		for (FCompactPoseBoneIndex P = Pose.GetParentBoneIndex(CI); P.IsValid(); P = Pose.GetParentBoneIndex(P))
		{
			CS = CS * Pose.GetRefPose(P);
		}
		CompPos[b] = CS.GetTranslation();
		CompRot[b] = CS.GetRotation();
		const int32 MyParent = BoneParent(b);
		if (MyParent >= 0 && Pose.GetParentBoneIndex(CI).GetInt() != Idx[MyParent])
		{
			bParentsMatch = false;   // не манекен UE4: между костями есть лишние — процедурная поза неприменима
		}
	}
	Rig = MakeRigFromMesh(CompPos, CompRot, MeshRot);
	RigMeshRot = MeshRot;
	bRigBuilt = true;
	bRigCompatible = bParentsMatch && Rig.LegLen > 10.f && Rig.HipH > 10.f;
}

void FTPAnimInstanceProxy::ApplyProcedural(FCompactPose& Pose, const int32* Idx, const FTPPoseCS& P, float Weight) const
{
	for (int32 b = B_Pelvis; b < B_Count; ++b)   // корень не трогаем: смещение и поворот актёра делает движение
	{
		if (Idx[b] == INDEX_NONE)
		{
			continue;
		}
		const FCompactPoseBoneIndex CI(Idx[b]);
		FTransform L = P.LocalTransform(Rig, b, RigMeshRot);
		L.SetScale3D(Pose[CI].GetScale3D());
		if (Weight >= 0.999f)
		{
			Pose[CI] = L;
		}
		else
		{
			const FTransform A = Pose[CI];
			Pose[CI].Blend(A, L, Weight);
		}
	}
}

bool FTPAnimInstanceProxy::SampleClips(FPoseContext& Output, const FTPLocoOutput& W) const
{
	int32 NumUsed = 0;
	for (int32 c = 0; c < C_Count; ++c)
	{
		if (W.Weight[c] > 1e-3f && Clips.IsValidIndex(c) && Clips[c])
		{
			++NumUsed;
		}
	}
	if (NumUsed == 0)
	{
		return false;
	}

	bool bFirst = true;
	float Acc = 0.f;
	FPoseContext Tmp(Output);   // поза-черновик с теми же костями (стековая память кадра анимации)
	for (int32 c = 0; c < C_Count; ++c)
	{
		const float Wc = W.Weight[c];
		const UAnimSequence* Seq = Clips.IsValidIndex(c) ? Clips[c] : nullptr;
		if (Wc <= 1e-3f || !Seq)
		{
			continue;
		}
		if (bFirst)
		{
			FAnimationPoseData OutData(Output);
			Seq->GetAnimationPose(OutData, FAnimExtractContext(W.Time[c], false));
			Acc = Wc;
			bFirst = false;
			continue;
		}
		FAnimationPoseData TmpData(Tmp);
		Seq->GetAnimationPose(TmpData, FAnimExtractContext(W.Time[c], false));
		Acc += Wc;
		const float Alpha = Wc / Acc;   // последовательное усреднение: итог — взвешенная смесь
		for (const FCompactPoseBoneIndex BI : Output.Pose.ForEachBoneIndex())
		{
			const FTransform A = Output.Pose[BI];
			Output.Pose[BI].Blend(A, Tmp.Pose[BI], Alpha);
		}
	}
	Output.Pose.NormalizeRotations();
	return true;
}

bool FTPAnimInstanceProxy::Evaluate(FPoseContext& Output)
{
	FCompactPose& Pose = Output.Pose;
	if (!Pose.IsValid() || Pose.GetNumBones() <= 0)
	{
		return false;
	}
	int32 Idx[B_Count];
	MapBones(Pose, Idx);
	if (!bRigBuilt || !RigMeshRot.Equals(MeshRot, 1e-3f))
	{
		BuildRig(Pose, Idx);
	}

	FTPLocoOutput W;
	Loco.GetOutput(Params, W);
	if (!bProcedural)
	{
		if (!SampleClips(Output, W))
		{
			Output.ResetToRefPose();
		}
		if (bRigBuilt && bRigCompatible && W.CrouchWeight > 0.01f)
		{
			FTPPoseCS Low;
			Loco.SolveGroundGroup(Rig, true, Low);
			ApplyProcedural(Pose, Idx, Low, W.CrouchWeight);
		}
		return true;
	}

	if (!bRigBuilt || !bRigCompatible)
	{
		return false;   // ни клипов, ни подходящего скелета — поза покоя (её даёт движок)
	}
	Output.ResetToRefPose();
	FTPPoseCS P;
	Loco.SolveProcedural(Rig, P);
	ApplyProcedural(Pose, Idx, P, 1.f);
	return true;
}

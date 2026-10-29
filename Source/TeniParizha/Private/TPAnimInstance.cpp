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
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "Engine/EngineTypes.h"
#include "CollisionQueryParams.h"
#include "TPTraversalComponent.h"
#include "TPCombatComponent.h"

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
	for (int32 S = 0; S < 2; ++S)
	{
		FootOffsetS[S] = 0.f;
		FootNormalS[S] = FVector(0.f, 0.f, 1.f);
	}
	Combat = nullptr;
	bCombatSearched = false;
	if (!bForceProcedural)
	{
		LoadDefaultClips();
	}
}

UAnimSequence* UTPAnimInstance::GetActionClip(int32 ActionIndex) const
{
	if (OverrideSequences.Num() == 0 || ActionIndex <= A_None || ActionIndex >= A_Count)
	{
		return nullptr;
	}
	return OverrideSequences.FindRef(FName(ActionName(ActionIndex)));
}

void UTPAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);   // Speed, bInAir, State… из движения и паркура
	UpdateAction(DeltaSeconds);
	UpdateFootIK(DeltaSeconds);
	UpdateLookAt(DeltaSeconds);
	GetProxyOnGameThread<FTPAnimInstanceProxy>().GatherInput(this);
}

bool UTPAnimInstance::UseIK() const
{
	return OwnerCharacter && (!bIKOnlyForPlayer || OwnerCharacter->IsPlayerControlled());
}

FVector UTPAnimInstance::ToCharacterSpace(const FVector& World) const
{
	const USkeletalMeshComponent* Mesh = GetSkelMeshComponent();
	if (!Mesh)
	{
		return FVector::ZeroVector;
	}
	const FVector Local = Mesh->GetComponentTransform().InverseTransformPosition(World);
	return Mesh->GetRelativeRotation().Quaternion().RotateVector(Local);
}

FVector UTPAnimInstance::ToCharacterDir(const FVector& WorldDir) const
{
	const USkeletalMeshComponent* Mesh = GetSkelMeshComponent();
	if (!Mesh)
	{
		return WorldDir;
	}
	const FVector Local = Mesh->GetComponentTransform().InverseTransformVectorNoScale(WorldDir);
	return Mesh->GetRelativeRotation().Quaternion().RotateVector(Local);
}

void UTPAnimInstance::UpdateAction(float DeltaSeconds)
{
	const float Dt = FMath::Clamp(DeltaSeconds, 0.f, 0.1f);
	int32 A = A_None;
	float U = 0.f;
	FrameCtx.Reset();
	if (State != PrevState)
	{
		PrevState = State;
		StateClock = 0.f;
	}
	else
	{
		StateClock += Dt;
	}

	// ---- паркур ----
	if (Traversal)
	{
		const float Progress = Traversal->GetActionProgress();
		switch (State)
		{
		case ETPTraversal::Hang: A = A_Hang; break;
		case ETPTraversal::Climb: A = A_Climb; break;
		case ETPTraversal::ClimbUp: A = Traversal->IsActionFromHang() ? A_ClimbUp : A_Mantle; U = Progress; break;
		case ETPTraversal::Vault: A = A_Vault; U = Progress; break;
		case ETPTraversal::WallRun: A = A_WallRun; break;
		case ETPTraversal::Roll: A = A_Roll; U = Progress; break;
		case ETPTraversal::Turn: A = A_Turn; U = Progress; break;
		case ETPTraversal::Drop: A = A_Drop; U = Progress; break;
		case ETPTraversal::Hop: A = A_Hop; U = Progress; break;
		case ETPTraversal::Land:
			// мягкое приземление ведёт локомоция (Jump_End / процедурное), жёсткое — своя поза
			if (Traversal->GetLastFallHeight() > 220.f) { A = A_LandHard; U = Progress; }
			break;
		default: break;
		}
		if (A != A_None)
		{
			// руки на кромке/стене из паркура (для Hop — следующий уступ), кромка уступа или препятствия
			FVector L, R;
			Traversal->GetHandIK(L, R);
			const bool bHands = !L.IsNearlyZero() && !R.IsNearlyZero();
			if (bHands)
			{
				FrameCtx.Hand[0] = ToCharacterSpace(L);
				FrameCtx.Hand[1] = ToCharacterSpace(R);
			}
			const FTPLedge& Ledge = Traversal->GetLedge();
			if (A == A_Hop && bHands)
			{
				FrameCtx.Ledge = (FrameCtx.Hand[0] + FrameCtx.Hand[1]) * 0.5f - FVector(4.f, 0.f, 2.f);
			}
			else if (Ledge.bValid)
			{
				FrameCtx.Ledge = ToCharacterSpace(Ledge.Edge);
			}
			FrameCtx.WallX = A == A_WallRun ? 45.f : FrameCtx.Ledge.X;
			FrameCtx.Shimmy = ShimmyDir;
			FrameCtx.ClimbRate = ClimbSpeed;
		}
	}

	// ---- бой (если паркур не занят) ----
	if (!Combat && !bCombatSearched && OwnerCharacter)
	{
		Combat = OwnerCharacter->FindComponentByClass<UTPCombatComponent>();
		bCombatSearched = true;
	}
	const float DodgeU = Combat ? DodgeTrack.Fraction(Combat->DodgeTime) : 0.f;
	const float ReactU = Combat ? ReactTrack.Fraction(Combat->ReactTime) : 0.f;
	const float StaggerU = Combat ? StaggerTrack.Fraction(Combat->StaggerTime) : 0.f;
	const float SwingU = Combat ? SwingTrack.Fraction(Combat->Swing) : 0.f;
	const float WindU = Combat ? WindUpTrack.Fraction(Combat->WindUp) : 0.f;
	if (A == A_None && Combat && !Combat->IsDefeated() && !bInAir)
	{
		if (Combat->StaggerTime > 0.f) { A = A_Stagger; U = StaggerU; }
		else if (Combat->DodgeTime > 0.f)
		{
			A = A_Dodge; U = DodgeU;
			const FVector V = OwnerCharacter ? OwnerCharacter->GetVelocity() : FVector::ZeroVector;
			const FVector D = ToCharacterDir(FVector(V.X, V.Y, 0.f)).GetSafeNormal2D();
			FrameCtx.DodgeDir = D.IsNearlyZero() ? FVector(-1.f, 0.f, 0.f) : D;
		}
		else if (Combat->AttackTime > 0.f)
		{
			// контратака — удар длительностью 0.5 с (TryCounter), серия — 0.38 с, третий удар — 0.55 с
			const bool bCounter = FMath::Abs(Combat->AttackDuration - 0.5f) < 0.01f;
			A = bCounter ? A_Counter : A_Attack1 + FMath::Clamp(Combat->GetComboIndex(), 0, 2);
			U = 1.f - Combat->AttackTime / FMath::Max(Combat->AttackDuration, 0.05f);
		}
		else if (Combat->Swing > 0.f) { A = A_Strike; U = SwingU; }
		else if (Combat->WindUp > 0.f) { A = A_WindUp; U = WindU; }
		else if (Combat->ReactTime > 0.f) { A = A_HitReact; U = ReactU; }
		else if (Combat->BlockTime > 0.f) { A = A_Block; }
		else if (Combat->CombatWeight > 0.3f) { A = A_Guard; }
	}

	// фаза циклов: не сбрасывается при повторном входе, скорость — по движению
	if (ActionLoops(A))
	{
		float Rate = 1.f;
		switch (A)
		{
		case A_Hang: Rate = FMath::Abs(ShimmyDir) > 0.1f ? 1.4f : 1.f / 1.6f; break;
		case A_Climb: Rate = 0.9f * ClimbSpeed; break;
		case A_WallRun: Rate = 2.4f; break;
		case A_Guard: Rate = 1.f / 3.f; break;
		case A_Block: Rate = 1.f / 1.5f; break;
		default: break;
		}
		LoopPhase = Detail::Frac(LoopPhase + Dt * Rate);
		U = LoopPhase;
	}
	LoopAction = A;
	FrameAction = A;
	FrameActionU = U;
	// наклон в повороте — только на земле
	FrameLean = (State == ETPTraversal::Ground && !bInAir) ? FMath::DegreesToRadians(LeanAngle) * LeanScale : 0.f;
}

void UTPAnimInstance::UpdateFootIK(float DeltaSeconds)
{
	const float Dt = FMath::Clamp(DeltaSeconds, 0.f, 0.1f);
	USkeletalMeshComponent* Mesh = GetSkelMeshComponent();
	UWorld* World = GetWorld();
	const bool bGround = OwnerCharacter && !bInAir && (State == ETPTraversal::Ground || State == ETPTraversal::Land)
		&& FrameAction != A_Dodge && FrameAction != A_Stagger && FrameAction != A_Counter && FrameAction != A_LandHard;
	const bool bOn = bFootIK && UseIK() && bGround && Mesh && World;
	static const FName FootNames[2] = { FName(TEXT("foot_l")), FName(TEXT("foot_r")) };
	float Target[2] = { 0.f, 0.f };
	FVector Normal[2] = { FVector(0.f, 0.f, 1.f), FVector(0.f, 0.f, 1.f) };
	if (bOn)
	{
		const float FloorZ = Mesh->GetComponentLocation().Z;   // корень меша — у ног капсулы
		FCollisionQueryParams Q(SCENE_QUERY_STAT(TPFootIK), false, OwnerCharacter);
		for (int32 S = 0; S < 2; ++S)
		{
			if (Mesh->GetBoneIndex(FootNames[S]) == INDEX_NONE)
			{
				continue;
			}
			const FVector Foot = Mesh->GetSocketLocation(FootNames[S]);
			const FVector Start(Foot.X, Foot.Y, FloorZ + 50.f), End(Foot.X, Foot.Y, FloorZ - 75.f);
			FHitResult Hit;
			if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Q) && Hit.ImpactNormal.Z > 0.6f)
			{
				Target[S] = FMath::Clamp(Hit.ImpactPoint.Z - FloorZ, -45.f, 45.f);
				Normal[S] = Hit.ImpactNormal;
			}
		}
	}
	FootIKWeightS = FMath::FInterpTo(FootIKWeightS, bOn ? 1.f : 0.f, Dt, 8.f);
	for (int32 S = 0; S < 2; ++S)
	{
		FootOffsetS[S] = FMath::FInterpTo(FootOffsetS[S], Target[S], Dt, 14.f);
		FootNormalS[S] = FMath::VInterpTo(FootNormalS[S], Normal[S], Dt, 10.f).GetSafeNormal();
		if (FootNormalS[S].IsNearlyZero())
		{
			FootNormalS[S] = FVector(0.f, 0.f, 1.f);
		}
		FrameFootIK.Offset[S] = FootOffsetS[S];
		FrameFootIK.Normal[S] = ToCharacterDir(FootNormalS[S]);
	}
	FrameFootIK.Weight = FootIKWeightS;
}

void UTPAnimInstance::UpdateLookAt(float DeltaSeconds)
{
	const float Dt = FMath::Clamp(DeltaSeconds, 0.f, 0.1f);
	float Yaw = 0.f, Pitch = 0.f, W = 0.f;
	if (bLookAt && UseIK())
	{
		FVector Dir = FVector::ZeroVector;
		if (const AActor* Target = Combat ? Combat->GetAttackTarget() : nullptr)
		{
			Dir = Target->GetActorLocation() + FVector(0.f, 0.f, 60.f) - (OwnerCharacter->GetActorLocation() + FVector(0.f, 0.f, 70.f));
		}
		else
		{
			Dir = OwnerCharacter->GetControlRotation().Vector();
		}
		const FVector D = ToCharacterDir(Dir).GetSafeNormal();
		if (!D.IsNearlyZero())
		{
			Yaw = FMath::Atan2(D.Y, D.X);
			Pitch = -FMath::Atan2(D.Z, FMath::Max(FVector(D.X, D.Y, 0.f).Size(), 1e-3f)) * 0.6f;
			// за спиной не выворачиваем шею: вес гаснет между 100° и 140°
			W = 1.f - Smooth01((FMath::Abs(Yaw) - 1.75f) / 0.7f);
		}
	}
	// во время паркура и полной позы боя голова — из позы действия
	if (FrameAction != A_None && !ActionUpperOnly(FrameAction))
	{
		W = 0.f;
	}
	LookYawS = FMath::FInterpTo(LookYawS, Yaw, Dt, 6.f);
	LookPitchS = FMath::FInterpTo(LookPitchS, Pitch, Dt, 6.f);
	LookWeightS = FMath::FInterpTo(LookWeightS, W, Dt, 5.f);
	FrameLookYaw = LookYawS;
	FrameLookPitch = LookPitchS;
	FrameLookWeight = LookWeightS;
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
	ActionClips.Reset();
	ActionClips.SetNumZeroed(A_Count);
	Layer.Reset();
	bLayerPose = false;
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

	// этап 7: действие кадра, контекст, постобработка
	Action = Inst->FrameAction;
	ActionU = Inst->FrameActionU;
	ActionCtx = Inst->FrameCtx;
	FootIK = Inst->FrameFootIK;
	Lean = Inst->FrameLean;
	LookYaw = Inst->FrameLookYaw;
	LookPitch = Inst->FrameLookPitch;
	LookWeight = Inst->FrameLookWeight;
	ActionClips.SetNumZeroed(A_Count);
	for (int32 a = 0; a < A_Count; ++a)
	{
		UAnimSequence* Seq = Inst->GetActionClip(a);
		if (Seq && Inst->CurrentSkeleton && Seq->GetSkeleton() != Inst->CurrentSkeleton)
		{
			Seq = nullptr;   // клип с чужого скелета (не ретаргетнут) — процедурная поза
		}
		ActionClips[a] = Seq;
	}
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
	Layer.Update(Action, ActionU, DeltaSeconds);
	bLayerPose = false;
	if (bRigBuilt && bRigCompatible && Layer.IsVisible())
	{
		Layer.Evaluate(Rig, ActionCtx);
		bLayerPose = true;
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

void FTPAnimInstanceProxy::ApplyProceduralMasked(FCompactPose& Pose, const int32* Idx, const FTPPoseCS& P, float WLower, float WUpper) const
{
	for (int32 b = B_Pelvis; b < B_Count; ++b)   // корень не трогаем
	{
		const float W = IsLowerBone(b) ? WLower : WUpper;
		if (Idx[b] == INDEX_NONE || W <= 1e-3f)
		{
			continue;
		}
		const FCompactPoseBoneIndex CI(Idx[b]);
		FTransform L = P.LocalTransform(Rig, b, RigMeshRot);
		L.SetScale3D(Pose[CI].GetScale3D());
		if (W >= 0.999f)
		{
			Pose[CI] = L;
		}
		else
		{
			const FTransform A = Pose[CI];
			Pose[CI].Blend(A, L, W);
		}
	}
}

bool FTPAnimInstanceProxy::ApplyOverrideClip(FPoseContext& Output, const int32* Idx) const
{
	const UAnimSequence* Seq = ActionClips.IsValidIndex(Layer.Cur) ? ActionClips[Layer.Cur] : nullptr;
	if (!Seq)
	{
		return false;
	}
	const float Len = FMath::Max(Seq->GetPlayLength(), 0.01f);
	const float Time = (ActionLoops(Layer.Cur) ? Detail::Frac(Layer.U) : FMath::Clamp(Layer.U, 0.f, 1.f)) * Len;
	FPoseContext Tmp(Output);
	FAnimationPoseData TmpData(Tmp);
	Seq->GetAnimationPose(TmpData, FAnimExtractContext(Time, false));
	const float WL = Layer.LowerWeight(), WU = Layer.UpperWeight();
	// маска «верх»: spine_01 и все её потомки (у родителя в компактной позе индекс всегда меньше)
	FCompactPose& Pose = Output.Pose;
	const int32 Num = Pose.GetNumBones();
	TArray<uint8, TInlineAllocator<160>> Upper;
	Upper.SetNumZeroed(Num);
	for (const FCompactPoseBoneIndex BI : Pose.ForEachBoneIndex())
	{
		const int32 i = BI.GetInt();
		const FCompactPoseBoneIndex Parent = Pose.GetParentBoneIndex(BI);
		Upper[i] = (i == Idx[B_Spine1] || (Parent.IsValid() && Upper[Parent.GetInt()])) ? 1 : 0;
		if (i == Idx[B_Root])
		{
			continue;   // корень ведёт движение актёра
		}
		const float W = Upper[i] ? WU : WL;
		if (W > 1e-3f)
		{
			const FTransform A = Pose[BI];
			Pose[BI].Blend(A, Tmp.Pose[BI], W);
		}
	}
	Pose.NormalizeRotations();
	return true;
}

bool FTPAnimInstanceProxy::PoseToBones(const FCompactPose& Pose, const int32* Idx, FTPBonesCS& O) const
{
	FTransform CS[B_Count];
	for (int32 b = 0; b < B_Count; ++b)
	{
		if (Idx[b] == INDEX_NONE)
		{
			return false;
		}
		const FTransform& L = Pose[FCompactPoseBoneIndex(Idx[b])];
		const int32 P = BoneParent(b);
		CS[b] = P < 0 ? L : L * CS[P];
		O.Pos[b] = RigMeshRot.RotateVector(CS[b].GetTranslation());
		O.Rot[b] = (RigMeshRot * CS[b].GetRotation()).GetNormalized();
	}
	return true;
}

void FTPAnimInstanceProxy::BonesToPose(FCompactPose& Pose, const int32* Idx, const FTPBonesCS& O) const
{
	for (int32 b = B_Pelvis; b < B_Count; ++b)
	{
		const int32 P = BoneParent(b);
		const FQuat PInv = O.Rot[P].Inverse();
		FTransform& T = Pose[FCompactPoseBoneIndex(Idx[b])];
		T.SetRotation((PInv * O.Rot[b]).GetNormalized());
		T.SetTranslation(PInv.RotateVector(O.Pos[b] - O.Pos[P]));
	}
}

void FTPAnimInstanceProxy::PostProcess(FCompactPose& Pose, const int32* Idx) const
{
	const float FreeLower = 1.f - Layer.LowerWeight();   // в полной позе действия наклон и IK стоп не нужны
	const float LeanNow = Lean * FreeLower;
	const float FootW = FootIK.Weight * FreeLower;
	if (!bRigBuilt || !bRigCompatible || (FMath::Abs(LeanNow) < 1e-3f && FootW < 1e-3f && LookWeight < 1e-3f))
	{
		return;
	}
	FTPBonesCS O;
	if (!PoseToBones(Pose, Idx, O))
	{
		return;   // нужные кости отсечены LOD
	}
	ApplyLean(O, LeanNow);
	ApplyLookAt(O, Rig, LookYaw, LookPitch, LookWeight);
	FTPFootIKInput F = FootIK;
	F.Weight = FootW;
	ApplyFootIK(O, F);
	BonesToPose(Pose, Idx, O);
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
	const bool bOverride = Layer.IsVisible() && ActionClips.IsValidIndex(Layer.Cur) && ActionClips[Layer.Cur] != nullptr;
	const bool bAction = bLayerPose && Layer.IsVisible() && !bOverride;
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
		if (bAction)
		{
			FTPPoseCS P;
			SolvePose(Rig, Layer.Pose, P);
			ApplyProceduralMasked(Pose, Idx, P, Layer.LowerWeight(), Layer.UpperWeight());
		}
	}
	else
	{
		if (!bRigBuilt || !bRigCompatible)
		{
			return false;   // ни клипов, ни подходящего скелета — поза покоя (её даёт движок)
		}
		Output.ResetToRefPose();
		FTPLimbPose L;
		Loco.ProceduralLimb(Rig, L);
		if (bAction)
		{
			Layer.BlendInto(L);   // смешивание целями: стопы и кисти остаются на целях
		}
		FTPPoseCS P;
		SolvePose(Rig, L, P);
		ApplyProcedural(Pose, Idx, P, 1.f);
	}
	if (bOverride)
	{
		ApplyOverrideClip(Output, Idx);
	}
	PostProcess(Pose, Idx);
	return true;
}

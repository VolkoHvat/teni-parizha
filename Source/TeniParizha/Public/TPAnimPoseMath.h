// Тени Парижа — математика поз (порт legacy/lutece/anim/lu_humanoid.c на UE 4.27).
// Только заголовок, без UObject: решатель поз по целям конечностей, двухзвенный IK с pole-вектором,
// процедурные позы (стойка, шаг, бег, спринт, присед, прыжок, падение, приземление, вис, лазание) и переходы.
//
// Пространство персонажа: сантиметры, X — вперёд, Y — вправо, Z — вверх (как у актёра UE).
// Меш манекена UE4 в своём компоненте смотрит вдоль +Y; перевод — MeshToCharacter()/CharacterToMesh().
// Индекс стороны 0 — левая (Y < 0), 1 — правая (Y > 0).
//
// Решатель не зависит от локальных осей костей конкретного меша: каждая кость получает поворот-«дельту»
// относительно своей позы покоя (компонентный поворот = Delta * RestRot). Поэтому поза покоя может быть
// и A-позой манекена, и T-позой Mixamo — меняются только длины и направления костей покоя.
//
// Углы — в радианах. Знаки: Pitch > 0 — наклон вперёд (для стопы — носок вниз), Yaw > 0 — поворот вправо
// (как у FRotator), Roll > 0 — крен вправо (правое плечо вниз).
#pragma once

#include "CoreMinimal.h"

namespace TPAnim
{
	// ---------------- скелет (имена костей манекена UE4) ----------------
	enum EBone : int32
	{
		B_Root, B_Pelvis, B_Spine1, B_Spine2, B_Spine3, B_Neck, B_Head,
		B_ClavL, B_UpperArmL, B_LowerArmL, B_HandL,
		B_ClavR, B_UpperArmR, B_LowerArmR, B_HandR,
		B_ThighL, B_CalfL, B_FootL, B_BallL,
		B_ThighR, B_CalfR, B_FootR, B_BallR,
		B_Count
	};

	inline const TCHAR* BoneName(int32 Bone)
	{
		static const TCHAR* Names[B_Count] = {
			TEXT("root"), TEXT("pelvis"), TEXT("spine_01"), TEXT("spine_02"), TEXT("spine_03"), TEXT("neck_01"), TEXT("head"),
			TEXT("clavicle_l"), TEXT("upperarm_l"), TEXT("lowerarm_l"), TEXT("hand_l"),
			TEXT("clavicle_r"), TEXT("upperarm_r"), TEXT("lowerarm_r"), TEXT("hand_r"),
			TEXT("thigh_l"), TEXT("calf_l"), TEXT("foot_l"), TEXT("ball_l"),
			TEXT("thigh_r"), TEXT("calf_r"), TEXT("foot_r"), TEXT("ball_r") };
		return (Bone >= 0 && Bone < B_Count) ? Names[Bone] : TEXT("");
	}

	// Родитель кости (-1 у корня). Родитель всегда стоит в перечислении раньше ребёнка.
	inline int32 BoneParent(int32 Bone)
	{
		static const int32 Parents[B_Count] = {
			-1, B_Root, B_Pelvis, B_Spine1, B_Spine2, B_Spine3, B_Neck,
			B_Spine3, B_ClavL, B_UpperArmL, B_LowerArmL,
			B_Spine3, B_ClavR, B_UpperArmR, B_LowerArmR,
			B_Pelvis, B_ThighL, B_CalfL, B_FootL,
			B_Pelvis, B_ThighR, B_CalfR, B_FootR };
		return (Bone >= 0 && Bone < B_Count) ? Parents[Bone] : -1;
	}

	inline int32 ClavBone(int32 Side) { return Side == 0 ? B_ClavL : B_ClavR; }
	inline int32 UpperArmBone(int32 Side) { return Side == 0 ? B_UpperArmL : B_UpperArmR; }
	inline int32 LowerArmBone(int32 Side) { return Side == 0 ? B_LowerArmL : B_LowerArmR; }
	inline int32 HandBone(int32 Side) { return Side == 0 ? B_HandL : B_HandR; }
	inline int32 ThighBone(int32 Side) { return Side == 0 ? B_ThighL : B_ThighR; }
	inline int32 CalfBone(int32 Side) { return Side == 0 ? B_CalfL : B_CalfR; }
	inline int32 FootBone(int32 Side) { return Side == 0 ? B_FootL : B_FootR; }
	inline int32 BallBone(int32 Side) { return Side == 0 ? B_BallL : B_BallR; }
	// Знак стороны по оси Y: левая -1, правая +1.
	inline float SideSign(int32 Side) { return Side == 0 ? -1.f : 1.f; }

	// ---------------- мелкая векторная математика ----------------
	inline FVector RejectFrom(const FVector& V, const FVector& UnitDir) { return V - UnitDir * FVector::DotProduct(V, UnitDir); }
	inline float Smooth01(float T) { T = FMath::Clamp(T, 0.f, 1.f); return T * T * (3.f - 2.f * T); }
	inline float Saturate(float T) { return FMath::Clamp(T, 0.f, 1.f); }

	// Любой единичный вектор, перпендикулярный единичному Dir.
	inline FVector AnyPerpendicular(const FVector& Dir)
	{
		const FVector Ref = FMath::Abs(Dir.Z) < 0.9f ? FVector(0.f, 0.f, 1.f) : FVector(1.f, 0.f, 0.f);
		return FVector::CrossProduct(Dir, Ref).GetSafeNormal();
	}

	inline FQuat AxisAngle(const FVector& Axis, float Angle) { return FQuat(Axis, Angle); }
	inline FQuat YawQ(float A) { return FQuat(FVector(0.f, 0.f, 1.f), A); }    // поворот вправо
	inline FQuat PitchQ(float A) { return FQuat(FVector(0.f, 1.f, 0.f), A); }  // наклон вперёд (Z к +X)
	inline FQuat RollQ(float A) { return FQuat(FVector(1.f, 0.f, 0.f), -A); }  // крен вправо (Z к +Y)
	// Сначала крен, затем наклон, затем рыскание — порядок как у Q3(RY, RX, RZ) в Lutèce.
	inline FQuat YprQ(float Yaw, float Pitch, float Roll) { return (YawQ(Yaw) * PitchQ(Pitch) * RollQ(Roll)).GetNormalized(); }

	// Поворот компонента меша манекена (смотрит вдоль +Y) в пространство персонажа (+X) и обратно.
	inline FQuat MeshToCharacter() { return FQuat(FVector(0.f, 0.f, 1.f), -HALF_PI); }
	inline FQuat CharacterToMesh() { return FQuat(FVector(0.f, 0.f, 1.f), HALF_PI); }

	// Поворот, который переводит RestDir в NewDir и при этом ось шарнира RestHinge — в NewHinge.
	// Все векторы единичные; оси шарнира проецируются на плоскость, перпендикулярную кости.
	inline FQuat AimWithHinge(const FVector& RestDir, const FVector& RestHinge, const FVector& NewDir, const FVector& NewHinge)
	{
		const FQuat Q1 = FQuat::FindBetweenNormals(RestDir, NewDir);
		FVector H1 = RejectFrom(Q1.RotateVector(RestHinge), NewDir).GetSafeNormal();
		FVector H2 = RejectFrom(NewHinge, NewDir).GetSafeNormal();
		if (H1.IsNearlyZero() || H2.IsNearlyZero())
		{
			return Q1;
		}
		const float SinA = FVector::DotProduct(NewDir, FVector::CrossProduct(H1, H2));
		const float CosA = FVector::DotProduct(H1, H2);
		const FQuat Q2(NewDir, FMath::Atan2(SinA, CosA));
		return (Q2 * Q1).GetNormalized();
	}

	// Двухзвенный IK (бедро–голень–стопа, плечо–предплечье–кисть).
	// A — корень цепи, L1/L2 — длины звеньев, Pole — точка, в сторону которой выгибается средний сустав,
	// FallbackBend — направление сгиба, если Pole лежит на линии цели.
	// SoftFraction — «мягкий» IK: последние доли длины цепи цель догоняется по экспоненте, без рывка колена
	// при почти выпрямленной ноге (у жёсткого IK колено там дёргается со скоростью, стремящейся к бесконечности).
	// Выход: OutB — средний сустав, OutC — конец цепи, OutBend — единичное направление сгиба.
	// Возвращает false, если конец цепи не дотянулся до цели больше чем на 0.5 мм.
	inline bool SolveTwoBoneIK(const FVector& A, float L1, float L2, const FVector& Target, const FVector& Pole,
		const FVector& FallbackBend, FVector& OutB, FVector& OutC, FVector& OutBend, float SoftFraction = 0.04f)
	{
		const FVector D = Target - A;
		const float TargetDist = D.Size();
		float Dist = TargetDist;
		FVector Dir = Dist > KINDA_SMALL_NUMBER ? D / Dist : FVector(0.f, 0.f, -1.f);
		const float Chain = L1 + L2;
		const float MaxD = Chain * 0.9999f;
		const float MinD = FMath::Abs(L1 - L2) * 1.001f + 0.01f;
		const float Soft = Chain * FMath::Clamp(SoftFraction, 0.f, 0.5f);
		const float SoftStart = Chain - Soft;
		if (Soft > KINDA_SMALL_NUMBER && Dist > SoftStart)
		{
			Dist = SoftStart + Soft * (1.f - FMath::Exp(-(Dist - SoftStart) / Soft));
		}
		if (Dist > MaxD) { Dist = MaxD; }
		if (Dist < MinD) { Dist = MinD; }
		const bool bReach = FMath::Abs(TargetDist - Dist) < 0.05f;

		const float Ax = (L1 * L1 - L2 * L2 + Dist * Dist) / (2.f * Dist);
		const float H = FMath::Sqrt(FMath::Max(L1 * L1 - Ax * Ax, 0.f));

		FVector Bend = RejectFrom(Pole - A, Dir);
		if (Bend.SizeSquared() < 1e-4f) { Bend = RejectFrom(FallbackBend, Dir); }
		Bend = Bend.SizeSquared() < 1e-8f ? AnyPerpendicular(Dir) : Bend.GetSafeNormal();

		OutB = A + Dir * Ax + Bend * H;
		OutC = A + Dir * Dist;
		OutBend = Bend;
		return bReach;
	}

	// ---------------- скелет покоя ----------------
	struct FTPRig
	{
		FVector RestPos[B_Count];   // положения суставов в пространстве персонажа
		FQuat RestRot[B_Count];     // компонентные повороты костей в позе покоя (в пространстве персонажа)
		float LegLen = 0.f;         // бедро + голень
		float ArmLen = 0.f;         // плечо + предплечье
		float FootH = 0.f;          // высота лодыжки над полом в позе покоя
		float HipH = 0.f;           // высота тазобедренного сустава
		float Scale = 1.f;          // LegLen / 84.5 см (эталон Lutèce — рост 1.78 м)

		FTPRig()
		{
			for (int32 i = 0; i < B_Count; ++i) { RestPos[i] = FVector::ZeroVector; RestRot[i] = FQuat::Identity; }
		}

		float BoneLength(int32 Bone) const
		{
			const int32 P = BoneParent(Bone);
			return P < 0 ? 0.f : (RestPos[Bone] - RestPos[P]).Size();
		}

		void Finalize()
		{
			LegLen = BoneLength(B_CalfL) + BoneLength(B_FootL);
			ArmLen = BoneLength(B_LowerArmL) + BoneLength(B_HandL);
			FootH = 0.5f * (RestPos[B_FootL].Z + RestPos[B_FootR].Z);
			HipH = 0.5f * (RestPos[B_ThighL].Z + RestPos[B_ThighR].Z);
			Scale = LegLen > 1.f ? LegLen / 84.5f : 1.f;
		}
	};

	// Приблизительный скелет манекена UE4 (SK_Mannequin), A-поза, пространство персонажа.
	// Для превью и тестов; в игре скелет берётся из FReferenceSkeleton меша (MakeRigFromMesh).
	inline FTPRig MakeMannequinRig()
	{
		FTPRig R;
		R.RestPos[B_Root] = FVector(0.f, 0.f, 0.f);
		R.RestPos[B_Pelvis] = FVector(0.f, 0.f, 97.f);
		R.RestPos[B_Spine1] = FVector(1.f, 0.f, 107.f);
		R.RestPos[B_Spine2] = FVector(1.f, 0.f, 122.f);
		R.RestPos[B_Spine3] = FVector(0.f, 0.f, 137.f);
		R.RestPos[B_Neck] = FVector(-1.f, 0.f, 153.f);
		R.RestPos[B_Head] = FVector(1.f, 0.f, 162.f);
		for (int32 S = 0; S < 2; ++S)
		{
			const float Y = SideSign(S);
			R.RestPos[ClavBone(S)] = FVector(2.f, 3.5f * Y, 149.f);
			R.RestPos[UpperArmBone(S)] = FVector(-2.f, 17.f * Y, 146.f);
			R.RestPos[LowerArmBone(S)] = FVector(-4.f, 38.f * Y, 125.f);   // плечо ~29.8 см, вниз-наружу под ~45°
			R.RestPos[HandBone(S)] = FVector(0.f, 56.f * Y, 107.f);        // предплечье ~25.8 см
			R.RestPos[ThighBone(S)] = FVector(0.f, 9.f * Y, 92.f);
			R.RestPos[CalfBone(S)] = FVector(2.f, 11.f * Y, 48.f);         // бедро ~44 см
			R.RestPos[FootBone(S)] = FVector(-1.f, 12.f * Y, 9.f);         // голень ~39 см
			R.RestPos[BallBone(S)] = FVector(13.f, 13.f * Y, 2.f);
		}
		R.Finalize();
		return R;
	}

	// Скелет из компонентных преобразований покоя меша (как их даёт FReferenceSkeleton, сложенные к корню).
	// MeshRot — поворот меша относительно актёра (у манекена — MeshToCharacter()).
	inline FTPRig MakeRigFromMesh(const FVector* CompPos, const FQuat* CompRot, const FQuat& MeshRot)
	{
		FTPRig R;
		for (int32 i = 0; i < B_Count; ++i)
		{
			R.RestPos[i] = MeshRot.RotateVector(CompPos[i]);
			R.RestRot[i] = (MeshRot * CompRot[i]).GetNormalized();
		}
		R.Finalize();
		return R;
	}

	// ---------------- поза целями конечностей ----------------
	struct FTPLimbPose
	{
		FVector RootOffset;            // смещение корня (корневое движение клипов)
		float RootYaw;
		FVector PelvisOffset;          // смещение таза от положения покоя (в осях корня)
		float PelvisYaw, PelvisPitch, PelvisRoll;
		float SpinePitch, SpineYaw, SpineRoll;   // делится поровну на три позвонка
		float NeckPitch, HeadPitch, HeadYaw, HeadRoll;
		// ноги: лодыжка в точке Foot (оси корня), колено смотрит на KneePole
		FVector Foot[2]; FVector KneePole[2];
		float FootPitch[2], FootYaw[2], ToeBend[2];
		// руки: HandIK = 0 — прямое управление углами, 1 — кисть в точке Hand; промежуточные значения смешивают
		float HandIK[2]; FVector Hand[2]; FVector ElbowPole[2];
		float ArmSwing[2];             // > 0 — рука вперёд
		float ArmOut[2];               // 0 — Т-поза, ~1.3 — рука вдоль тела, < 0 — выше плеча
		float ArmTwist[2];             // поворот плоскости локтя вокруг плеча
		float Elbow[2];                // сгиб локтя (предплечье уходит вперёд)
		float HandPitch[2], HandRoll[2], FingerCurl[2], ClavUp[2], ClavFwd[2];

		FTPLimbPose() { Reset(); }

		void Reset()
		{
			RootOffset = FVector::ZeroVector; RootYaw = 0.f;
			PelvisOffset = FVector::ZeroVector; PelvisYaw = PelvisPitch = PelvisRoll = 0.f;
			SpinePitch = SpineYaw = SpineRoll = 0.f;
			NeckPitch = HeadPitch = HeadYaw = HeadRoll = 0.f;
			for (int32 S = 0; S < 2; ++S)
			{
				Foot[S] = FVector::ZeroVector; KneePole[S] = FVector::ZeroVector;
				FootPitch[S] = FootYaw[S] = ToeBend[S] = 0.f;
				HandIK[S] = 0.f; Hand[S] = FVector::ZeroVector; ElbowPole[S] = FVector::ZeroVector;
				ArmSwing[S] = ArmOut[S] = ArmTwist[S] = Elbow[S] = 0.f;
				HandPitch[S] = HandRoll[S] = FingerCurl[S] = ClavUp[S] = ClavFwd[S] = 0.f;
			}
		}
	};

	// Стойка: ноги на ширине таза, руки вдоль тела (lu_hpose_rest).
	inline void MakeRestPose(const FTPRig& Rig, FTPLimbPose& P)
	{
		P.Reset();
		const float K = Rig.Scale;
		for (int32 S = 0; S < 2; ++S)
		{
			const float Y = SideSign(S);
			P.Foot[S] = FVector(0.f, 10.f * K * Y, Rig.FootH);
			P.KneePole[S] = FVector(150.f, 12.f * K * Y, 50.f * K);
			P.ArmOut[S] = 1.32f; P.Elbow[S] = 0.18f; P.FingerCurl[S] = 0.35f;
			P.ElbowPole[S] = FVector(-100.f, 60.f * Y, 100.f * K);
		}
	}

	// Линейное смешивание поз целями (переход между позами без скачков: цели и углы меняются непрерывно).
	inline FTPLimbPose LerpLimbPose(const FTPLimbPose& A, const FTPLimbPose& B, float Alpha)
	{
		FTPLimbPose R;
		const float T = Saturate(Alpha);
#define TP_LERP(F) R.F = FMath::Lerp(A.F, B.F, T)
		TP_LERP(RootOffset); TP_LERP(RootYaw); TP_LERP(PelvisOffset); TP_LERP(PelvisYaw); TP_LERP(PelvisPitch); TP_LERP(PelvisRoll);
		TP_LERP(SpinePitch); TP_LERP(SpineYaw); TP_LERP(SpineRoll); TP_LERP(NeckPitch); TP_LERP(HeadPitch); TP_LERP(HeadYaw); TP_LERP(HeadRoll);
		for (int32 S = 0; S < 2; ++S)
		{
			TP_LERP(Foot[S]); TP_LERP(KneePole[S]); TP_LERP(FootPitch[S]); TP_LERP(FootYaw[S]); TP_LERP(ToeBend[S]);
			TP_LERP(HandIK[S]); TP_LERP(Hand[S]); TP_LERP(ElbowPole[S]);
			TP_LERP(ArmSwing[S]); TP_LERP(ArmOut[S]); TP_LERP(ArmTwist[S]); TP_LERP(Elbow[S]);
			TP_LERP(HandPitch[S]); TP_LERP(HandRoll[S]); TP_LERP(FingerCurl[S]); TP_LERP(ClavUp[S]); TP_LERP(ClavFwd[S]);
		}
#undef TP_LERP
		return R;
	}

	// ---------------- решённая поза ----------------
	struct FTPPoseCS
	{
		FVector Pos[B_Count];     // суставы в пространстве персонажа
		FQuat Delta[B_Count];     // поворот кости относительно покоя (в пространстве персонажа)
		bool bFootReached[2];
		bool bHandReached[2];

		FTPPoseCS()
		{
			for (int32 i = 0; i < B_Count; ++i) { Pos[i] = FVector::ZeroVector; Delta[i] = FQuat::Identity; }
			bFootReached[0] = bFootReached[1] = bHandReached[0] = bHandReached[1] = true;
		}

		// Компонентный поворот кости в пространстве персонажа.
		FQuat CharRot(const FTPRig& Rig, int32 Bone) const { return (Delta[Bone] * Rig.RestRot[Bone]).GetNormalized(); }

		// Локальное преобразование кости относительно родителя (для FCompactPose). MeshRot — как в MakeRigFromMesh.
		FTransform LocalTransform(const FTPRig& Rig, int32 Bone, const FQuat& MeshRot) const
		{
			const FQuat MeshInv = MeshRot.Inverse();
			const FQuat Rot = (MeshInv * CharRot(Rig, Bone)).GetNormalized();
			const FVector P = MeshInv.RotateVector(Pos[Bone]);
			const int32 Parent = BoneParent(Bone);
			if (Parent < 0)
			{
				return FTransform(Rot, P);
			}
			const FQuat PRot = (MeshInv * CharRot(Rig, Parent)).GetNormalized();
			const FVector PP = MeshInv.RotateVector(Pos[Parent]);
			const FQuat PInv = PRot.Inverse();
			return FTransform((PInv * Rot).GetNormalized(), PInv.RotateVector(P - PP));
		}
	};

	// Прямая кинематика одной кости: Local — поворот относительно родителя в осях покоя.
	inline void FKBone(const FTPRig& Rig, FTPPoseCS& O, int32 Bone, const FQuat& Local)
	{
		const int32 P = BoneParent(Bone);
		O.Delta[Bone] = (O.Delta[P] * Local).GetNormalized();
		O.Pos[Bone] = O.Pos[P] + O.Delta[P].RotateVector(Rig.RestPos[Bone] - Rig.RestPos[P]);
	}

	// Ставит две кости цепи по решённым суставам. RestBend — направление сгиба в покое (колено — вперёд, локоть — назад).
	inline void PlaceChain(const FTPRig& Rig, FTPPoseCS& O, int32 BA, int32 BB, int32 BC,
		const FVector& NewB, const FVector& NewC, const FVector& NewBend, const FVector& RestBend)
	{
		const FVector RA = Rig.RestPos[BA], RB = Rig.RestPos[BB], RC = Rig.RestPos[BC];
		const FVector RestLine = (RC - RA).GetSafeNormal();
		FVector RestBendP = RejectFrom(RestBend, RestLine);
		RestBendP = RestBendP.SizeSquared() < 1e-6f ? AnyPerpendicular(RestLine) : RestBendP.GetSafeNormal();
		const FVector RestHinge = FVector::CrossProduct(RestLine, RestBendP);

		const FVector A = O.Pos[BA];
		const FVector NewLine = (NewC - A).GetSafeNormal();
		const FVector NewHinge = FVector::CrossProduct(NewLine, NewBend).GetSafeNormal();

		const FVector D1r = (RB - RA).GetSafeNormal(), D2r = (RC - RB).GetSafeNormal();
		const FVector D1 = (NewB - A).GetSafeNormal(), D2 = (NewC - NewB).GetSafeNormal();
		O.Delta[BA] = AimWithHinge(D1r, RestHinge, D1, NewHinge);
		O.Pos[BB] = A + O.Delta[BA].RotateVector(RB - RA);
		O.Delta[BB] = AimWithHinge(D2r, RestHinge, D2, NewHinge);
		O.Pos[BC] = O.Pos[BB] + O.Delta[BB].RotateVector(RC - RB);
	}

	// Решить позу: туловище — прямой кинематикой, ноги и руки — IK (lu_hpose_solve).
	inline void SolvePose(const FTPRig& Rig, const FTPLimbPose& P, FTPPoseCS& O)
	{
		const FQuat RootRot = YawQ(P.RootYaw);
		O.Delta[B_Root] = RootRot;
		O.Pos[B_Root] = P.RootOffset;

		// таз: смещение в осях корня
		O.Delta[B_Pelvis] = (RootRot * YprQ(P.PelvisYaw, P.PelvisPitch, P.PelvisRoll)).GetNormalized();
		O.Pos[B_Pelvis] = O.Pos[B_Root] + RootRot.RotateVector(Rig.RestPos[B_Pelvis] - Rig.RestPos[B_Root] + P.PelvisOffset);

		const FQuat SpineLocal = YprQ(P.SpineYaw / 3.f, P.SpinePitch / 3.f, P.SpineRoll / 3.f);
		FKBone(Rig, O, B_Spine1, SpineLocal);
		FKBone(Rig, O, B_Spine2, SpineLocal);
		FKBone(Rig, O, B_Spine3, SpineLocal);
		FKBone(Rig, O, B_Neck, PitchQ(P.NeckPitch));
		FKBone(Rig, O, B_Head, YprQ(P.HeadYaw, P.HeadPitch, P.HeadRoll));

		// ноги
		for (int32 S = 0; S < 2; ++S)
		{
			const int32 BT = ThighBone(S), BCalf = CalfBone(S), BF = FootBone(S), BBall = BallBone(S);
			FKBone(Rig, O, BT, FQuat::Identity);   // положение тазобедренного сустава
			const FVector Target = O.Pos[B_Root] + RootRot.RotateVector(P.Foot[S]);
			const FVector Pole = O.Pos[B_Root] + RootRot.RotateVector(P.KneePole[S]);
			const FVector Fwd = RootRot.RotateVector(FVector(1.f, 0.f, 0.f));
			FVector NB, NC, Bend;
			O.bFootReached[S] = SolveTwoBoneIK(O.Pos[BT], Rig.BoneLength(BCalf), Rig.BoneLength(BF), Target, Pole, Fwd, NB, NC, Bend);
			PlaceChain(Rig, O, BT, BCalf, BF, NB, NC, Bend, FVector(1.f, 0.f, 0.f));
			// стопа — в осях корня, независимо от голени
			const int32 PF = BoneParent(BF);
			O.Delta[BF] = (RootRot * YawQ(P.FootYaw[S]) * PitchQ(P.FootPitch[S])).GetNormalized();
			O.Pos[BF] = O.Pos[PF] + O.Delta[PF].RotateVector(Rig.RestPos[BF] - Rig.RestPos[PF]);
			FKBone(Rig, O, BBall, PitchQ(-P.ToeBend[S]));
		}

		// руки
		for (int32 S = 0; S < 2; ++S)
		{
			const float Y = SideSign(S);
			const int32 BCl = ClavBone(S), BU = UpperArmBone(S), BL = LowerArmBone(S), BH = HandBone(S);
			FKBone(Rig, O, BCl, (FQuat(FVector(1.f, 0.f, 0.f), Y * P.ClavUp[S]) * YawQ(-Y * P.ClavFwd[S])).GetNormalized());
			FKBone(Rig, O, BU, FQuat::Identity);   // положение плечевого сустава
			const float L1 = Rig.BoneLength(BL), L2 = Rig.BoneLength(BH);
			const FVector Sh = O.Pos[BU];

			// прямое управление: опустить из Т-позы на ArmOut, качнуть на ArmSwing, согнуть локоть вперёд
			const float SO = FMath::Sin(P.ArmOut[S]), CO = FMath::Cos(P.ArmOut[S]);
			const float SS = FMath::Sin(P.ArmSwing[S]), CS = FMath::Cos(P.ArmSwing[S]);
			const FVector DLoc(SO * SS, Y * CO, -SO * CS);
			const FVector FLoc = FQuat(DLoc, Y * P.ArmTwist[S]).RotateVector(FVector(CS, 0.f, SS));
			const FVector D = O.Delta[BCl].RotateVector(DLoc);
			const FVector Bf = O.Delta[BCl].RotateVector(FLoc);
			const FVector ForeDir = D * FMath::Cos(P.Elbow[S]) + Bf * FMath::Sin(P.Elbow[S]);
			const FVector DirectHand = Sh + D * L1 + ForeDir * L2;
			const FVector DirectPole = Sh + D * L1 - Bf * 50.f;

			const float W = Saturate(P.HandIK[S]);
			const FVector IKHand = O.Pos[B_Root] + RootRot.RotateVector(P.Hand[S]);
			const FVector IKPole = O.Pos[B_Root] + RootRot.RotateVector(P.ElbowPole[S]);
			const FVector Target = FMath::Lerp(DirectHand, IKHand, W);
			const FVector Pole = FMath::Lerp(DirectPole, IKPole, W);
			FVector NB, NC, Bend;
			O.bHandReached[S] = SolveTwoBoneIK(Sh, L1, L2, Target, Pole, -Bf, NB, NC, Bend) || W < 0.5f;
			PlaceChain(Rig, O, BU, BL, BH, NB, NC, Bend, FVector(-1.f, 0.f, 0.f));

			// кисть: сгиб вокруг оси локтя покоя и вращение вокруг предплечья покоя
			const FVector ForeRest = (Rig.RestPos[BH] - Rig.RestPos[BL]).GetSafeNormal();
			const FVector ArmLineRest = (Rig.RestPos[BH] - Rig.RestPos[BU]).GetSafeNormal();
			FVector HingeRest = FVector::CrossProduct(ArmLineRest, RejectFrom(FVector(-1.f, 0.f, 0.f), ArmLineRest).GetSafeNormal());
			HingeRest = HingeRest.IsNearlyZero() ? AnyPerpendicular(ForeRest) : HingeRest.GetSafeNormal();
			const FQuat HandLocal = (FQuat(HingeRest, Y * P.HandPitch[S]) * FQuat(ForeRest, P.HandRoll[S])).GetNormalized();
			O.Delta[BH] = (O.Delta[BL] * HandLocal).GetNormalized();
		}
	}

	// ---------------- переходы ----------------
	// Локальный поворот кости относительно родителя (в осях покоя).
	inline FQuat LocalDelta(const FTPPoseCS& O, int32 Bone)
	{
		const int32 P = BoneParent(Bone);
		return P < 0 ? O.Delta[Bone] : (O.Delta[P].Inverse() * O.Delta[Bone]).GetNormalized();
	}

	// Смешивание двух решённых поз по локальным поворотам: длины костей сохраняются точно.
	// Корень и таз смешиваются линейно, остальные суставы восстанавливаются прямой кинематикой.
	inline void BlendPoses(const FTPRig& Rig, const FTPPoseCS& A, const FTPPoseCS& B, float Alpha, FTPPoseCS& Out)
	{
		const float T = Saturate(Alpha);
		Out.Pos[B_Root] = FMath::Lerp(A.Pos[B_Root], B.Pos[B_Root], T);
		Out.Delta[B_Root] = FQuat::Slerp(A.Delta[B_Root], B.Delta[B_Root], T).GetNormalized();
		for (int32 i = 1; i < B_Count; ++i)
		{
			const FQuat L = FQuat::Slerp(LocalDelta(A, i), LocalDelta(B, i), T).GetNormalized();
			FKBone(Rig, Out, i, L);
			if (i == B_Pelvis)
			{
				Out.Pos[i] = FMath::Lerp(A.Pos[i], B.Pos[i], T);   // смещение таза — не поворот
			}
		}
		for (int32 S = 0; S < 2; ++S)
		{
			Out.bFootReached[S] = A.bFootReached[S] && B.bFootReached[S];
			Out.bHandReached[S] = A.bHandReached[S] && B.bHandReached[S];
		}
	}

	// Плавный переход: запоминает позу-источник и за Duration секунд сводит её к текущей (сглаженная кривая).
	struct FTPPoseTransition
	{
		FTPPoseCS From;
		float Duration = 0.f;
		float Elapsed = 0.f;

		void Start(const FTPPoseCS& Current, float InDuration) { From = Current; Duration = FMath::Max(InDuration, 0.f); Elapsed = 0.f; }
		bool IsActive() const { return Duration > 0.f && Elapsed < Duration; }
		float Alpha() const { return Duration > 0.f ? Smooth01(Elapsed / Duration) : 1.f; }

		// Target — новая поза этого кадра; Out получает смесь. Можно передать один и тот же объект в Target и Out.
		void Apply(const FTPRig& Rig, float DeltaTime, const FTPPoseCS& Target, FTPPoseCS& Out)
		{
			if (!IsActive()) { if (&Out != &Target) { Out = Target; } return; }
			Elapsed += FMath::Max(DeltaTime, 0.f);
			FTPPoseCS Mixed;
			BlendPoses(Rig, From, Target, Alpha(), Mixed);
			Out = Mixed;
		}
	};

	// Критически задемпфированная пружина (сглаживание углов наклона, смещения таза и т. п.).
	inline void SpringDamp(float& Value, float& Velocity, float Target, float HalfLife, float DeltaTime)
	{
		const float Omega = 1.3862944f / FMath::Max(HalfLife, 1e-4f);   // 2*ln2 / период полураспада
		const float X = Omega * DeltaTime;
		const float E = 1.f / (1.f + X + 0.48f * X * X + 0.235f * X * X * X);
		const float Diff = Value - Target;
		const float Tmp = (Velocity + Omega * Diff) * DeltaTime;
		Velocity = (Velocity - Omega * Tmp) * E;
		Value = Target + (Diff + Tmp) * E;
	}
	inline void SpringDamp(FVector& Value, FVector& Velocity, const FVector& Target, float HalfLife, float DeltaTime)
	{
		SpringDamp(Value.X, Velocity.X, Target.X, HalfLife, DeltaTime);
		SpringDamp(Value.Y, Velocity.Y, Target.Y, HalfLife, DeltaTime);
		SpringDamp(Value.Z, Velocity.Z, Target.Z, HalfLife, DeltaTime);
	}

	// ---------------- процедурные позы ----------------
	enum EPose : int32
	{
		P_Idle, P_Walk, P_Run, P_Sprint, P_CrouchIdle, P_CrouchWalk,
		P_JumpUp, P_JumpAir, P_Fall, P_LandSoft, P_LandHard, P_Hang, P_Brace, P_Climb,
		P_Count
	};

	inline const TCHAR* PoseName(int32 Pose)
	{
		static const TCHAR* Names[P_Count] = {
			TEXT("idle"), TEXT("walk"), TEXT("run"), TEXT("sprint"), TEXT("crouch_idle"), TEXT("crouch_walk"),
			TEXT("jump_up"), TEXT("jump_air"), TEXT("fall"), TEXT("land_soft"), TEXT("land_hard"), TEXT("hang"), TEXT("brace"), TEXT("climb") };
		return (Pose >= 0 && Pose < P_Count) ? Names[Pose] : TEXT("");
	}

	// Зациклена ли поза (фазы 0 и 1 совпадают).
	inline bool PoseLoops(int32 Pose)
	{
		return !(Pose == P_JumpUp || Pose == P_LandSoft || Pose == P_LandHard);
	}

	// Скорость (см/с), под которую сделаны циклы: стоя, шаг, бег, спринт (lu_gait_speed).
	inline float GaitSpeed(int32 Gait)
	{
		static const float Speeds[4] = { 0.f, 145.f, 460.f, 740.f };
		return Speeds[FMath::Clamp(Gait, 0, 3)];
	}

	namespace Detail
	{
		// Перевод чисел Lutèce (метры; Y вверх, +Z вперёд, +X влево) в сантиметры пространства персонажа.
		inline FVector LU(const FTPRig& R, float X, float Y, float Z) { return FVector(Z, -X, Y) * (100.f * R.Scale); }
		inline float Ease(float T) { return Smooth01(T); }
		// Кусочная кривая по ключам (u, v) со сглаживанием между ключами.
		inline float Curve(float U, const float* K, int32 N)
		{
			if (U <= K[0]) { return K[1]; }
			for (int32 i = 0; i + 1 < N; ++i)
			{
				if (U <= K[(i + 1) * 2])
				{
					const float A = K[i * 2], B = K[(i + 1) * 2];
					return FMath::Lerp(K[i * 2 + 1], K[(i + 1) * 2 + 1], Ease((U - A) / (B - A)));
				}
			}
			return K[(N - 1) * 2 + 1];
		}
		inline float Frac(float X) { return X - FMath::FloorToFloat(X); }

		struct FGait { float Stride, Duty, Lift, Bounce, Drop, Lean, Arm, Elbow, Yaw, Roll, Heel, Sx, Crouch, Kick; };

		inline const FGait& GaitParams(int32 Pose)
		{
			//                         шаг   опора подъём качан. просад наклон руки  локоть рыск. крен  пятка шир.  присед мах
			static const FGait Walk = { 1.45f, .62f, .1f, .018f, .02f, .06f, .35f, .25f, .1f, .04f, 1.f, .09f, 0.f, 0.f };
			static const FGait Run = { 2.3f, .38f, .2f, -.035f, .07f, .3f, .75f, 1.4f, .14f, .05f, .8f, .07f, 0.f, .14f };
			static const FGait Sprint = { 3.0f, .34f, .26f, -.04f, .09f, .5f, .95f, 1.55f, .16f, .05f, .7f, .06f, 0.f, .2f };
			static const FGait Crouch = { 1.1f, .6f, .1f, .01f, .05f, .3f, .2f, .8f, .08f, .03f, .6f, .12f, .32f, 0.f };
			return Pose == P_Run ? Run : Pose == P_Sprint ? Sprint : Pose == P_CrouchWalk ? Crouch : Walk;
		}

		// Походка (порт gait()). Размах рук бега и спринта уменьшен (0.9→0.75, 1.25→0.95): с наклоном корпуса
		// отведённое назад плечо в Lutèce поднималось выше горизонтали.
		// Исправлено против Lutèce: на отрыве носка стопа больше не «падает»
		// на 7 см и пальцы не распрямляются скачком — подъём пятки и сгиб пальцев плавно гаснут в переносе.
		inline void Gait(const FTPRig& R, const FGait& G, float U, FTPLimbPose& P)
		{
			const float K = R.Scale, FootHm = R.FootH / (100.f * K);
			for (int32 S = 0; S < 2; ++S)
			{
				const float Sg = S == 0 ? 1.f : -1.f;   // знак стороны в осях Lutèce (+X — влево)
				const float Ph = Frac(U + (S ? .5f : 0.f));
				const float L = G.Stride * .5f, D = G.Duty;
				const float PitchOff = .55f * G.Heel + .1f;
				float Z, Y = FootHm, Pitch, Toe;
				if (Ph < D)
				{
					const float St = Ph / D;
					Z = FMath::Lerp(L * D, -L * D, St);
					Pitch = St < .15f ? FMath::Lerp(-.25f * G.Heel, 0.f, St / .15f) : St > .7f ? FMath::Lerp(0.f, PitchOff, (St - .7f) / .3f) : 0.f;
					if (Pitch > 0.f) { Y += .12f * FMath::Sin(Pitch); }
					Toe = Pitch > 0.f ? Pitch * .8f : 0.f;
				}
				else
				{
					const float Sw = (Ph - D) / (1.f - D);
					Z = FMath::Lerp(-L * D, L * D, Ease(Sw));
					Y += G.Lift * FMath::Sin(PI * FMath::Pow(Sw, .8f)) + G.Kick * FMath::Sin(PI * FMath::Min(Sw * 1.6f, 1.f)) * (1.f - Sw);
					Pitch = FMath::Lerp(PitchOff, -.25f * G.Heel, Ease(Sw));
					const float Fade = 1.f - Ease(Sw * 2.5f);
					Y += .12f * FMath::Sin(PitchOff) * Fade;
					Toe = PitchOff * .8f * Fade;
				}
				P.Foot[S] = LU(R, Sg * G.Sx, 0.f, Z) + FVector(0.f, 0.f, R.FootH) + FVector(0.f, 0.f, (Y - FootHm) * 100.f * K);
				P.FootPitch[S] = Pitch; P.ToeBend[S] = Toe;
				P.KneePole[S] = LU(R, Sg * .15f, .6f, 2.f);
				// руки в противофазе с ногами
				const float Swing = -G.Arm * FMath::Cos(2.f * PI * Ph);
				P.ArmSwing[S] = Swing;
				P.Elbow[S] = G.Elbow + FMath::Max(0.f, Swing) * G.Elbow * .6f;
				P.ArmOut[S] = 1.3f - G.Elbow * .08f;
				P.FingerCurl[S] = .4f + G.Elbow * .3f;
			}
			const float Bob = FMath::Cos(2.f * PI * 2.f * (U - G.Duty * .5f));
			P.PelvisOffset = LU(R, 0.f, -G.Drop + G.Bounce * Bob - G.Crouch, G.Lean * .05f);
			P.PelvisYaw = -G.Yaw * FMath::Sin(2.f * PI * U);
			P.PelvisRoll = G.Roll * FMath::Cos(2.f * PI * U);
			P.PelvisPitch = G.Lean * .35f + G.Crouch * .4f;
			P.SpineYaw = -P.PelvisYaw * 1.5f;
			P.SpinePitch = G.Lean * .65f + G.Crouch * .5f;
			P.SpineRoll = -P.PelvisRoll * .7f;
			P.HeadPitch = -(P.SpinePitch + P.PelvisPitch) * .7f;
			P.HeadYaw = -P.SpineYaw * .4f - P.PelvisYaw * .5f;
		}

		inline FVector FootAt(const FTPRig& R, float X, float Lift, float Z)
		{
			return LU(R, X, Lift, Z) + FVector(0.f, 0.f, R.FootH);
		}
	}

	// Поза Pose в фазе U (0..1). P заполняется целиком (начиная со стойки покоя).
	inline void EvaluatePose(const FTPRig& R, int32 Pose, float U, FTPLimbPose& P)
	{
		using namespace Detail;
		MakeRestPose(R, P);
		const float Tau = 2.f * PI;
		switch (Pose)
		{
		case P_Walk: case P_Run: case P_Sprint: case P_CrouchWalk:
			Gait(R, GaitParams(Pose), U, P);
			break;

		case P_Idle:
		{
			const float B = FMath::Sin(Tau * U);
			// в Lutèce покачивание шло с периодом 2 цикла (sin(πu)) и давало скачок на стыке — здесь период 1
			const float Hs = FMath::Sin(Tau * U);
			P.PelvisOffset = LU(R, .012f * Hs, -.012f + .004f * B, 0.f);
			P.PelvisRoll = .02f * Hs;
			P.SpinePitch = .02f + .015f * B; P.HeadPitch = -.02f - .01f * B;
			P.HeadYaw = -.06f * FMath::Sin(Tau * U + 1.f);
			for (int32 S = 0; S < 2; ++S)
			{
				const float Sg = S == 0 ? 1.f : -1.f;
				P.Foot[S] = FootAt(R, Sg * .12f, 0.f, S ? -.03f : .04f);
				P.FootYaw[S] = -Sg * .12f;
				P.ArmOut[S] = 1.33f - .015f * B; P.Elbow[S] = .2f + .02f * B; P.ClavUp[S] = .02f * B;
			}
			break;
		}

		case P_CrouchIdle:
		{
			const float B = FMath::Sin(Tau * U);
			P.PelvisOffset = LU(R, 0.f, -.38f + .006f * B, -.05f);
			P.PelvisPitch = .35f; P.SpinePitch = .45f + .02f * B; P.HeadPitch = -.55f;
			for (int32 S = 0; S < 2; ++S)
			{
				const float Sg = S == 0 ? 1.f : -1.f;
				P.Foot[S] = FootAt(R, Sg * .15f, 0.f, S ? -.12f : .14f);
				P.FootYaw[S] = -Sg * .2f; P.ArmOut[S] = 1.2f; P.ArmSwing[S] = .5f; P.Elbow[S] = .9f;
			}
			break;
		}

		case P_JumpUp:
		{
			static const float Keys[] = { 0.f, 0.f, .35f, 1.f, 1.f, -.3f };
			const float C = Curve(U, Keys, 3);
			P.PelvisOffset = LU(R, 0.f, -.22f * C, 0.f);
			P.SpinePitch = .25f * C; P.PelvisPitch = .2f * C; P.HeadPitch = -.3f * C;
			for (int32 S = 0; S < 2; ++S)
			{
				const float Sg = S == 0 ? 1.f : -1.f;
				P.Foot[S] = FootAt(R, Sg * .12f, FMath::Max(0.f, -C) * .25f, 0.f);
				P.FootPitch[S] = FMath::Max(0.f, -C) * 1.2f;
				P.ArmSwing[S] = FMath::Lerp(-.6f * C, 1.6f, Ease((U - .35f) / .65f));
				P.Elbow[S] = .4f; P.ArmOut[S] = 1.2f;
			}
			break;
		}

		case P_JumpAir:
		{
			const float B = FMath::Sin(Tau * U);
			P.PelvisOffset = LU(R, 0.f, .02f, 0.f); P.SpinePitch = .1f; P.HeadPitch = -.1f;
			for (int32 S = 0; S < 2; ++S)
			{
				const float Sg = S == 0 ? 1.f : -1.f;
				P.Foot[S] = FootAt(R, Sg * .12f, .3f + (S ? .1f : 0.f) - .075f, S ? -.1f : .18f);
				P.FootPitch[S] = .5f;
				P.ArmSwing[S] = 1.f + .1f * B; P.ArmOut[S] = .9f; P.Elbow[S] = .6f;
			}
			break;
		}

		case P_Fall:
		{
			const float B = FMath::Sin(Tau * U), C = FMath::Cos(Tau * U);
			P.SpinePitch = -.05f; P.HeadPitch = .2f;
			for (int32 S = 0; S < 2; ++S)
			{
				const float Sg = S == 0 ? 1.f : -1.f, W = S ? C : B;
				P.Foot[S] = FootAt(R, Sg * .14f, .22f + .06f * W - .075f, .08f * W);
				P.FootPitch[S] = .4f;
				P.ArmOut[S] = .35f + .2f * W; P.ArmSwing[S] = .6f + .4f * W; P.Elbow[S] = .7f;
			}
			break;
		}

		case P_LandSoft: case P_LandHard:
		{
			const float Hard = Pose == P_LandHard ? 1.f : 0.f;
			const float Keys[] = { 0.f, .2f, .18f, 1.f, .55f + Hard * .3f, .8f, 1.f, 0.f };
			const float C = Curve(U, Keys, 4);
			P.PelvisOffset = LU(R, 0.f, -(.28f + .25f * Hard) * C, -.05f * C);
			P.PelvisPitch = (.3f + .3f * Hard) * C; P.SpinePitch = (.35f + .4f * Hard) * C; P.HeadPitch = -.5f * C;
			for (int32 S = 0; S < 2; ++S)
			{
				const float Sg = S == 0 ? 1.f : -1.f;
				P.Foot[S] = FootAt(R, Sg * .15f, 0.f, S ? -.08f : .1f);
				P.FootYaw[S] = -Sg * .15f;
				// жёсткое приземление: ладони к земле; в Lutèce IK включался скачком при c > 0.5 — здесь плавно по фазе
				P.HandIK[S] = Hard * Smooth01((U - .03f) / .2f) * Smooth01((.8f - U) / .25f);
				// ладони у земли (в Lutèce 5 см над полом — недосягаемо при этом приседе, кисть зависала на 14 см выше цели)
				P.Hand[S] = LU(R, Sg * .25f, .16f + .3f * (1.f - C), .28f);
				P.ElbowPole[S] = LU(R, Sg * 1.f, .8f, -.3f);
				P.ArmOut[S] = 1.f; P.ArmSwing[S] = .5f * C; P.Elbow[S] = .6f;
			}
			break;
		}

		case P_Hang: case P_Brace:
		{
			// вис на карнизе: верх уступа ~2.05 м над корнем, стена в 0.32 м впереди
			const bool bBraced = Pose == P_Brace;
			const float Sw = FMath::Sin(Tau * U), LedgeY = 2.05f, WallZ = .32f;
			P.PelvisOffset = LU(R, 0.f, -.03f, .06f + (bBraced ? .06f : 0.f));
			P.PelvisPitch = bBraced ? -.2f : -.05f + .03f * Sw; P.HeadPitch = -.45f;
			for (int32 S = 0; S < 2; ++S)
			{
				const float Sg = S == 0 ? 1.f : -1.f;
				P.HandIK[S] = 1.f;
				P.Hand[S] = LU(R, Sg * .24f, LedgeY - .02f, WallZ + .02f);
				P.ElbowPole[S] = LU(R, Sg * 1.f, 1.5f, -.5f);
				P.FingerCurl[S] = 1.3f; P.HandPitch[S] = .9f;
				if (bBraced)
				{
					P.Foot[S] = LU(R, Sg * .17f, .62f, WallZ - .12f); P.FootPitch[S] = .2f;
					P.KneePole[S] = LU(R, Sg * .4f, .7f, 2.f);
				}
				else
				{
					P.Foot[S] = LU(R, Sg * .1f, .14f + .02f * Sw * Sg, .08f + .04f * Sw); P.FootPitch[S] = .6f;
				}
			}
			break;
		}

		case P_Climb:
		{
			const float WallZ = .32f;
			P.PelvisOffset = LU(R, 0.f, -.05f, .1f);
			P.PelvisPitch = -.12f; P.SpinePitch = .05f; P.HeadPitch = -.35f;
			for (int32 S = 0; S < 2; ++S)
			{
				const float Sg = S == 0 ? 1.f : -1.f, Ph = Frac(U + (S ? .5f : 0.f));
				float Hy, Fy;
				if (Ph < .5f) { Hy = FMath::Lerp(2.15f, 1.75f, Ph / .5f); Fy = FMath::Lerp(.55f, .15f, Ph / .5f); }
				else { const float T = (Ph - .5f) / .5f; Hy = FMath::Lerp(1.75f, 2.15f, Ease(T)); Fy = FMath::Lerp(.15f, .55f, Ease(T)); }
				const float Reach = Ph >= .5f ? FMath::Sin(PI * (Ph - .5f) * 2.f) : 0.f;
				P.HandIK[S] = 1.f;
				P.Hand[S] = LU(R, Sg * .2f, Hy, WallZ - .02f - .06f * Reach);
				P.ElbowPole[S] = LU(R, Sg * .9f, 1.2f, -.6f);
				P.FingerCurl[S] = Ph < .5f ? 1.1f : .4f; P.HandPitch[S] = .4f;
				const int32 F = (S + 1) % 2;   // противоположная нога
				P.Foot[F] = FootAt(R, -Sg * .14f, Fy, WallZ - .1f - .1f * Reach);
				P.FootPitch[F] = .3f;
				P.KneePole[F] = LU(R, -Sg * .3f, .6f, 2.f);
			}
			P.PelvisRoll = .06f * FMath::Sin(Tau * U); P.SpineRoll = -.08f * FMath::Sin(Tau * U);
			break;
		}

		default:
			break;
		}
	}
}

// Тени Парижа — позы паркура и боя без UObject (для UTPAnimInstance и тестов вне UE).
// Каждое действие — поза целями конечностей (FTPLimbPose) по доле выполнения U = 0..1 (или фазе цикла);
// цели рук и стоп берутся из контекста (кромка, стена, руки на уступе из GetHandIK) в пространстве персонажа.
// Слой действий (FTPActionLayer) плавно включает действие поверх локомоции с маской «всё тело» или «верх»
// и делает перекрёстный переход между действиями без скачков.
// Ниже — постобработка решённой позы: наклон в повороте, взгляд и IK стоп (FTPBonesCS).
// Единицы: см, радианы; оси — как в TPAnimPoseMath.h (X вперёд, Y вправо, Z вверх; 0 — левая сторона).
#pragma once

#include "CoreMinimal.h"
#include "TPAnimPoseMath.h"

namespace TPAnim
{
	// ---------------- действия ----------------
	enum EAction : int32
	{
		A_None,
		// паркур
		A_Hang, A_Climb, A_ClimbUp, A_Mantle, A_Vault, A_WallRun, A_Roll, A_Turn, A_Drop, A_Hop, A_LandHard,
		// бой
		A_Guard, A_Attack1, A_Attack2, A_Attack3, A_Counter, A_Block, A_Dodge, A_WindUp, A_Strike, A_HitReact, A_Stagger,
		A_Count
	};

	// Имя действия — оно же ключ в OverrideSequences (подмена позы клипом, например из Mixamo).
	inline const TCHAR* ActionName(int32 A)
	{
		static const TCHAR* Names[A_Count] = {
			TEXT("None"),
			TEXT("Hang"), TEXT("Climb"), TEXT("ClimbUp"), TEXT("Mantle"), TEXT("Vault"), TEXT("WallRun"), TEXT("Roll"),
			TEXT("Turn"), TEXT("Drop"), TEXT("Hop"), TEXT("LandHard"),
			TEXT("Guard"), TEXT("Attack1"), TEXT("Attack2"), TEXT("Attack3"), TEXT("Counter"), TEXT("Block"), TEXT("Dodge"),
			TEXT("WindUp"), TEXT("Strike"), TEXT("HitReact"), TEXT("Stagger") };
		return (A >= 0 && A < A_Count) ? Names[A] : TEXT("");
	}

	// Зациклено ли действие (U — фаза цикла), иначе U — доля выполнения.
	inline bool ActionLoops(int32 A)
	{
		return A == A_Hang || A == A_Climb || A == A_WallRun || A == A_Guard || A == A_Block;
	}

	// Маска: верхние действия (удары, блок, стойка) не трогают таз и ноги — их ведёт локомоция.
	inline bool ActionUpperOnly(int32 A)
	{
		return A == A_Guard || A == A_Attack1 || A == A_Attack2 || A == A_Attack3 || A == A_Block
			|| A == A_WindUp || A == A_Strike || A == A_HitReact;
	}

	// Время включения действия, с (выключение — так же).
	inline float ActionBlendTime(int32 A)
	{
		switch (A)
		{
		case A_Attack1: case A_Attack2: case A_Attack3: case A_Counter: case A_Strike: case A_HitReact: return 0.1f;
		case A_Dodge: case A_Hop: case A_Vault: return 0.08f;
		case A_Roll: case A_LandHard: return 0.05f;
		case A_Guard: return 0.25f;
		default: return 0.15f;
		}
	}

	// Контекст действия: всё в пространстве персонажа (корень — у ног капсулы).
	struct FTPActionContext
	{
		FVector Hand[2];     // точки ладоней на кромке или стене (из GetHandIK)
		FVector Ledge;       // точка на кромке (середина между ладонями); Z — высота кромки над корнем
		float WallX;         // расстояние до стены вперёд (лазание, бег по стене)
		float Shimmy;        // перебежка в висе / сдвиг при лазании: -1 влево, +1 вправо
		float ClimbRate;     // скорость цикла лазания (0 — висит неподвижно)
		FVector DodgeDir;    // направление уклонения (единичное, XY)
		float AttackSide;    // для стражи: 0 — удар сверху, 1 — наискось (выбор варианта)

		FTPActionContext() { Reset(); }
		void Reset()
		{
			// как в игре: центр капсулы в 115 см под кромкой и в 40 см от стены, капсула 92 см
			Ledge = FVector(40.f, 0.f, 207.f);
			Hand[0] = FVector(44.f, -22.f, 209.f);
			Hand[1] = FVector(44.f, 22.f, 209.f);
			WallX = 40.f; Shimmy = 0.f; ClimbRate = 0.f;
			DodgeDir = FVector(-1.f, 0.f, 0.f); AttackSide = 0.f;
		}
	};

	namespace Act
	{
		inline float Bell(float U, float A, float B) { return Smooth01((U - A) / FMath::Max(B - A, 1e-3f)); }
		// окно: 0 до A, 1 между B и C, 0 после D (плавные края)
		inline float Window(float U, float A, float B, float C, float D) { return Bell(U, A, B) * (1.f - Bell(U, C, D)); }

		// Смешивание только верхней части (позвоночник, шея, голова, руки).
		inline void LerpUpperInto(FTPLimbPose& R, const FTPLimbPose& B, float T)
		{
			T = Saturate(T);
#define TP_UL(F) R.F = FMath::Lerp(R.F, B.F, T)
			TP_UL(SpinePitch); TP_UL(SpineYaw); TP_UL(SpineRoll); TP_UL(NeckPitch); TP_UL(HeadPitch); TP_UL(HeadYaw); TP_UL(HeadRoll);
			for (int32 S = 0; S < 2; ++S)
			{
				TP_UL(HandIK[S]); TP_UL(Hand[S]); TP_UL(ElbowPole[S]);
				TP_UL(ArmSwing[S]); TP_UL(ArmOut[S]); TP_UL(ArmTwist[S]); TP_UL(Elbow[S]);
				TP_UL(HandPitch[S]); TP_UL(HandRoll[S]); TP_UL(FingerCurl[S]); TP_UL(ClavUp[S]); TP_UL(ClavFwd[S]);
			}
#undef TP_UL
		}

		// Смешивание нижней части (корень, таз, ноги).
		inline void LerpLowerInto(FTPLimbPose& R, const FTPLimbPose& B, float T)
		{
			T = Saturate(T);
#define TP_LL(F) R.F = FMath::Lerp(R.F, B.F, T)
			TP_LL(RootOffset); TP_LL(RootYaw); TP_LL(PelvisOffset); TP_LL(PelvisYaw); TP_LL(PelvisPitch); TP_LL(PelvisRoll);
			for (int32 S = 0; S < 2; ++S)
			{
				TP_LL(Foot[S]); TP_LL(KneePole[S]); TP_LL(FootPitch[S]); TP_LL(FootYaw[S]); TP_LL(ToeBend[S]);
			}
#undef TP_LL
		}

		// Середина плечевых суставов покоя.
		inline FVector ShoulderMid(const FTPRig& R) { return (R.RestPos[B_UpperArmL] + R.RestPos[B_UpperArmR]) * 0.5f; }

		// Тело висит на руках: плечи в Gap см от стены и ниже ладоней на почти вытянутую руку.
		// Возвращает смещение таза (в осях корня), при котором плечи оказываются там.
		inline FVector HangBodyOffset(const FTPRig& R, const FVector& HandMid, float WallX, float Gap, float Reach)
		{
			const FVector S0 = ShoulderMid(R);
			const float Sx = WallX - Gap;
			const float Dx = HandMid.X - Sx;
			const float Dz = FMath::Sqrt(FMath::Max(Reach * Reach - Dx * Dx, 0.f));
			return FVector(Sx - S0.X, 0.f, FMath::Clamp(HandMid.Z - Dz - S0.Z, -120.f, 60.f));
		}

		// Ноги свободно свисают под тазом (чуть согнуты, носки к стене не достают).
		inline void DangleLegs(const FTPRig& R, FTPLimbPose& P, float Sway, float Tuck)
		{
			const float K = R.Scale;
			for (int32 S = 0; S < 2; ++S)
			{
				const float Y = SideSign(S);
				const float W = Sway * (S == 0 ? 1.f : -1.f);
				const FVector Hip = R.RestPos[ThighBone(S)] + P.PelvisOffset;
				P.Foot[S] = FVector(Hip.X + (4.f + 6.f * W + 20.f * Tuck) * K, Y * 10.f * K, Hip.Z - (R.LegLen * (0.97f - 0.35f * Tuck)));
				P.KneePole[S] = FVector(Hip.X + 150.f, Y * 14.f * K, Hip.Z - 30.f * K);
				P.FootPitch[S] = 0.5f + 0.2f * Tuck;
			}
		}

		// Руки на точках Hand (IK), локти наружу-назад, пальцы обхватывают кромку.
		inline void HandsOn(FTPLimbPose& P, const FVector* Hand, float Weight, float PoleUp)
		{
			for (int32 S = 0; S < 2; ++S)
			{
				const float Y = SideSign(S);
				P.HandIK[S] = Weight;
				P.Hand[S] = Hand[S];
				P.ElbowPole[S] = Hand[S] + FVector(-80.f, Y * 70.f, PoleUp);
				P.FingerCurl[S] = FMath::Lerp(P.FingerCurl[S], 1.3f, Weight);
				P.HandPitch[S] = FMath::Lerp(P.HandPitch[S], 0.9f, Weight);
			}
		}

		// Кусочное сглаженное смешивание ключевых поз: Keys[i] в моменты Times[i] (по возрастанию).
		inline FTPLimbPose KeyLerp(const FTPLimbPose* Keys, const float* Times, int32 N, float U)
		{
			if (U <= Times[0]) { return Keys[0]; }
			for (int32 i = 0; i + 1 < N; ++i)
			{
				if (U <= Times[i + 1])
				{
					return LerpLimbPose(Keys[i], Keys[i + 1], Smooth01((U - Times[i]) / FMath::Max(Times[i + 1] - Times[i], 1e-4f)));
				}
			}
			return Keys[N - 1];
		}

		// Боевая стойка шпагой (правая рука вперёд, левая за спиной для равновесия), только верх.
		inline void GuardUpper(const FTPRig& R, FTPLimbPose& P, float Breath)
		{
			const float K = R.Scale;
			P.SpineYaw = 0.25f; P.SpinePitch = 0.08f + 0.01f * Breath; P.HeadYaw = -0.22f; P.HeadPitch = -0.04f;
			// правая: кисть перед грудью справа, остриё вперёд
			P.HandIK[1] = 1.f;
			P.Hand[1] = FVector(32.f * K, 14.f * K, 118.f * K + 1.5f * Breath);
			P.ElbowPole[1] = FVector(-30.f, 90.f, 60.f * K);
			P.HandPitch[1] = -0.3f; P.FingerCurl[1] = 1.2f;
			// левая: согнута, кисть у пояса сзади
			P.ArmOut[0] = 1.f; P.ArmSwing[0] = -0.35f; P.Elbow[0] = 1.3f; P.FingerCurl[0] = 0.5f;
		}
	}

	// ---------------- позы действий ----------------
	// Поза действия A в момент U (доля выполнения или фаза цикла). P заполняется целиком.
	inline void EvaluateAction(const FTPRig& R, int32 A, float U, const FTPActionContext& C, FTPLimbPose& P)
	{
		using namespace Act;
		const float K = R.Scale, Tau = 2.f * PI;
		U = ActionLoops(A) ? Detail::Frac(U) : Saturate(U);
		MakeRestPose(R, P);
		switch (A)
		{
		case A_Hang:
		{
			// вис на кромке; при перебежке ведущая рука уходит вперёд, ведомая догоняет
			const float Sh = FMath::Clamp(C.Shimmy, -1.f, 1.f);
			const float S1 = FMath::Sin(PI * U), S2 = FMath::Cos(PI * U), Sw = FMath::Sin(Tau * U);
			FVector H[2] = { C.Hand[0], C.Hand[1] };
			const int32 Lead = Sh >= 0.f ? 1 : 0;
			H[Lead].Y += Sh * 12.f * K * S1 * S1;
			H[Lead].Z += FMath::Abs(Sh) * 5.f * FMath::Max(0.f, Sw);
			H[1 - Lead].Y += Sh * 10.f * K * S2 * S2 - Sh * 10.f * K;
			H[1 - Lead].Z += FMath::Abs(Sh) * 5.f * FMath::Max(0.f, -Sw);
			const FVector Mid = (C.Hand[0] + C.Hand[1]) * 0.5f;
			P.PelvisOffset = HangBodyOffset(R, Mid, C.WallX, 18.f * K, R.ArmLen * 0.97f)
				+ FVector(0.f, -Sh * 3.f * K * Sw, 1.5f * K * Sw * (1.f - FMath::Abs(Sh)));
			P.PelvisPitch = -0.05f + 0.03f * Sw; P.PelvisRoll = 0.05f * Sh * Sw;
			P.SpinePitch = -0.08f; P.HeadPitch = -0.35f; P.HeadYaw = 0.25f * Sh;
			HandsOn(P, H, 1.f, -40.f);
			DangleLegs(R, P, 0.25f * Sw * (0.4f + 0.6f * FMath::Abs(Sh)), 0.f);
			break;
		}

		case A_Climb:
		{
			// лазание по стене: руки и ноги попеременно, тело у стены на почти вытянутых руках
			const FVector Mid = (C.Hand[0] + C.Hand[1]) * 0.5f;
			const float Wall = Mid.X;
			const float Side = FMath::Clamp(C.Shimmy, -1.f, 1.f);
			FVector H[2];
			for (int32 S = 0; S < 2; ++S)
			{
				const float Ph = Detail::Frac(U + (S ? .5f : 0.f));
				const float Dz = Ph < .5f ? FMath::Lerp(18.f, -14.f, Ph / .5f) : FMath::Lerp(-14.f, 18.f, Smooth01((Ph - .5f) / .5f));
				const float Reach = Ph >= .5f ? FMath::Sin(PI * (Ph - .5f) * 2.f) : 0.f;
				const float Dy = Side * 8.f * K * (Ph < .5f ? 1.f - Ph / .5f : Smooth01((Ph - .5f) / .5f));
				H[S] = C.Hand[S] + FVector(-3.f - 6.f * Reach, Dy, Dz * K);
				P.FingerCurl[S] = Ph < .5f ? 1.1f : 0.5f;
				const int32 F = 1 - S;   // противоположная нога
				const float Fz = Ph < .5f ? FMath::Lerp(58.f, 18.f, Ph / .5f) : FMath::Lerp(18.f, 58.f, Smooth01((Ph - .5f) / .5f));
				P.Foot[F] = FVector(Wall - (14.f + 9.f * Reach) * K, SideSign(F) * 14.f * K + Dy, R.FootH + Fz * K);
				P.FootPitch[F] = 0.3f;
				P.KneePole[F] = FVector(Wall + 100.f, SideSign(F) * 40.f * K, 80.f * K);
			}
			const float Bob = FMath::Cos(2.f * Tau * U);
			// тело висит на верхней руке (она тянется выше всего), нижняя рука согнута
			const FVector Top = H[0].Z > H[1].Z ? H[0] : H[1];
			P.PelvisOffset = HangBodyOffset(R, FVector(Mid.X, 0.f, Top.Z), Wall, 22.f * K, R.ArmLen * 0.95f) + FVector(0.f, 0.f, 2.f * K * Bob);
			P.PelvisPitch = -0.1f; P.PelvisRoll = 0.06f * FMath::Sin(Tau * U); P.SpineRoll = -0.08f * FMath::Sin(Tau * U);
			P.SpinePitch = 0.05f; P.HeadPitch = -0.35f;
			HandsOn(P, H, 1.f, -50.f);
			for (int32 S = 0; S < 2; ++S) { P.HandPitch[S] = 0.4f; }
			break;
		}

		case A_ClimbUp: case A_Mantle:
		{
			// подтягивание из виса / заскок на низкий уступ: руки держат кромку (точки из контекста), тело
			// поднимается над руками, одна нога встаёт на кромку, затем вторая, и персонаж выпрямляется
			const bool bHang = A == A_ClimbUp;
			const float H = C.Ledge.Z, X = C.Ledge.X;
			// руки отпускают кромку и по времени, и когда она опустилась ниже пояса (дальше выпрямленных рук)
			const float HandW = (bHang ? 1.f - Bell(U, .55f, .75f) : Bell(U, 0.f, .15f) * (1.f - Bell(U, .45f, .65f)))
				* Saturate((H - 20.f * K) / (40.f * K));
			const FVector Mid = (C.Hand[0] + C.Hand[1]) * 0.5f;
			const FVector HangOff = HangBodyOffset(R, Mid, X, 18.f * K, R.ArmLen * 0.97f);
			const FVector Crouch(8.f * K, 0.f, -34.f * K);
			const float Up = bHang ? Bell(U, .05f, .5f) : Bell(U, .05f, .35f);
			const float Stand = Bell(U, bHang ? .7f : .6f, 1.f);
			const FVector Start = bHang ? HangOff : FVector(0.f, 0.f, -12.f * K * Window(U, 0.f, .1f, .12f, .3f));
			P.PelvisOffset = FMath::Lerp(FMath::Lerp(Start, Crouch, Up), FVector::ZeroVector, Stand);
			const float Lean = Window(U, .1f, .4f, .65f, .95f);
			P.PelvisPitch = 0.35f * Lean; P.SpinePitch = 0.65f * Lean - (bHang ? 0.08f : 0.f) * (1.f - Up);
			P.HeadPitch = -0.35f * (1.f - Up) - 0.25f * Lean;
			// локти: в висе — наружу-назад вниз, когда кромка ниже плеч — назад-вверх (упор)
			HandsOn(P, C.Hand, HandW, FMath::Lerp(-40.f, 60.f, Up));
			if (bHang) { DangleLegs(R, P, 0.f, 0.6f * Window(U, .05f, .3f, .4f, .55f)); }
			// шаг на кромку: правая нога, затем левая; пока стопа стоит, она неподвижна относительно кромки
			const float StepR = Bell(U, bHang ? .3f : .1f, bHang ? .52f : .4f);
			const float StepL = Bell(U, bHang ? .5f : .3f, bHang ? .7f : .52f);
			const float Final = Bell(U, .8f, 1.f);
			for (int32 S = 0; S < 2; ++S)
			{
				const float W = S == 1 ? StepR : StepL;
				// на кромке; когда капсула уже над уступом (H < 0 — корень выше кромки), стопа всё равно стоит на кромке
				const FVector OnLedge(X + (S == 1 ? 14.f : 6.f) * K, SideSign(S) * 11.f * K, FMath::Max(H, -40.f * K) + R.FootH);
				const FVector Rest(0.f, SideSign(S) * 10.f * K, R.FootH);
				P.Foot[S] = FMath::Lerp(FMath::Lerp(P.Foot[S], OnLedge, W), Rest, Final);
				P.FootPitch[S] = FMath::Lerp(P.FootPitch[S], 0.f, FMath::Max(W, Final));
				// колено на шаге: pole перпендикулярен линии бедро–стопа в плоскости XZ и смотрит вперёд-вверх
				// (неподвижный pole оказывался на этой линии и переворачивал колено)
				const FVector Hip = R.RestPos[ThighBone(S)] + P.PelvisOffset;
				const FVector Tgt = FMath::Lerp(P.Foot[S], OnLedge, W);
				const FVector Dl = Tgt - Hip;
				const FVector Perp = FVector(-Dl.Z, 0.f, Dl.X).GetSafeNormal();
				const FVector StepPole = (Hip + Tgt) * 0.5f + (Perp.IsNearlyZero() ? FVector(1.f, 0.f, 0.f) : Perp) * 100.f
					+ FVector(0.f, SideSign(S) * 10.f * K, 0.f);
				const FVector RestPole(150.f, SideSign(S) * 12.f * K, 50.f * K);
				P.KneePole[S] = FMath::Lerp(StepPole, RestPole, Final);
			}
			break;
		}

		case A_Vault:
		{
			// перемах опорой на левую руку: путь капсулы идёт дугой над препятствием (ноги выше кромки),
			// поэтому таз опускается к кромке, ноги поджаты и уходят вправо, корпус кренится к опоре
			// опора на подходе, пока кромка ещё досягаема; затем тело переносится над препятствием
			const float Plant = Window(U, .04f, .15f, .32f, .5f);
			const float Tuck = Window(U, .12f, .34f, .58f, .85f);
			const float PelvisZ0 = R.RestPos[B_Pelvis].Z;
			const float Low = FMath::Clamp(C.Ledge.Z + 10.f * K - PelvisZ0, -85.f * K, 0.f);
			const float Down = FMath::Max(Tuck, Plant);
			P.PelvisOffset = FVector(0.f, 6.f * K * Tuck, Low * Down);
			const float PelvisZ = PelvisZ0 + P.PelvisOffset.Z;
			P.HandIK[0] = Plant;
			P.Hand[0] = C.Ledge + FVector(2.f, -12.f * K, 3.f);
			P.ElbowPole[0] = C.Ledge + FVector(-60.f, -80.f, 60.f);
			P.HandPitch[0] = 0.9f * Plant;
			P.ArmOut[0] = FMath::Lerp(1.32f, 0.9f, Bell(U, 0.f, .15f)); P.ArmSwing[0] = 0.9f * Bell(U, 0.f, .15f);
			P.ArmOut[1] = FMath::Lerp(1.32f, 0.55f, Tuck); P.ArmSwing[1] = 0.4f * Tuck; P.Elbow[1] = 0.5f;
			P.PelvisRoll = -0.4f * Tuck; P.PelvisYaw = 0.25f * Tuck; P.PelvisPitch = 0.15f * Tuck;
			P.SpinePitch = 0.3f * Tuck + 0.35f * Plant; P.SpineRoll = -0.1f * Tuck; P.HeadPitch = -0.15f * Tuck;
			for (int32 S = 0; S < 2; ++S)
			{
				const float Y = SideSign(S);
				const float Fz = FMath::Max(R.FootH, PelvisZ - (S ? 10.f : 18.f) * K);
				P.Foot[S] = FMath::Lerp(P.Foot[S], FVector((14.f + (S ? 8.f : -4.f)) * K, (Y * 10.f + 30.f) * K, Fz), Tuck);
				P.KneePole[S] = FVector(150.f, (Y * 12.f + 70.f * Tuck) * K, PelvisZ);
				P.FootPitch[S] = 0.4f * Tuck;
			}
			break;
		}

		case A_WallRun:
		{
			// забег на стену: стопы толкаются от стены, руки работают, взгляд вверх
			const float Wall = C.WallX;
			for (int32 S = 0; S < 2; ++S)
			{
				const float Ph = Detail::Frac(U + (S ? .5f : 0.f));
				float Fx, Fz;
				if (Ph < .45f) { const float T = Ph / .45f; Fx = Wall - 14.f * K; Fz = FMath::Lerp(72.f, 12.f, T); }
				else { const float T = (Ph - .45f) / .55f; Fx = Wall - (14.f + 26.f * FMath::Sin(PI * T)) * K; Fz = FMath::Lerp(12.f, 72.f, Smooth01(T)); }
				P.Foot[S] = FVector(Fx, SideSign(S) * 11.f * K, R.FootH + Fz * K);
				P.FootPitch[S] = -0.6f;   // носок вверх: стопа упирается в стену
				P.KneePole[S] = FVector(Wall + 100.f, SideSign(S) * 15.f * K, 120.f * K);
				const float Swing = FMath::Cos(Tau * Ph);
				P.ArmOut[S] = 0.7f; P.ArmSwing[S] = 1.1f + 0.7f * Swing; P.Elbow[S] = 1.1f + 0.3f * FMath::Max(0.f, -Swing);
			}
			P.PelvisOffset = FVector(14.f * K, 0.f, 4.f * K * FMath::Cos(2.f * Tau * U));
			P.PelvisPitch = -0.12f; P.SpinePitch = -0.1f; P.HeadPitch = -0.55f;
			break;
		}

		case A_Roll:
		{
			// кувырок вперёд через плечи: таз опускается, всё тело поворачивается на полный оборот
			const float Th = Tau * Smooth01((U - .1f) / .78f);
			const float Dip = Bell(U, 0.f, .15f) * (1.f - Bell(U, .8f, 1.f));
			const float Curl = Window(U, .05f, .2f, .75f, .92f);
			// таз выше, пока тело стоит «на голове» (Th ≈ 90°), и ниже всего на спине: голова не уходит в пол
			const float PelvisZ = FMath::Lerp(R.RestPos[B_Pelvis].Z, (48.f + 34.f * FMath::Abs(FMath::Sin(Th))) * K, Dip);
			P.PelvisOffset = FVector(10.f * K * Dip, 0.f, PelvisZ - R.RestPos[B_Pelvis].Z);
			// угол держим в (-π, π]: в конце кувырка он снова 0, и переход к следующей позе не крутит тело
			P.PelvisPitch = (Th > PI ? Th - Tau : Th) + 0.3f * Dip;
			P.SpinePitch = 0.9f * Curl + 0.3f * Dip; P.NeckPitch = 0.3f * Curl; P.HeadPitch = 0.5f * Curl;
			const FVector Pelvis = R.RestPos[B_Pelvis] + P.PelvisOffset;
			const FQuat Rot = PitchQ(Th);
			for (int32 S = 0; S < 2; ++S)
			{
				const float Y = SideSign(S);
				FVector Rolled = Pelvis + Rot.RotateVector(FVector(22.f * K, Y * 11.f * K, -36.f * K));
				Rolled.Z = FMath::Max(Rolled.Z, R.FootH);
				P.Foot[S] = FMath::Lerp(P.Foot[S] + FVector((S ? 8.f : -6.f) * K * Dip, 0.f, 0.f), Rolled, Curl);
				P.KneePole[S] = FMath::Lerp(P.KneePole[S], Pelvis + Rot.RotateVector(FVector(120.f, Y * 12.f * K, 0.f)), Curl);
				P.FootPitch[S] = 0.5f * Curl;
				// руки: ладони к земле перед собой, затем прижаты к груди
				const float Touch = Window(U, .06f, .17f, .24f, .34f);
				P.HandIK[S] = Touch;
				P.Hand[S] = FVector(36.f * K, Y * 18.f * K, 14.f);
				P.ElbowPole[S] = FVector(-50.f, Y * 80.f, 60.f);
				P.ArmOut[S] = FMath::Lerp(1.32f, 1.f, Curl); P.ArmSwing[S] = 1.f * Curl; P.Elbow[S] = FMath::Lerp(0.18f, 1.6f, Curl);
			}
			break;
		}

		case A_Turn:
		{
			// разворот на месте: таз ведёт, приседание и руки в стороны для равновесия
			const float S1 = FMath::Sin(PI * U);
			P.PelvisOffset = FVector(0.f, 0.f, -12.f * K * S1);
			P.PelvisYaw = 0.35f * S1; P.SpineYaw = 0.25f * S1; P.HeadYaw = 0.3f * S1;
			P.PelvisPitch = 0.1f * S1; P.SpinePitch = 0.15f * S1;
			for (int32 S = 0; S < 2; ++S) { P.ArmOut[S] = FMath::Lerp(1.32f, 0.95f, S1); P.Elbow[S] = 0.4f; }
			break;
		}

		case A_Drop:
		{
			// спуск в вис: присесть на краю, опереться руками, соскользнуть и повиснуть
			const FTPLimbPose Stand = P;
			FTPLimbPose Low;
			MakeRestPose(R, Low);
			Low.PelvisOffset = FVector(-6.f * K, 0.f, -40.f * K);
			Low.PelvisPitch = 0.45f; Low.SpinePitch = 0.6f; Low.HeadPitch = -0.4f;
			for (int32 S = 0; S < 2; ++S)
			{
				Low.Foot[S] = FVector((S ? 8.f : -4.f) * K, SideSign(S) * 14.f * K, R.FootH);
				Low.ArmOut[S] = 1.2f; Low.ArmSwing[S] = 0.7f; Low.Elbow[S] = 0.5f;
			}
			FTPLimbPose Hang;
			EvaluateAction(R, A_Hang, 0.f, C, Hang);
			Hang.HandIK[0] = Hang.HandIK[1] = Bell(U, .4f, .8f);
			P = LerpLimbPose(LerpLimbPose(Stand, Low, Bell(U, 0.f, .3f)), Hang, Bell(U, .35f, .85f));
			break;
		}

		case A_Hop:
		{
			// перескок между уступами: руки тянутся к следующему уступу, ноги поджаты в полёте
			EvaluateAction(R, A_Hang, 0.f, C, P);
			const float Grab = Bell(U, .55f, .85f);
			for (int32 S = 0; S < 2; ++S)
			{
				P.HandIK[S] = Grab;
				P.ArmOut[S] = -0.15f; P.ArmSwing[S] = 0.35f; P.Elbow[S] = 0.2f; P.FingerCurl[S] = FMath::Lerp(0.2f, 1.3f, Grab);
			}
			DangleLegs(R, P, 0.f, 0.6f * FMath::Sin(PI * U));
			break;
		}

		case A_LandHard:
			EvaluatePose(R, P_LandHard, U, P);
			break;

		// ---------------- бой (шпага в правой руке) ----------------
		case A_Guard:
			GuardUpper(R, P, FMath::Sin(Tau * U));
			break;

		case A_Attack1: case A_Attack2: case A_Attack3: case A_Strike:
		{
			// удар: стойка → замах → удар → стойка. 1 — наискось сверху справа, 2 — обратный слева направо,
			// 3 — тяжёлый сверху с выпадом, Strike (стража) — из замаха сверху вниз
			FTPLimbPose Keys[4];
			Keys[0] = P;
			GuardUpper(R, Keys[0], 0.f);
			Keys[1] = Keys[0]; Keys[2] = Keys[0]; Keys[3] = Keys[0];
			float T[4] = { 0.f, .26f, .58f, 1.f };
			FTPLimbPose& W = Keys[1];
			FTPLimbPose& Hit = Keys[2];
			if (A == A_Attack1 || (A == A_Strike && C.AttackSide > 0.5f))
			{
				W.Hand[1] = FVector(4.f * K, 34.f * K, 170.f * K); W.ElbowPole[1] = FVector(-40.f, 100.f, 170.f * K);
				W.SpineYaw = 0.55f; W.HeadYaw = -0.45f; W.SpinePitch = -0.05f;
				Hit.Hand[1] = FVector(44.f * K, -18.f * K, 100.f * K); Hit.ElbowPole[1] = FVector(-10.f, 70.f, 60.f * K);
				Hit.SpineYaw = -0.45f; Hit.HeadYaw = 0.3f; Hit.SpinePitch = 0.25f;
			}
			else if (A == A_Attack2)
			{
				W.Hand[1] = FVector(24.f * K, -26.f * K, 138.f * K); W.ElbowPole[1] = FVector(70.f, 0.f, 190.f * K);
				W.SpineYaw = -0.5f; W.HeadYaw = 0.4f;
				Hit.Hand[1] = FVector(40.f * K, 42.f * K, 114.f * K); Hit.ElbowPole[1] = FVector(-40.f, 110.f, 80.f * K);
				Hit.SpineYaw = 0.45f; Hit.HeadYaw = -0.35f; Hit.SpinePitch = 0.15f;
			}
			else
			{
				// сверху вниз (тяжёлый удар героя и удар стражи из замаха)
				W.Hand[1] = FVector(-6.f * K, 22.f * K, 188.f * K); W.ElbowPole[1] = FVector(-40.f, 90.f, 220.f * K);
				W.SpinePitch = -0.18f; W.SpineYaw = 0.2f; W.HeadPitch = 0.1f;
				W.ArmOut[0] = 0.5f; W.ArmSwing[0] = -0.6f; W.Elbow[0] = 0.6f;
				Hit.Hand[1] = FVector(52.f * K, 6.f * K, 102.f * K); Hit.ElbowPole[1] = FVector(-30.f, 80.f, 80.f * K);
				Hit.SpinePitch = 0.45f; Hit.SpineYaw = -0.2f; Hit.HeadPitch = -0.3f;
				Hit.ArmOut[0] = 0.9f; Hit.ArmSwing[0] = -0.9f; Hit.Elbow[0] = 0.3f;
				if (A == A_Attack3) { T[1] = .34f; T[2] = .66f; }
				if (A == A_Strike) { Keys[0] = W; T[1] = .02f; T[2] = .5f; }   // замах уже сделан (A_WindUp)
			}
			P = KeyLerp(Keys, T, 4, U);
			break;
		}

		case A_Counter:
		{
			// контратака: отвести клинок (парирование), шаг вперёд и укол на выпаде
			FTPLimbPose Keys[4];
			Keys[0] = P;
			GuardUpper(R, Keys[0], 0.f);
			Keys[1] = Keys[0];
			Keys[1].Hand[1] = FVector(30.f * K, -4.f * K, 150.f * K); Keys[1].ElbowPole[1] = FVector(40.f, 100.f, 200.f * K);
			Keys[1].SpineYaw = -0.2f; Keys[1].SpinePitch = -0.05f;
			Keys[2] = Keys[0];
			Keys[2].Hand[1] = FVector(62.f * K, 6.f * K, 124.f * K); Keys[2].ElbowPole[1] = FVector(-20.f, 90.f, 100.f * K);
			Keys[2].SpineYaw = 0.1f; Keys[2].SpinePitch = 0.3f; Keys[2].HeadPitch = -0.25f; Keys[2].HeadYaw = -0.1f;
			Keys[2].PelvisOffset = FVector(12.f * K, 0.f, -14.f * K); Keys[2].PelvisPitch = 0.15f;
			Keys[2].Foot[1] = FVector(34.f * K, 12.f * K, R.FootH); Keys[2].Foot[0] = FVector(-24.f * K, -12.f * K, R.FootH);
			Keys[2].FootYaw[0] = -0.4f;
			Keys[2].ArmOut[0] = 0.8f; Keys[2].ArmSwing[0] = -0.9f; Keys[2].Elbow[0] = 0.4f;
			Keys[3] = Keys[0];
			const float T[4] = { 0.f, .18f, .45f, 1.f };
			P = KeyLerp(Keys, T, 4, U);
			break;
		}

		case A_Block:
		{
			// блок: клинок поперёк над головой
			const float Sh = FMath::Sin(Tau * U);
			GuardUpper(R, P, Sh);
			P.Hand[1] = FVector(26.f * K, 4.f * K, 152.f * K + Sh); P.ElbowPole[1] = FVector(0.f, 110.f, 170.f * K);
			P.HandRoll[1] = 1.2f; P.SpineYaw = 0.1f; P.SpinePitch = -0.04f; P.HeadPitch = 0.05f;
			P.ArmOut[0] = 0.9f; P.ArmSwing[0] = 0.6f; P.Elbow[0] = 1.2f;
			break;
		}

		case A_Dodge:
		{
			// уклонение: короткий низкий отскок в сторону DodgeDir с приседанием на толчке и приземлении
			const FVector D = C.DodgeDir.GetSafeNormal();
			const float Dip = Window(U, 0.f, .12f, .2f, .35f) + Window(U, .55f, .7f, .78f, 1.f);
			const float Air = Window(U, .15f, .3f, .5f, .65f);
			P.PelvisOffset = FVector(-D.X * 6.f * K, -D.Y * 6.f * K, -20.f * K * Dip);
			P.PelvisPitch = 0.25f * Dip; P.SpinePitch = 0.3f * (Dip + Air) * 0.7f; P.SpineRoll = 0.3f * D.Y * Air;
			P.HeadPitch = -0.2f * (Dip + Air);
			for (int32 S = 0; S < 2; ++S)
			{
				P.Foot[S] = FVector((S ? 6.f : -6.f) * K, SideSign(S) * 13.f * K, R.FootH + 14.f * K * Air);
				P.FootPitch[S] = 0.35f * Air;
				P.ArmOut[S] = FMath::Lerp(1.32f, 0.85f, Air); P.ArmSwing[S] = 0.3f; P.Elbow[S] = 0.9f;
			}
			GuardUpper(R, P, 0.f);
			P.SpinePitch += 0.2f * (Dip + Air);
			break;
		}

		case A_WindUp:
		{
			// замах стражи: клинок поднят над правым плечом (окно контратаки)
			FTPLimbPose G = P, W;
			GuardUpper(R, G, 0.f);
			W = G;
			W.Hand[1] = FVector(-6.f * K, 22.f * K, 188.f * K); W.ElbowPole[1] = FVector(-40.f, 90.f, 220.f * K);
			W.SpinePitch = -0.18f; W.SpineYaw = 0.2f; W.HeadPitch = 0.1f;
			W.ArmOut[0] = 0.5f; W.ArmSwing[0] = -0.6f; W.Elbow[0] = 0.6f;
			W.Hand[1].Z += 1.5f * FMath::Sin(Tau * 3.f * U) * Bell(U, .4f, .5f);   // дрожь напряжения
			P = LerpLimbPose(G, W, Bell(U, 0.f, .4f));
			break;
		}

		case A_HitReact:
		{
			// пропущенный удар: корпус отбрасывает назад, руки разлетаются, затем стойка
			GuardUpper(R, P, 0.f);
			const float Hit = Bell(U, 0.f, .2f) * (1.f - Bell(U, .35f, 1.f));
			P.SpinePitch = FMath::Lerp(P.SpinePitch, -0.35f, Hit); P.HeadPitch = FMath::Lerp(P.HeadPitch, -0.3f, Hit);
			P.SpineYaw = FMath::Lerp(P.SpineYaw, -0.1f, Hit);
			P.HandIK[1] = 1.f - 0.6f * Hit;
			P.ArmOut[1] = 0.9f; P.ArmSwing[1] = -0.2f; P.Elbow[1] = 0.5f;
			P.ArmOut[0] = FMath::Lerp(P.ArmOut[0], 0.7f, Hit); P.ArmSwing[0] = FMath::Lerp(P.ArmSwing[0], -0.2f, Hit);
			break;
		}

		case A_Stagger:
		{
			// оглушение: шаг назад, колени согнуты, руки в стороны
			const float S1 = Bell(U, 0.f, .25f) * (1.f - Bell(U, .6f, 1.f));
			const float Wob = FMath::Sin(Tau * 1.5f * U);
			P.PelvisOffset = FVector(-10.f * K * S1, 3.f * K * Wob * S1, -14.f * K * S1);
			P.PelvisPitch = -0.1f * S1; P.SpinePitch = 0.25f * S1; P.SpineRoll = 0.12f * Wob * S1;
			P.HeadPitch = 0.3f * S1; P.HeadRoll = -0.15f * Wob * S1;
			P.Foot[0] = FVector(-28.f * K * S1, -13.f * K, R.FootH); P.Foot[1] = FVector(6.f * K * S1, 12.f * K, R.FootH);
			for (int32 S = 0; S < 2; ++S) { P.ArmOut[S] = FMath::Lerp(1.32f, 0.7f, S1); P.ArmSwing[S] = 0.3f * S1; P.Elbow[S] = 0.6f; }
			break;
		}

		default:
			break;
		}
	}

	// Доля выполнения по убывающему таймеру (DodgeTime, ReactTime, StaggerTime…): запоминает начальное значение;
	// таймер вырос — значит, действие началось заново. Возвращает 0..1 (0 — таймер не идёт).
	struct FTPTimerTrack
	{
		float Start = 0.f, Last = 0.f;
		float Fraction(float T)
		{
			if (T <= 0.f) { Start = Last = 0.f; return 0.f; }
			if (T > Last + 1e-4f || Start <= 0.f) { Start = T; }
			Last = T;
			return Saturate(1.f - T / FMath::Max(Start, 1e-3f));
		}
	};

	// ---------------- слой действий ----------------
	// Плавно включает действие поверх локомоции. Веса двух масок: нижняя (корень, таз, ноги) и верхняя
	// (позвоночник, голова, руки). Смена действия — перекрёстный переход от последней позы к новой.
	struct FTPActionLayer
	{
		int32 Cur = A_None;      // текущее действие (после окончания — последнее, пока гаснет)
		bool bActive = false;
		float U = 0.f;
		float WLower = 0.f, WUpper = 0.f;   // линейные веса масок 0..1
		float XFade = 1.f, XFadeDur = 0.1f;
		bool bHasLast = false;
		FTPLimbPose From, Last, Pose;

		void Reset() { *this = FTPActionLayer(); }

		static float MoveTo(float V, float Target, float Step) { return V < Target ? FMath::Min(V + Step, Target) : FMath::Max(V - Step, Target); }

		// Action — действие этого кадра (A_None — нет), InU — его доля выполнения или фаза.
		void Update(int32 Action, float InU, float Dt)
		{
			Dt = FMath::Clamp(Dt, 0.f, 0.1f);
			// новое действие, повтор после паузы или перезапуск того же (доля выполнения пошла назад) — переход
			const bool bRestart = Action == Cur && bActive && !ActionLoops(Action) && InU + 0.2f < U;
			if (Action != A_None && (Action != Cur || !bActive || bRestart))
			{
				if (bHasLast && (WLower > 1e-3f || WUpper > 1e-3f))
				{
					From = Last; XFade = 0.f; XFadeDur = ActionBlendTime(Action);
				}
				else { XFade = 1.f; }
				Cur = Action;
			}
			bActive = Action != A_None;
			if (bActive) { U = InU; }
			const float Step = Dt / FMath::Max(ActionBlendTime(Cur), 1e-3f);
			WLower = MoveTo(WLower, bActive && !ActionUpperOnly(Cur) ? 1.f : 0.f, Step);
			WUpper = MoveTo(WUpper, bActive ? 1.f : 0.f, Step);
			if (XFade < 1.f) { XFade = FMath::Min(1.f, XFade + Dt / FMath::Max(XFadeDur, 1e-3f)); }
		}

		// Считает позу действия этого кадра (Pose). Пока действие гаснет — держит последнюю позу.
		void Evaluate(const FTPRig& R, const FTPActionContext& C)
		{
			if (bActive || !bHasLast) { EvaluateAction(R, Cur, U, C, Pose); }
			else { Pose = Last; }
			if (XFade < 1.f) { Pose = LerpLimbPose(From, Pose, Smooth01(XFade)); }
			Last = Pose;
			bHasLast = true;
		}

		float LowerWeight() const { return Smooth01(WLower); }
		float UpperWeight() const { return Smooth01(WUpper); }
		bool IsVisible() const { return WUpper > 1e-3f || WLower > 1e-3f; }

		// Наложить на позу локомоции целями (процедурный путь).
		void BlendInto(FTPLimbPose& Base) const
		{
			Act::LerpLowerInto(Base, Pose, LowerWeight());
			Act::LerpUpperInto(Base, Pose, UpperWeight());
		}
	};

	// Кость в нижней маске (корень, таз, ноги)?
	inline bool IsLowerBone(int32 B)
	{
		return B == B_Root || B == B_Pelvis || (B >= B_ThighL && B <= B_BallR);
	}

	// ---------------- постобработка решённой позы ----------------
	// Положения и повороты 23 костей в пространстве персонажа (компонентные, повёрнутые в оси актёра).
	struct FTPBonesCS
	{
		FVector Pos[B_Count];
		FQuat Rot[B_Count];

		FTPBonesCS()
		{
			for (int32 i = 0; i < B_Count; ++i) { Pos[i] = FVector::ZeroVector; Rot[i] = FQuat::Identity; }
		}

		// Из решённой позы (для тестов и процедурного пути).
		void FromPose(const FTPRig& R, const FTPPoseCS& P)
		{
			for (int32 i = 0; i < B_Count; ++i) { Pos[i] = P.Pos[i]; Rot[i] = P.CharRot(R, i); }
		}
	};

	inline bool IsInSubtree(int32 B, int32 Ancestor)
	{
		for (int32 P = B; P >= 0; P = BoneParent(P)) { if (P == Ancestor) { return true; } }
		return false;
	}

	// Повернуть кость со всеми потомками вокруг её сустава (Q — в пространстве персонажа).
	inline void RotateSubtree(FTPBonesCS& O, int32 Bone, const FQuat& Q)
	{
		const FVector Pivot = O.Pos[Bone];
		for (int32 B = Bone; B < B_Count; ++B)
		{
			if (!IsInSubtree(B, Bone)) { continue; }
			O.Pos[B] = Pivot + Q.RotateVector(O.Pos[B] - Pivot);
			O.Rot[B] = (Q * O.Rot[B]).GetNormalized();
		}
	}

	inline void TranslateSubtree(FTPBonesCS& O, int32 Bone, const FVector& D)
	{
		for (int32 B = Bone; B < B_Count; ++B)
		{
			if (IsInSubtree(B, Bone)) { O.Pos[B] += D; }
		}
	}

	// Ограничение поворота углом MaxAngle (рад).
	inline FQuat ClampAngle(const FQuat& Q, float MaxAngle)
	{
		FQuat N = Q.GetNormalized();
		if (N.W < 0.f) { N = FQuat(-N.X, -N.Y, -N.Z, -N.W); }
		const float Ang = 2.f * FMath::Acos(FMath::Clamp(N.W, -1.f, 1.f));
		if (Ang <= MaxAngle || Ang < 1e-5f) { return N; }
		const FVector Axis = FVector(N.X, N.Y, N.Z).GetSafeNormal();
		return FQuat(Axis, MaxAngle);
	}

	// IK ноги: лодыжка в точку Ankle, колено сгибается в ту же сторону, что и было; стопа сохраняет поворот
	// FootRot (в пространстве персонажа). Длины костей не меняются.
	inline void LegIK(FTPBonesCS& O, int32 Side, const FVector& Ankle, const FQuat& FootRot)
	{
		const int32 BT = ThighBone(Side), BC = CalfBone(Side), BF = FootBone(Side);
		const FVector A = O.Pos[BT], B = O.Pos[BC], C = O.Pos[BF];
		const float L1 = (B - A).Size(), L2 = (C - B).Size();
		if (L1 < 1.f || L2 < 1.f) { return; }
		const FVector Line = (C - A).GetSafeNormal();
		FVector Bend = RejectFrom(B - A, Line);
		const FVector Fwd = O.Rot[B_Root].RotateVector(FVector(1.f, 0.f, 0.f));
		Bend = Bend.SizeSquared() < 1e-4f ? Fwd : Bend.GetSafeNormal();
		FVector NB, NC, NBend;
		SolveTwoBoneIK(A, L1, L2, Ankle, A + Bend * 100.f, Fwd, NB, NC, NBend, 0.02f);
		const FVector Hinge0 = FVector::CrossProduct(Line, Bend).GetSafeNormal();
		const FVector Hinge1 = FVector::CrossProduct((NC - A).GetSafeNormal(), NBend).GetSafeNormal();
		RotateSubtree(O, BT, AimWithHinge((B - A).GetSafeNormal(), Hinge0, (NB - A).GetSafeNormal(), Hinge1));
		const FVector D2 = (O.Pos[BF] - O.Pos[BC]).GetSafeNormal();
		RotateSubtree(O, BC, FQuat::FindBetweenNormals(D2, (NC - O.Pos[BC]).GetSafeNormal()));
		RotateSubtree(O, BF, (FootRot * O.Rot[BF].Inverse()).GetNormalized());
	}

	// Вход IK стоп: на сколько поверхность под стопой выше (+) или ниже (-) пола капсулы и её нормаль.
	struct FTPFootIKInput
	{
		float Offset[2];
		FVector Normal[2];
		float Weight;   // 0 — выключено (полёт, паркур), 1 — полностью
		FTPFootIKInput() { Offset[0] = Offset[1] = 0.f; Normal[0] = Normal[1] = FVector(0.f, 0.f, 1.f); Weight = 0.f; }
	};

	// IK стоп на неровной земле (ступени, мостовая): таз опускается к нижней стопе, каждая лодыжка
	// поднимается или опускается на свою поверхность, стопа ложится по нормали (до 25°).
	// Возвращает, на сколько опущен таз (<= 0).
	inline float ApplyFootIK(FTPBonesCS& O, const FTPFootIKInput& In, float MaxDrop = 40.f)
	{
		const float W = Saturate(In.Weight);
		if (W <= 1e-3f) { return 0.f; }
		const float Off0 = FMath::Clamp(In.Offset[0], -MaxDrop, MaxDrop) * W;
		const float Off1 = FMath::Clamp(In.Offset[1], -MaxDrop, MaxDrop) * W;
		const float Drop = FMath::Clamp(FMath::Min(FMath::Min(Off0, Off1), 0.f), -MaxDrop, 0.f);
		FVector Ankle[2];
		FQuat FootRot[2];
		for (int32 S = 0; S < 2; ++S) { Ankle[S] = O.Pos[FootBone(S)]; FootRot[S] = O.Rot[FootBone(S)]; }
		TranslateSubtree(O, B_Pelvis, FVector(0.f, 0.f, Drop));
		for (int32 S = 0; S < 2; ++S)
		{
			const FVector N = In.Normal[S].GetSafeNormal();
			const FQuat Tilt = N.IsNearlyZero() ? FQuat::Identity
				: ClampAngle(FQuat::Slerp(FQuat::Identity, FQuat::FindBetweenNormals(FVector(0.f, 0.f, 1.f), N), W), 0.44f);
			LegIK(O, S, Ankle[S] + FVector(0.f, 0.f, S == 0 ? Off0 : Off1), (Tilt * FootRot[S]).GetNormalized());
		}
		return Drop;
	}

	// Наклон в повороте на бегу (Lean > 0 — вправо, рад): тело клонится как маятник над стопами,
	// стопы остаются на месте (IK), голова частично выравнивается к горизонту.
	inline void ApplyLean(FTPBonesCS& O, float Lean)
	{
		Lean = FMath::Clamp(Lean, -0.35f, 0.35f);
		if (FMath::Abs(Lean) < 1e-4f) { return; }
		FVector Ankle[2];
		FQuat FootRot[2];
		for (int32 S = 0; S < 2; ++S) { Ankle[S] = O.Pos[FootBone(S)]; FootRot[S] = O.Rot[FootBone(S)]; }
		const FVector Fwd = O.Rot[B_Root].RotateVector(FVector(1.f, 0.f, 0.f));
		const FVector Right = O.Rot[B_Root].RotateVector(FVector(0.f, 1.f, 0.f));
		const float Hgt = O.Pos[B_Pelvis].Z - O.Pos[B_Root].Z;
		// маятник: таз смещается в сторону наклона и чуть опускается
		// таз опускается ещё и на подъём бедра от крена — иначе прямая нога стойки не дотягивается до стопы
		const float Down = Hgt * (1.f - FMath::Cos(Lean)) + 10.f * FMath::Abs(FMath::Sin(Lean * 0.6f));
		TranslateSubtree(O, B_Pelvis, Right * (Hgt * FMath::Sin(Lean) * 0.6f) - FVector(0.f, 0.f, Down));
		// крен вправо — поворот вокруг «вперёд» на отрицательный угол (верх к +Y, как RollQ)
		RotateSubtree(O, B_Pelvis, FQuat(Fwd, -Lean * 0.6f));
		RotateSubtree(O, B_Spine1, FQuat(Fwd, -Lean * 0.3f));
		RotateSubtree(O, B_Neck, FQuat(Fwd, Lean * 0.5f));
		for (int32 S = 0; S < 2; ++S) { LegIK(O, S, Ankle[S], FootRot[S]); }
	}

	// Взгляд: поворот шеи (40 %) и головы (60 %) к направлению Yaw/Pitch (рад, относительно актёра; Pitch > 0 — вниз),
	// отсчитанному от груди; ограничение по рысканию MaxYaw и наклону MaxPitch.
	inline void ApplyLookAt(FTPBonesCS& O, const FTPRig& R, float Yaw, float Pitch, float Weight, float MaxYaw = 1.2f, float MaxPitch = 0.6f)
	{
		const float W = Saturate(Weight);
		if (W <= 1e-3f) { return; }
		const FQuat ChestDelta = (O.Rot[B_Spine3] * R.RestRot[B_Spine3].Inverse()).GetNormalized();
		const FVector F = ChestDelta.RotateVector(FVector(1.f, 0.f, 0.f));
		const float ChestYaw = FMath::Atan2(F.Y, F.X);
		float DY = Yaw - ChestYaw;
		while (DY > PI) { DY -= 2.f * PI; }
		while (DY < -PI) { DY += 2.f * PI; }
		DY = FMath::Clamp(DY, -MaxYaw, MaxYaw) * W;
		const float DP = FMath::Clamp(Pitch, -MaxPitch, MaxPitch) * W;
		const FVector Up(0.f, 0.f, 1.f);
		RotateSubtree(O, B_Neck, FQuat(Up, DY * 0.4f));
		RotateSubtree(O, B_Head, FQuat(Up, DY * 0.6f));
		const FVector HF = (O.Rot[B_Head] * R.RestRot[B_Head].Inverse()).RotateVector(FVector(1.f, 0.f, 0.f));
		const FVector Side = FVector::CrossProduct(Up, FVector(HF.X, HF.Y, 0.f)).GetSafeNormal();
		if (!Side.IsNearlyZero())
		{
			RotateSubtree(O, B_Neck, FQuat(Side, DP * 0.4f));
			RotateSubtree(O, B_Head, FQuat(Side, DP * 0.6f));
		}
	}
}

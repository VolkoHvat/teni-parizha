// Тени Парижа — логика локомоции без UObject (для UTPAnimInstance и тестов вне UE).
// 1) Веса и времена клипов манекена UE4 (стойка, шаг, бег, прыжок) по скорости с общей фазой шага/бега:
//    шаг и бег играются в одной нормированной фазе, поэтому при смешивании ноги не «заплетаются».
// 2) Процедурная походка из TPAnimPoseMath.h — запасной вариант, если клипов нет (и для приседа:
//    в пакете Third Person нет анимаций приседа).
// Скорости — см/с, время — секунды.
#pragma once

#include "CoreMinimal.h"
#include "TPAnimPoseMath.h"

namespace TPAnim
{
	// Клипы локомоции (пакет Third Person UE4: ThirdPersonIdle/Walk/Run/Jump_Start/Jump_Loop/Jump_End).
	enum ELocoClip : int32
	{
		C_Idle, C_Walk, C_Run, C_JumpStart, C_JumpLoop, C_JumpEnd,
		C_Count
	};

	inline float FracPos(float X) { return X - FMath::FloorToFloat(X); }

	// Частота цикла процедурной походки (циклов/с) на скорости Speed: за цикл стопа проходит Stride метров.
	inline float GaitCycleRate(const FTPRig& Rig, int32 Pose, float Speed)
	{
		const float StrideCm = Detail::GaitParams(Pose).Stride * 100.f * Rig.Scale;
		return StrideCm > 1.f ? FMath::Max(Speed, 0.f) / StrideCm : 0.f;
	}

	struct FTPLocoInput
	{
		float Speed = 0.f;           // горизонтальная скорость
		float VerticalSpeed = 0.f;   // + вверх
		bool bInAir = false;
		bool bCrouching = false;
	};

	struct FTPLocoParams
	{
		float WalkAnimSpeed = 150.f;   // скорость, под которую снят клип шага (ThirdPersonWalk)
		float RunAnimSpeed = 375.f;    // скорость клипа бега (ThirdPersonRun)
		float MinPlayRate = 0.6f;      // ниже — ноги «плывут» медленнее, чем идёт тело, но не замирают
		float MaxPlayRate = 1.7f;      // спринт: бег ускоряется, а не растягивается
		float SpeedHalfLife = 0.06f;   // сглаживание скорости (убирает дрожание весов)
		float AirBlendTime = 0.12f;    // вход в полёт и выход из него
		float CrouchHalfLife = 0.08f;
		float LandMinFallSpeed = 350.f;   // с какой скорости падения играть приземление
		float ClipLength[C_Count];

		FTPLocoParams()
		{
			// длины клипов Third Person (подменяются настоящими из UAnimSequence)
			ClipLength[C_Idle] = 2.f; ClipLength[C_Walk] = 1.07f; ClipLength[C_Run] = 0.7f;
			ClipLength[C_JumpStart] = 0.43f; ClipLength[C_JumpLoop] = 1.f; ClipLength[C_JumpEnd] = 0.6f;
		}
		float Len(int32 C) const { return FMath::Max(ClipLength[C], 0.05f); }
	};

	struct FTPLocoOutput
	{
		float Weight[C_Count];   // сумма = 1
		float Time[C_Count];     // время в клипе, 0..длина
		float CrouchWeight = 0.f;   // вес процедурного приседа поверх клипов, 0..1

		FTPLocoOutput()
		{
			for (int32 i = 0; i < C_Count; ++i) { Weight[i] = 0.f; Time[i] = 0.f; }
			Weight[C_Idle] = 1.f;
		}
	};

	// Состояние локомоции одного персонажа. Update — раз в кадр (в рабочем потоке анимации).
	struct FTPLocoState
	{
		float Speed = 0.f, SpeedVel = 0.f;   // сглаженная скорость
		float VerticalSpeed = 0.f;
		float Phase = 0.f;          // общая фаза шага и бега, 0..1
		float IdleTime = 0.f;
		float Air = 0.f;            // вес полёта 0..1 (линейный, сглаживается при выдаче)
		float AirTime = 0.f;        // сколько длится текущий полёт
		bool bJumped = false;       // полёт начался прыжком (вверх), а не сходом с края
		bool bWasInAir = false;
		float LandTime = -1.f;      // время с приземления (<0 — не играется)
		float Crouch = 0.f, CrouchVel = 0.f;
		float ProcPhase = 0.f;      // фаза процедурной походки
		float ProcIdlePhase = 0.f;
		float MaxFallSpeed = 0.f;
		float FallBlend = 0.f, FallBlendVel = 0.f;   // 0 — поза прыжка вверх, 1 — падение

		void Reset() { *this = FTPLocoState(); }

		// Скорость проигрывания клипа шага/бега (без учёта фазы).
		static float ClipRate(const FTPLocoParams& Pm, float S, float AnimSpeed)
		{
			return FMath::Clamp(S / FMath::Max(AnimSpeed, 1.f), Pm.MinPlayRate, Pm.MaxPlayRate);
		}

		// Веса стоя/шаг/бег по скорости (кусочно-линейно, сумма 1).
		static void GroundWeights(const FTPLocoParams& Pm, float S, float& WIdle, float& WWalk, float& WRun)
		{
			const float Wk = FMath::Max(Pm.WalkAnimSpeed, 1.f), Rn = FMath::Max(Pm.RunAnimSpeed, Wk + 1.f);
			WIdle = WWalk = WRun = 0.f;
			if (S <= 0.f) { WIdle = 1.f; }
			else if (S < Wk) { WWalk = S / Wk; WIdle = 1.f - WWalk; }
			else if (S < Rn) { WRun = (S - Wk) / (Rn - Wk); WWalk = 1.f - WRun; }
			else { WRun = 1.f; }
		}

		void Update(const FTPLocoParams& Pm, const FTPLocoInput& In, float Dt)
		{
			Dt = FMath::Clamp(Dt, 0.f, 0.1f);   // после паузы или подвисания — без рывка
			SpringDamp(Speed, SpeedVel, FMath::Max(In.Speed, 0.f), Pm.SpeedHalfLife, Dt);
			if (Speed < 0.5f && In.Speed <= 0.f) { Speed = 0.f; SpeedVel = 0.f; }
			VerticalSpeed = In.VerticalSpeed;

			// общая фаза: скорость фазы — смесь частот шага и бега по их весам
			float WI, WW, WR;
			GroundWeights(Pm, Speed, WI, WW, WR);
			const float RateW = ClipRate(Pm, Speed, Pm.WalkAnimSpeed) / Pm.Len(C_Walk);
			const float RateR = ClipRate(Pm, Speed, Pm.RunAnimSpeed) / Pm.Len(C_Run);
			const float WMove = WW + WR;
			if (WMove > 1e-4f) { Phase = FracPos(Phase + Dt * (WW * RateW + WR * RateR) / WMove); }
			IdleTime = FracPos((IdleTime + Dt) / Pm.Len(C_Idle)) * Pm.Len(C_Idle);

			// полёт
			if (In.bInAir)
			{
				if (!bWasInAir) { AirTime = 0.f; bJumped = In.VerticalSpeed > 150.f; MaxFallSpeed = 0.f; LandTime = -1.f; }
				AirTime += Dt;
				const float FallTarget = Saturate(0.5f - In.VerticalSpeed / 500.f);
				if (AirTime <= Dt) { FallBlend = FallTarget; FallBlendVel = 0.f; }
				SpringDamp(FallBlend, FallBlendVel, FallTarget, 0.1f, Dt);
				FallBlend = Saturate(FallBlend);
				MaxFallSpeed = FMath::Max(MaxFallSpeed, -In.VerticalSpeed);
			}
			else if (bWasInAir)
			{
				LandTime = MaxFallSpeed >= Pm.LandMinFallSpeed ? 0.f : -1.f;
			}
			else if (LandTime >= 0.f)
			{
				LandTime += Dt;
				if (LandTime >= Pm.Len(C_JumpEnd)) { LandTime = -1.f; }
			}
			bWasInAir = In.bInAir;
			const float AirStep = Dt / FMath::Max(Pm.AirBlendTime, 1e-3f);
			Air = FMath::Clamp(Air + (In.bInAir ? AirStep : -AirStep), 0.f, 1.f);

			SpringDamp(Crouch, CrouchVel, In.bCrouching ? 1.f : 0.f, Pm.CrouchHalfLife, Dt);
			Crouch = FMath::Clamp(Crouch, 0.f, 1.f);
		}

		void GetOutput(const FTPLocoParams& Pm, FTPLocoOutput& Out) const
		{
			float WI, WW, WR;
			GroundWeights(Pm, Speed, WI, WW, WR);

			// приземление: клип Jump_End заменяет стойку/шаг и гаснет к концу; на бегу не играется
			float Land = 0.f;
			if (LandTime >= 0.f)
			{
				const float T = LandTime / Pm.Len(C_JumpEnd);
				const float Moving = Saturate(Speed / FMath::Max(Pm.WalkAnimSpeed, 1.f));
				Land = (1.f - Smooth01((T - 0.4f) / 0.6f)) * (1.f - Moving);
			}

			// полёт: Jump_Start (только после прыжка), затем Jump_Loop
			const float A = Smooth01(Air);
			float WStart = 0.f;
			if (bJumped)
			{
				const float LenS = Pm.Len(C_JumpStart), Fade = FMath::Min(0.15f, LenS * 0.5f);
				WStart = 1.f - Smooth01((AirTime - (LenS - Fade)) / Fade);
			}
			const float G = (1.f - A);
			Out.Weight[C_Idle] = G * (1.f - Land) * WI;
			Out.Weight[C_Walk] = G * (1.f - Land) * WW;
			Out.Weight[C_Run] = G * (1.f - Land) * WR;
			Out.Weight[C_JumpEnd] = G * Land;
			Out.Weight[C_JumpStart] = A * WStart;
			Out.Weight[C_JumpLoop] = A * (1.f - WStart);

			Out.Time[C_Idle] = IdleTime;
			Out.Time[C_Walk] = Phase * Pm.Len(C_Walk);
			Out.Time[C_Run] = Phase * Pm.Len(C_Run);
			Out.Time[C_JumpStart] = FMath::Clamp(AirTime, 0.f, Pm.Len(C_JumpStart));
			Out.Time[C_JumpLoop] = FracPos(AirTime / Pm.Len(C_JumpLoop)) * Pm.Len(C_JumpLoop);
			Out.Time[C_JumpEnd] = FMath::Clamp(LandTime, 0.f, Pm.Len(C_JumpEnd));
			Out.CrouchWeight = Crouch * G;
		}

		// ---------------- процедурный запасной вариант ----------------
		// Продвигает фазы процедурной походки. Вызывать вместе с Update, если клипов нет.
		void UpdateProcedural(const FTPRig& Rig, float Dt)
		{
			Dt = FMath::Clamp(Dt, 0.f, 0.1f);
			int32 PA, PB; float T;
			ProcGait(Speed, Crouch > 0.5f, PA, PB, T);
			// у стойки нет шага: пока ноги в паре со стойкой, фаза идёт с частотой второй походки
			const bool bIdleA = PA == P_Idle || PA == P_CrouchIdle;
			const float Rate = FMath::Lerp(GaitCycleRate(Rig, bIdleA ? PB : PA, Speed),
				GaitCycleRate(Rig, PB, Speed), bIdleA ? 1.f : T);
			ProcPhase = FracPos(ProcPhase + Dt * Rate);
			ProcIdlePhase = FracPos(ProcIdlePhase + Dt / 3.5f);   // дыхание стойки — цикл 3.5 с
		}

		// Пара соседних походок и доля второй по скорости.
		static void ProcGait(float S, bool bCrouch, int32& PA, int32& PB, float& T)
		{
			if (bCrouch)
			{
				PA = P_CrouchIdle; PB = P_CrouchWalk; T = Saturate(S / 120.f);
				return;
			}
			if (S < GaitSpeed(1)) { PA = P_Idle; PB = P_Walk; T = S / GaitSpeed(1); }
			else if (S < GaitSpeed(2)) { PA = P_Walk; PB = P_Run; T = (S - GaitSpeed(1)) / (GaitSpeed(2) - GaitSpeed(1)); }
			else { PA = P_Run; PB = P_Sprint; T = Saturate((S - GaitSpeed(2)) / (GaitSpeed(3) - GaitSpeed(2))); }
		}

		// Цели конечностей наземной группы (стоя или в приседе). Смешивание — по целям (LerpLimbPose), а не по
		// поворотам костей: тогда IK держит стопы на полу и в промежуточных позах (смесь поворотов стоя/присед
		// опускала лодыжку под пол на 4–5 см).
		void GroundLimb(const FTPRig& Rig, bool bCrouch, FTPLimbPose& Out) const
		{
			int32 PA, PB; float T;
			ProcGait(Speed, bCrouch, PA, PB, T);
			const bool bIdleA = PA == P_Idle || PA == P_CrouchIdle;
			EvaluatePose(Rig, PA, bIdleA ? ProcIdlePhase : ProcPhase, Out);
			if (T <= 1e-4f) { return; }
			FTPLimbPose LB;
			EvaluatePose(Rig, PB, ProcPhase, LB);
			Out = LerpLimbPose(Out, LB, T);
		}

		// Решённая наземная поза одной группы (для слоя приседа поверх клипов).
		void SolveGroundGroup(const FTPRig& Rig, bool bCrouch, FTPPoseCS& Out) const
		{
			FTPLimbPose L;
			GroundLimb(Rig, bCrouch, L);
			SolvePose(Rig, L, Out);
		}

		// Цели конечностей полной процедурной позы: земля (стоя/присед) + приземление + полёт.
		void ProceduralLimb(const FTPRig& Rig, FTPLimbPose& Out) const
		{
			if (Crouch < 0.01f) { GroundLimb(Rig, false, Out); }
			else if (Crouch > 0.99f) { GroundLimb(Rig, true, Out); }
			else
			{
				FTPLimbPose Low;
				GroundLimb(Rig, false, Out);
				GroundLimb(Rig, true, Low);
				Out = LerpLimbPose(Out, Low, Crouch);
			}
			if (LandTime >= 0.f)
			{
				const float U = Saturate(LandTime / 0.45f);
				const float W = (1.f - Smooth01((U - 0.5f) / 0.5f)) * (1.f - Saturate(Speed / GaitSpeed(1)));
				if (W > 1e-4f)
				{
					FTPLimbPose LL;
					EvaluatePose(Rig, MaxFallSpeed > 900.f ? P_LandHard : P_LandSoft, U, LL);
					Out = LerpLimbPose(Out, LL, W);
				}
			}
			const float A = Smooth01(Air);
			if (A <= 1e-4f) { return; }
			// в полёте: вверх — поза прыжка, вниз — падение (FallBlend сглажен и после касания не меняется)
			FTPLimbPose LUp, LDown;
			EvaluatePose(Rig, P_JumpAir, FracPos(AirTime / 1.2f), LUp);
			EvaluatePose(Rig, P_Fall, FracPos(AirTime / 0.9f), LDown);
			Out = LerpLimbPose(Out, LerpLimbPose(LUp, LDown, FallBlend), A);
		}

		void SolveProcedural(const FTPRig& Rig, FTPPoseCS& Out) const
		{
			FTPLimbPose L;
			ProceduralLimb(Rig, L);
			SolvePose(Rig, L, Out);
		}
	};
}

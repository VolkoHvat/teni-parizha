// Тени Парижа — тесты математики поз (TPAnimPoseMath.h) вне UE, на рабочей замене CoreMinimal.h (shim/).
// Сборка и запуск: Tools/AnimTest/run.sh
// Режим «--dump файл»: пишет суставы поз для листа PNG (render_sheet.c).
#include "TPAnimPoseMath.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <vector>

using namespace TPAnim;

#include <map>
#include <string>
static int GFails = 0, GChecks = 0;
static std::map<std::string, int> GKinds;
static void Check(bool bOk, const char* What, const char* Pose = "", float U = -1.f, float Val = 0.f)
{
	++GChecks;
	if (!bOk)
	{
		GKinds[std::string(What) + " / " + Pose]++;
		if (++GFails <= 12) { std::printf("  ПРОВАЛ: %s [%s u=%.3f] значение=%.4f\n", What, Pose, U, Val); }
	}
}

static bool Near(const FVector& A, const FVector& B, float Tol) { return (A - B).Size() <= Tol; }
static float QAngle(const FQuat& A, const FQuat& B)   // угол между поворотами, рад
{
	float D = std::fabs(A.X * B.X + A.Y * B.Y + A.Z * B.Z + A.W * B.W);
	if (D > 1.f) { D = 1.f; }
	return 2.f * std::acos(D);
}
static bool PoseFinite(const FTPPoseCS& O)
{
	for (int i = 0; i < B_Count; ++i)
	{
		if (O.Pos[i].ContainsNaN()) { return false; }
		const FQuat& Q = O.Delta[i];
		if (!(std::isfinite(Q.X) && std::isfinite(Q.Y) && std::isfinite(Q.Z) && std::isfinite(Q.W))) { return false; }
		if (std::fabs(Q.SizeSquared() - 1.f) > 1e-3f) { return false; }
	}
	return true;
}
static char GName[64];
static const char* PN(int P) { std::snprintf(GName, sizeof GName, "%s", PoseName(P)); return GName; }

// 1. Соглашения об осях — те же, что заложены в комментарии заголовка.
static void TestConventions()
{
	std::printf("1. оси и повороты\n");
	Check(Near(PitchQ(HALF_PI).RotateVector(FVector(0, 0, 1)), FVector(1, 0, 0), 1e-5f), "Pitch>0: верх уходит вперёд");
	Check(Near(PitchQ(HALF_PI).RotateVector(FVector(1, 0, 0)), FVector(0, 0, -1), 1e-5f), "Pitch>0: носок вниз");
	Check(Near(YawQ(HALF_PI).RotateVector(FVector(1, 0, 0)), FVector(0, 1, 0), 1e-5f), "Yaw>0: вправо");
	Check(Near(RollQ(HALF_PI).RotateVector(FVector(0, 0, 1)), FVector(0, 1, 0), 1e-5f), "Roll>0: крен вправо");
	Check(Near(MeshToCharacter().RotateVector(FVector(0, 1, 0)), FVector(1, 0, 0), 1e-5f), "меш +Y → персонаж +X");
	Check(Near(MeshToCharacter().RotateVector(FVector(1, 0, 0)), FVector(0, -1, 0), 1e-5f), "меш +X → левая сторона");
	// A*B: сначала B, потом A
	const FQuat AB = YawQ(HALF_PI) * PitchQ(HALF_PI);
	Check(Near(AB.RotateVector(FVector(0, 0, 1)), FVector(0, 1, 0), 1e-5f), "порядок умножения");
	// AimWithHinge: кость и шарнир попадают куда нужно
	for (int k = 0; k < 200; ++k)
	{
		auto Rnd = [&](int s) { return FVector(std::sin(k * 1.7f + s), std::cos(k * 2.3f + s * 3), std::sin(k * .7f + s * 5)).GetSafeNormal(); };
		const FVector Rd = Rnd(1), Nd = Rnd(2);
		const FVector Rh = RejectFrom(Rnd(3), Rd).GetSafeNormal(), Nh = RejectFrom(Rnd(4), Nd).GetSafeNormal();
		const FQuat Q = AimWithHinge(Rd, Rh, Nd, Nh);
		Check(Near(Q.RotateVector(Rd), Nd, 1e-4f) && Near(Q.RotateVector(Rh), Nh, 2e-3f), "AimWithHinge", "", k);
	}
}

// 2. Двухзвенный IK отдельно.
static void TestTwoBone()
{
	std::printf("2. двухзвенный IK\n");
	int Worst = 0; float WorstErr = 0;
	for (int k = 0; k < 2000; ++k)
	{
		const FVector A(0, 0, 0);
		const float L1 = 30.f + 15.f * std::sin(k * .37f), L2 = 28.f + 12.f * std::cos(k * .53f);
		const float R = (L1 + L2) * (0.05f + 1.15f * (0.5f + 0.5f * std::sin(k * 1.31f)));
		const FVector Dir = FVector(std::sin(k * .9f), std::cos(k * 1.9f), std::sin(k * 2.9f + 1)).GetSafeNormal();
		const FVector T = Dir * R, Pole = FVector(std::cos(k * .2f), std::sin(k * .4f), std::cos(k * .6f)) * 100.f;
		FVector B, C, Bend;
		const bool bReach = SolveTwoBoneIK(A, L1, L2, T, Pole, FVector(1, 0, 0), B, C, Bend);
		Check(!B.ContainsNaN() && !C.ContainsNaN(), "IK без NaN", "", k);
		Check(std::fabs((B - A).Size() - L1) < 1e-2f && std::fabs((C - B).Size() - L2) < 1e-2f, "IK длины", "", k);
		const float Err = (C - T).Size();
		if (bReach) { Check(Err < 0.05f, "IK попадает в досягаемую цель", "", k, Err); if (Err > WorstErr) { WorstErr = Err; Worst = k; } }
		// сустав выгибается в сторону pole
		const FVector Line = (C - A).GetSafeNormal();
		const FVector PoleSide = RejectFrom(Pole - A, Line);
		if (PoleSide.Size() > 1.f && (B - A - Line * FVector::DotProduct(B - A, Line)).Size() > 0.5f)
		{
			Check(FVector::DotProduct(B - A, PoleSide) > 0.f, "сустав в сторону pole", "", k);
		}
	}
	std::printf("   худшая ошибка досягаемой цели %.4f см (случай %d)\n", WorstErr, Worst);
}

// Проверки одной решённой позы.
static void CheckPose(const FTPRig& Rig, const FTPLimbPose& P, const FTPPoseCS& O, int Pose, float U)
{
	const char* N = PN(Pose);
	Check(PoseFinite(O), "нет NaN/Inf, кватернионы единичные", N, U);
	for (int i = 2; i < B_Count; ++i)   // root→pelvis — смещение таза, не кость
	{
		const float L = (O.Pos[i] - O.Pos[BoneParent(i)]).Size(), L0 = Rig.BoneLength(i);
		Check(std::fabs(L - L0) < 0.02f, "длина кости сохраняется", N, U, L - L0);
	}
	const FQuat RootRot = YawQ(P.RootYaw);
	const FVector Fwd = RootRot.RotateVector(FVector(1, 0, 0));
	for (int S = 0; S < 2; ++S)
	{
		// колено вперёд (в сторону pole), если нога согнута
		const FVector A = O.Pos[ThighBone(S)], B = O.Pos[CalfBone(S)], C = O.Pos[FootBone(S)];
		const FVector Line = (C - A).GetSafeNormal();
		const FVector Off = RejectFrom(B - A, Line);
		const FVector PoleW = O.Pos[B_Root] + RootRot.RotateVector(P.KneePole[S]);
		if (Off.Size() > 1.f)
		{
			Check(FVector::DotProduct(Off, RejectFrom(PoleW - A, Line)) > 0.f, "колено гнётся к pole", N, U);
			Check(FVector::DotProduct(Off, Fwd) > -0.2f * Off.Size(), "колено не назад", N, U, FVector::DotProduct(Off.GetSafeNormal(), Fwd));
		}
		if (O.bFootReached[S])
		{
			const FVector T = O.Pos[B_Root] + RootRot.RotateVector(P.Foot[S]);
			Check(Near(O.Pos[FootBone(S)], T, 0.1f), "лодыжка в цели", N, U, (O.Pos[FootBone(S)] - T).Size());
		}
		// локоть назад/наружу от линии плечо–кисть: предплечье сгибается вперёд, не за спину
		const FVector SA = O.Pos[UpperArmBone(S)], SB = O.Pos[LowerArmBone(S)], SC = O.Pos[HandBone(S)];
		const FVector ALine = (SC - SA).GetSafeNormal();
		const FVector EOff = RejectFrom(SB - SA, ALine);
		// (поднятая вперёд рука может держать локоть вниз — тогда вперёд он немного смотрит законно)
		if (EOff.Size() > 2.f && P.HandIK[S] < 0.01f && FVector::DotProduct(EOff.GetSafeNormal(), FVector(0, 0, 1)) > -0.5f)
		{
			Check(FVector::DotProduct(EOff, Fwd) < 0.3f * EOff.Size(), "локоть не вперёд", N, U, FVector::DotProduct(EOff.GetSafeNormal(), Fwd));
		}
		if (P.HandIK[S] > 0.999f && O.bHandReached[S])
		{
			const FVector T = O.Pos[B_Root] + RootRot.RotateVector(P.Hand[S]);
			Check(Near(SC, T, 0.1f), "кисть в цели", N, U, (SC - T).Size());
		}
		// поворот бедра действительно переводит кость покоя в текущую
		const FVector RestD = (Rig.RestPos[CalfBone(S)] - Rig.RestPos[ThighBone(S)]).GetSafeNormal();
		Check(Near(O.Delta[ThighBone(S)].RotateVector(RestD), (B - A).GetSafeNormal(), 1e-3f), "поворот бедра согласован", N, U);
	}
	// стопы над полом (кроме виса и лазания, где ноги в воздухе или на стене)
	if (Pose != P_Hang && Pose != P_Brace && Pose != P_Climb)
	{
		for (int S = 0; S < 2; ++S) { Check(O.Pos[BallBone(S)].Z > -3.f, "носок не под полом", N, U, O.Pos[BallBone(S)].Z); }
	}
}

// 3. Все позы по фазам: NaN, длины, сгибы, цели; 4. нет скачков между соседними кадрами и на стыке цикла.
static void TestPoses(const FTPRig& Rig)
{
	std::printf("3–4. позы (%d) по фазам: NaN, длины костей, сгибы, цели, скачки\n", (int)P_Count);
	const int N = 480;
	for (int Pose = 0; Pose < P_Count; ++Pose)
	{
		std::vector<FTPPoseCS> Fr(N + 1);
		for (int k = 0; k <= N; ++k)
		{
			const float U = (float)k / N;
			FTPLimbPose P; EvaluatePose(Rig, Pose, U, P);
			SolvePose(Rig, P, Fr[k]);
			CheckPose(Rig, P, Fr[k], Pose, U);
		}
		float MaxStep = 0, MaxRot = 0; int AtStep = 0, AtRot = 0, BoneStep = 0, BoneRot = 0;
		for (int k = 1; k <= N; ++k)
		{
			for (int i = 0; i < B_Count; ++i)
			{
				const float D = (Fr[k].Pos[i] - Fr[k - 1].Pos[i]).Size();
				const float A = QAngle(Fr[k].Delta[i], Fr[k - 1].Delta[i]);
				if (D > MaxStep) { MaxStep = D; AtStep = k; BoneStep = i; }
				if (A > MaxRot) { MaxRot = A; AtRot = k; BoneRot = i; }
			}
		}
		// при 480 кадрах на цикл плавное движение сдвигает сустав меньше чем на 2.5 см и поворачивает меньше чем на 6°
		Check(MaxStep < 2.5f, "скачок положения", PN(Pose), (float)AtStep / N, MaxStep);
		Check(MaxRot < 0.105f, "скачок поворота", PN(Pose), (float)AtRot / N, MaxRot);
		float Seam = 0, SeamRot = 0;
		if (PoseLoops(Pose))
		{
			for (int i = 0; i < B_Count; ++i)
			{
				Seam = FMath::Max(Seam, (Fr[0].Pos[i] - Fr[N].Pos[i]).Size());
				SeamRot = FMath::Max(SeamRot, QAngle(Fr[0].Delta[i], Fr[N].Delta[i]));
			}
			Check(Seam < 0.05f && SeamRot < 0.002f, "стык цикла", PN(Pose), 1.f, Seam);
		}
		std::printf("   %-12s шаг ≤ %.2f см (%s), поворот ≤ %.2f° (%s)%s\n", PN(Pose), MaxStep, BoneName(BoneStep), MaxRot * 57.2958f, BoneName(BoneRot),
			PoseLoops(Pose) ? "" : ", не цикл");
	}
}

// 5. Независимость от осей и направлений костей покоя: Т-поза рук и случайные повороты покоя дают те же суставы.
static void TestRestInvariance(const FTPRig& Rig)
{
	std::printf("5. независимость от позы покоя\n");
	FTPRig T = Rig;
	for (int S = 0; S < 2; ++S)
	{
		const float Y = SideSign(S);
		const FVector Sh = T.RestPos[UpperArmBone(S)];
		T.RestPos[LowerArmBone(S)] = Sh + FVector(0, Y * Rig.BoneLength(LowerArmBone(S)), 0);   // Т-поза
		T.RestPos[HandBone(S)] = T.RestPos[LowerArmBone(S)] + FVector(0, Y * Rig.BoneLength(HandBone(S)), 0);
		const FVector Hip = T.RestPos[ThighBone(S)];   // прямая нога
		T.RestPos[CalfBone(S)] = Hip + FVector(0, 0, -Rig.BoneLength(CalfBone(S)));
		T.RestPos[FootBone(S)] = T.RestPos[CalfBone(S)] + FVector(0, 0, -Rig.BoneLength(FootBone(S)));
		T.RestPos[BallBone(S)] = T.RestPos[FootBone(S)] + (Rig.RestPos[BallBone(S)] - Rig.RestPos[FootBone(S)]);
	}
	for (int i = 0; i < B_Count; ++i) { T.RestRot[i] = FQuat(FVector(std::sin(i * 1.3f), std::cos(i * .7f), .5f).GetSafeNormal(), i * .9f); }
	T.Finalize();
	float Worst = 0;
	for (int Pose = 0; Pose < P_Count; ++Pose)
	{
		for (int k = 0; k < 32; ++k)
		{
			const float U = k / 32.f;
			FTPLimbPose P; EvaluatePose(Rig, Pose, U, P);
			FTPPoseCS A, B; SolvePose(Rig, P, A); SolvePose(T, P, B);
			for (int i = 0; i < B_Count; ++i)
			{
				const float D = (A.Pos[i] - B.Pos[i]).Size();
				Worst = FMath::Max(Worst, D);
				Check(D < 0.05f, "те же суставы при другой позе покоя", PN(Pose), U, D);
			}
			// «Компонентный» поворот кости с другим RestRot ведёт себя согласованно: Delta одинаковы по смыслу
			const FVector RD = (T.RestPos[LowerArmBone(0)] - T.RestPos[UpperArmBone(0)]).GetSafeNormal();
			Check(Near(B.Delta[UpperArmBone(0)].RotateVector(RD), (B.Pos[LowerArmBone(0)] - B.Pos[UpperArmBone(0)]).GetSafeNormal(), 1e-3f), "плечо по направлению", PN(Pose), U);
		}
	}
	std::printf("   наибольшее расхождение %.4f см\n", Worst);
}

// 6. Переходы: смешивание поз целями и решённых поз, плавный переход по времени, пружина.
static void TestTransitions(const FTPRig& Rig)
{
	std::printf("6. переходы\n");
	const int Pairs[][2] = { {P_Idle, P_Walk}, {P_Walk, P_Run}, {P_Run, P_Sprint}, {P_Idle, P_CrouchIdle}, {P_Fall, P_LandHard}, {P_JumpAir, P_Hang}, {P_Hang, P_Climb}, {P_Run, P_JumpUp}, {P_Brace, P_Fall} };
	for (const auto& Pr : Pairs)
	{
		FTPLimbPose A, B; EvaluatePose(Rig, Pr[0], .3f, A); EvaluatePose(Rig, Pr[1], .6f, B);
		FTPPoseCS SA, SB; SolvePose(Rig, A, SA); SolvePose(Rig, B, SB);
		FTPPoseCS Prev, PrevB; float MaxStep = 0, MaxStepB = 0;
		for (int k = 0; k <= 200; ++k)
		{
			const float T = k / 200.f;
			FTPPoseCS O; SolvePose(Rig, LerpLimbPose(A, B, T), O);
			CheckPose(Rig, LerpLimbPose(A, B, T), O, Pr[0], T);
			FTPPoseCS OB; BlendPoses(Rig, SA, SB, T, OB);
			Check(PoseFinite(OB), "смесь без NaN", PN(Pr[0]), T);
			for (int i = 2; i < B_Count; ++i) { Check(std::fabs((OB.Pos[i] - OB.Pos[BoneParent(i)]).Size() - Rig.BoneLength(i)) < 0.02f, "смесь сохраняет длины", PN(Pr[0]), T); }
			if (k > 0)
			{
				for (int i = 0; i < B_Count; ++i)
				{
					MaxStep = FMath::Max(MaxStep, (O.Pos[i] - Prev.Pos[i]).Size());
					MaxStepB = FMath::Max(MaxStepB, (OB.Pos[i] - PrevB.Pos[i]).Size());
				}
			}
			Prev = O; PrevB = OB;
		}
		// концы смеси совпадают с исходными позами
		FTPPoseCS O0, O1; BlendPoses(Rig, SA, SB, 0.f, O0); BlendPoses(Rig, SA, SB, 1.f, O1);
		float E0 = 0, E1 = 0; for (int i = 0; i < B_Count; ++i) { E0 = FMath::Max(E0, (O0.Pos[i] - SA.Pos[i]).Size()); E1 = FMath::Max(E1, (O1.Pos[i] - SB.Pos[i]).Size()); }
		Check(E0 < 0.05f && E1 < 0.05f, "концы смеси", PN(Pr[0]), 0, FMath::Max(E0, E1));
		Check(MaxStep < 4.f && MaxStepB < 4.f, "плавность перехода (200 шагов)", PN(Pr[0]), 0, FMath::Max(MaxStep, MaxStepB));
		std::printf("   %-11s → %-11s шаг по целям ≤ %.2f см, по поворотам ≤ %.2f см\n", PN(Pr[0]), PoseName(Pr[1]), MaxStep, MaxStepB);
	}
	// переход по времени: альфа монотонна, достигает 1, без рывка в начале
	{
		FTPLimbPose A, B; EvaluatePose(Rig, P_Run, .1f, A); EvaluatePose(Rig, P_Hang, .1f, B);
		FTPPoseCS SA, SB; SolvePose(Rig, A, SA); SolvePose(Rig, B, SB);
		FTPPoseTransition Tr; Tr.Start(SA, .25f);
		float PrevA = 0; FTPPoseCS Prev = SA, Out; bool bMono = true; float MaxStep = 0;
		for (int f = 0; f < 30; ++f)
		{
			Tr.Apply(Rig, 1.f / 60.f, SB, Out);
			const float Al = Tr.Alpha(); if (Al < PrevA) { bMono = false; } PrevA = Al;
			for (int i = 0; i < B_Count; ++i) { MaxStep = FMath::Max(MaxStep, (Out.Pos[i] - Prev.Pos[i]).Size()); }
			Prev = Out;
		}
		Check(bMono && !Tr.IsActive(), "переход завершён, альфа монотонна");
		float E = 0; for (int i = 0; i < B_Count; ++i) { E = FMath::Max(E, (Out.Pos[i] - SB.Pos[i]).Size()); }
		Check(E < 0.05f, "после перехода — целевая поза", "", 0, E);
		std::printf("   переход бег → вис за 0.25 с: шаг кадра ≤ %.1f см при 60 к/с\n", MaxStep);
	}
	// пружина: сходится, не перелетает, устойчива при большом шаге
	{
		float V = 0, Vel = 0; bool bOver = false;
		for (int f = 0; f < 120; ++f) { SpringDamp(V, Vel, 10.f, .1f, 1.f / 60.f); if (V > 10.001f) { bOver = true; } }
		Check(std::fabs(V - 10.f) < 0.01f && !bOver, "пружина сходится без перелёта", "", 0, V);
		float V2 = 0, Vel2 = 0; for (int f = 0; f < 10; ++f) { SpringDamp(V2, Vel2, 10.f, .05f, .5f); }
		Check(std::isfinite(V2) && std::fabs(V2 - 10.f) < 0.5f, "пружина устойчива при dt=0.5 с", "", 0, V2);
	}
}

// Вывод суставов для листа поз: строки «имя фаза x y z ...» (23 сустава, пространство персонажа).
static void Dump(const FTPRig& Rig, const char* Path)
{
	FILE* F = std::fopen(Path, "w");
	if (!F) { std::printf("не открыть %s\n", Path); std::exit(2); }
	const float Phases[] = { 0.f, .25f, .5f, .75f };
	for (int Pose = 0; Pose < P_Count; ++Pose)
	{
		const int NP = (Pose == P_JumpUp || Pose == P_LandSoft || Pose == P_LandHard) ? 4 : (Pose == P_Walk || Pose == P_Run || Pose == P_Sprint || Pose == P_Climb || Pose == P_CrouchWalk) ? 4 : 1;
		for (int k = 0; k < NP; ++k)
		{
			const float U = (Pose == P_JumpUp || Pose == P_LandSoft || Pose == P_LandHard) ? (k + .5f) / 4.f : Phases[k];
			FTPLimbPose P; EvaluatePose(Rig, Pose, U, P);
			FTPPoseCS O; SolvePose(Rig, P, O);
			std::fprintf(F, "%s %.2f", PoseName(Pose), U);
			for (int i = 0; i < B_Count; ++i) { std::fprintf(F, " %.3f %.3f %.3f", O.Pos[i].X, O.Pos[i].Y, O.Pos[i].Z); }
			std::fprintf(F, "\n");
		}
	}
	std::fclose(F);
}

int main(int argc, char** argv)
{
	const FTPRig Rig = MakeMannequinRig();
	if (argc > 2 && std::strcmp(argv[1], "--dump") == 0) { Dump(Rig, argv[2]); return 0; }
	std::printf("скелет: нога %.1f см, рука %.1f см, лодыжка %.1f см, таз %.1f см, масштаб %.3f\n", Rig.LegLen, Rig.ArmLen, Rig.FootH, Rig.HipH, Rig.Scale);
	TestConventions();
	TestTwoBone();
	TestPoses(Rig);
	TestRestInvariance(Rig);
	TestTransitions(Rig);
	for (const auto& KV : GKinds) { std::printf("  провалов %6d: %s\n", KV.second, KV.first.c_str()); }
	std::printf("проверок: %d, провалов: %d\nИТОГ: %s\n", GChecks, GFails, GFails ? "есть ошибки" : "чисто");
	return GFails ? 1 : 0;
}

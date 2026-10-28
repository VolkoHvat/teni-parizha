// Тени Парижа — тесты локомоции (TPAnimLocomotion.h) вне UE: веса клипов, фазы, процедурный запасной вариант.
// Сборка и запуск: Tools/AnimTest/run.sh. Режим «--dump файл» — кадры сценария для листа PNG.
#include "TPAnimLocomotion.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <map>
#include <string>

using namespace TPAnim;

static int GFails = 0, GChecks = 0;
static std::map<std::string, int> GKinds;
static void Check(bool bOk, const char* What, float T = -1.f, float Val = 0.f)
{
	++GChecks;
	if (!bOk)
	{
		GKinds[What]++;
		if (++GFails <= 15) { std::printf("  ПРОВАЛ: %s [t=%.3f] значение=%.4f\n", What, T, Val); }
	}
}

// Сценарий движения персонажа: скорость с разгоном как у CharacterMovement, прыжки, сход с края, присед.
struct FScenario
{
	float Speed = 0.f, Vz = 0.f, Z = 0.f;
	bool bAir = false, bCrouch = false;
	float TargetSpeed(float T) const
	{
		if (T < 1.f) return 0.f;
		if (T < 3.f) return 150.f;
		if (T < 5.f) return 375.f;
		if (T < 7.f) return 740.f;      // спринт
		if (T < 8.f) return 60.f;
		if (T < 11.f) return 120.f;     // присед
		if (T < 13.f) return 460.f;
		if (T < 14.f) return 0.f;
		return 250.f;
	}
	void Step(float T, float Dt)
	{
		const float Tgt = TargetSpeed(T);
		const float Acc = Tgt > Speed ? 2048.f : 2600.f;   // разгон и торможение (см/с²)
		Speed = Tgt > Speed ? FMath::Min(Tgt, Speed + Acc * Dt) : FMath::Max(Tgt, Speed - Acc * Dt);
		bCrouch = T >= 8.f && T < 11.f;
		// прыжки: 4.0 и 12.0 с; 15.0 с — сход с высоты 4 м
		if (!bAir && ((T >= 4.f && T - Dt < 4.f) || (T >= 12.f && T - Dt < 12.f))) { bAir = true; Vz = 520.f; Z = 0.f; }
		if (!bAir && T >= 15.f && T - Dt < 15.f) { bAir = true; Vz = 0.f; Z = 400.f; }
		if (bAir)
		{
			Vz -= 980.f * Dt; Z += Vz * Dt;
			if (Z <= 0.f) { Z = 0.f; bAir = false; Vz = 0.f; }
		}
	}
	FTPLocoInput Input() const { FTPLocoInput I; I.Speed = Speed; I.VerticalSpeed = bAir ? Vz : 0.f; I.bInAir = bAir; I.bCrouching = bCrouch; return I; }
};

static const float GDuration = 18.f;

// 1. Веса клипов: сумма 1, без отрицательных, без скачков; времена в пределах клипа.
static void TestClipWeights()
{
	std::printf("1. веса и времена клипов\n");
	const float Rates[] = { 30.f, 60.f, 120.f, 240.f };
	float Jumps[4];
	for (int r = 0; r < 4; ++r)
	{
		const float Dt = 1.f / Rates[r];
		FTPLocoParams Pm; FTPLocoState S; FScenario Sc;
		FTPLocoOutput Prev; S.GetOutput(Pm, Prev);
		float MaxJump = 0.f, PrevPhase = 0.f;
		for (float T = 0.f; T < GDuration; T += Dt)
		{
			Sc.Step(T, Dt);
			S.Update(Pm, Sc.Input(), Dt);
			FTPLocoOutput O; S.GetOutput(Pm, O);
			float Sum = 0.f;
			for (int c = 0; c < C_Count; ++c)
			{
				Check(O.Weight[c] >= -1e-6f && O.Weight[c] <= 1.f + 1e-6f && FMath::IsFinite(O.Weight[c]), "вес в 0..1", T, O.Weight[c]);
				Check(O.Time[c] >= 0.f && O.Time[c] <= Pm.Len(c) + 1e-4f, "время в пределах клипа", T, O.Time[c]);
				Sum += O.Weight[c];
				MaxJump = FMath::Max(MaxJump, FMath::Abs(O.Weight[c] - Prev.Weight[c]));
			}
			MaxJump = FMath::Max(MaxJump, FMath::Abs(O.CrouchWeight - Prev.CrouchWeight));
			Check(FMath::Abs(Sum - 1.f) < 1e-4f, "сумма весов 1", T, Sum);
			Check(O.CrouchWeight >= 0.f && O.CrouchWeight <= 1.f, "вес приседа 0..1", T, O.CrouchWeight);
			// фаза шага/бега только растёт (с переходом через 1) и не прыгает
			const float DPh = FracPos(S.Phase - PrevPhase);
			Check(DPh < 0.1f * (Dt * 60.f), "фаза без скачков", T, DPh);
			PrevPhase = S.Phase;
			Prev = O;
		}
		Jumps[r] = MaxJump;
		std::printf("   %3.0f к/с: наибольший шаг веса за кадр %.3f\n", Rates[r], MaxJump);
	}
	// непрерывность (от 60 к/с и выше; при 30 к/с пружина скорости ещё нелинейна): при вдвое меньшем шаге кадра скачок веса тоже почти вдвое меньше (настоящий разрыв не уменьшается)
	for (int r = 2; r < 4; ++r) { Check(Jumps[r] < 0.62f * Jumps[r - 1] + 0.005f, "вес непрерывен (шаг уменьшается с кадром)", Rates[r], Jumps[r] / Jumps[r - 1]); }
	Check(Jumps[1] < 0.4f, "шаг веса при 60 к/с", 60.f, Jumps[1]);
}

// 2. Соответствие скоростей: на скорости клипа он играется с частотой 1 и весом 1.
static void TestRates()
{
	std::printf("2. скорость клипов\n");
	FTPLocoParams Pm;
	const float Speeds[] = { Pm.WalkAnimSpeed, Pm.RunAnimSpeed };
	const int Clips[] = { C_Walk, C_Run };
	for (int k = 0; k < 2; ++k)
	{
		FTPLocoState S; FTPLocoInput In; In.Speed = Speeds[k];
		for (int i = 0; i < 120; ++i) { S.Update(Pm, In, 1.f / 60.f); }   // скорость устоялась
		FTPLocoOutput O; S.GetOutput(Pm, O);
		Check(FMath::Abs(O.Weight[Clips[k]] - 1.f) < 1e-3f, "вес клипа на его скорости", -1.f, O.Weight[Clips[k]]);
		const float P0 = S.Phase;
		for (int i = 0; i < 60; ++i) { S.Update(Pm, In, 1.f / 60.f); }
		const float Cycles = FracPos(S.Phase - P0) + FMath::FloorToFloat(1.f / Pm.Len(Clips[k]));
		const float Expect = 1.f / Pm.Len(Clips[k]);
		std::printf("   %s: %.3f цикла/с (ожидалось %.3f)\n", k ? "бег" : "шаг", Cycles, Expect);
		Check(FMath::Abs(Cycles - Expect) < 0.02f, "частота цикла = 1/длина клипа", -1.f, Cycles);
	}
	// спринт ограничен MaxPlayRate
	FTPLocoState S; FTPLocoInput In; In.Speed = 2000.f;
	for (int i = 0; i < 120; ++i) { S.Update(Pm, In, 1.f / 60.f); }
	const float P0 = S.Phase;
	S.Update(Pm, In, 0.01f);
	const float Rate = FracPos(S.Phase - P0) / 0.01f * Pm.Len(C_Run);
	Check(FMath::Abs(Rate - Pm.MaxPlayRate) < 0.01f, "ускорение бега ограничено", -1.f, Rate);
	// пауза в игре: огромный Dt не ломает фазу
	S.Update(Pm, In, 5.f);
	Check(S.Phase >= 0.f && S.Phase < 1.f && FMath::IsFinite(S.Speed), "большой шаг времени", -1.f, S.Phase);
	// прыжок: Jump_Start только после прыжка, сход с края — сразу Jump_Loop
	{
		FTPLocoState J; FTPLocoInput I; I.bInAir = true; I.VerticalSpeed = -50.f;
		for (int i = 0; i < 30; ++i) { J.Update(Pm, I, 1.f / 60.f); }
		FTPLocoOutput O; J.GetOutput(Pm, O);
		Check(O.Weight[C_JumpStart] < 1e-4f && O.Weight[C_JumpLoop] > 0.99f, "сход с края без Jump_Start", -1.f, O.Weight[C_JumpStart]);
	}
	// мягкое приземление с малой высоты не играет Jump_End
	{
		FTPLocoState J; FTPLocoInput I; I.bInAir = true; I.VerticalSpeed = -200.f;
		for (int i = 0; i < 10; ++i) { J.Update(Pm, I, 1.f / 60.f); }
		I.bInAir = false; I.VerticalSpeed = 0.f;
		J.Update(Pm, I, 1.f / 60.f);
		Check(J.LandTime < 0.f, "низкий соскок без приземления", -1.f, J.LandTime);
	}
}

// 3. Процедурный вариант: нет NaN, длины костей сохраняются, стопы не под полом, суставы без скачков.
static float MaxJointStep = 0.f;
static void TestProcedural(const FTPRig& Rig)
{
	std::printf("3. процедурная локомоция\n");
	const float Dt = 1.f / 240.f;
	FTPLocoParams Pm; FTPLocoState S; FScenario Sc;
	FTPPoseCS Prev; bool bHavePrev = false;
	float MaxLenErr = 0.f, MinFootZ = 1e9f;
	int Frames = 0;
	for (float T = 0.f; T < GDuration; T += Dt)
	{
		Sc.Step(T, Dt);
		S.Update(Pm, Sc.Input(), Dt);
		S.UpdateProcedural(Rig, Dt);
		FTPPoseCS O; S.SolveProcedural(Rig, O);
		++Frames;
		bool bFinite = true;
		for (int i = 0; i < B_Count; ++i)
		{
			bFinite = bFinite && !O.Pos[i].ContainsNaN() && FMath::Abs(O.Delta[i].SizeSquared() - 1.f) < 1e-3f;
			const int P = BoneParent(i);
			if (P >= 0 && i != B_Pelvis)   // таз смещается переносом, а не поворотом — его «длина» меняется по замыслу
			{
				const float Err = FMath::Abs((O.Pos[i] - O.Pos[P]).Size() - Rig.BoneLength(i));
				MaxLenErr = FMath::Max(MaxLenErr, Err);
			}
			if (bHavePrev) { MaxJointStep = FMath::Max(MaxJointStep, (O.Pos[i] - Prev.Pos[i]).Size()); }
		}
		Check(bFinite, "поза без NaN", T);
		if (S.Air < 1e-3f && S.LandTime < 0.f)
		{
			for (int Sd = 0; Sd < 2; ++Sd) { MinFootZ = FMath::Min(MinFootZ, O.Pos[FootBone(Sd)].Z); }
			// колено впереди линии бедро-лодыжка (сгиб в нужную сторону)
			for (int Sd = 0; Sd < 2; ++Sd)
			{
				const FVector H = O.Pos[ThighBone(Sd)], K = O.Pos[CalfBone(Sd)], A = O.Pos[FootBone(Sd)];
				const FVector Dir = (A - H).GetSafeNormal();
				const FVector Off = RejectFrom(K - H, Dir);
				Check(Off.X >= -0.5f, "колено вперёд", T, Off.X);
			}
		}
		Prev = O; bHavePrev = true;
	}
	std::printf("   кадров %d: длины костей ±%.4f см, низ лодыжки %.1f см (покой %.1f), наибольший шаг сустава за 1/240 с %.2f см\n",
		Frames, MaxLenErr, MinFootZ, Rig.FootH, MaxJointStep);
	Check(MaxLenErr < 0.05f, "длины костей", -1.f, MaxLenErr);
	Check(MinFootZ > Rig.FootH - 3.f, "лодыжка не уходит под пол", -1.f, MinFootZ);
	// 6 см за 1/240 с = 14 м/с: быстрее стопа спринтера не движется; больше — разрыв позы
	Check(MaxJointStep < 6.f, "суставы без скачков", -1.f, MaxJointStep);
}

static void Dump(const FTPRig& Rig, const char* Path)
{
	FILE* F = std::fopen(Path, "w");
	if (!F) { std::printf("не открыть %s\n", Path); std::exit(2); }
	// моменты сценария: стойка, шаг, бег, спринт, присед, прыжок, падение с высоты, приземление
	const float Times[] = { 0.5f, 2.2f, 2.45f, 4.6f, 4.75f, 6.2f, 6.32f, 7.6f, 9.5f, 9.8f, 10.6f, 12.1f, 12.4f, 12.75f, 13.1f,
		14.5f, 15.3f, 15.8f, 15.95f, 16.05f, 16.2f, 16.5f };
	const int NT = sizeof(Times) / sizeof(Times[0]);
	const float Dt = 1.f / 120.f;
	FTPLocoParams Pm; FTPLocoState S; FScenario Sc;
	int k = 0;
	for (float T = 0.f; T < GDuration && k < NT; T += Dt)
	{
		Sc.Step(T, Dt);
		S.Update(Pm, Sc.Input(), Dt);
		S.UpdateProcedural(Rig, Dt);
		if (T + 1e-4f < Times[k]) { continue; }
		FTPPoseCS O; S.SolveProcedural(Rig, O);
		const char* Name = Sc.bAir ? (Sc.Vz > 0.f ? "air_up" : "air_down") : S.LandTime >= 0.f ? "land" : S.Crouch > 0.5f ? "crouch" :
			S.Speed < 20.f ? "idle" : S.Speed < 200.f ? "walk" : S.Speed < 500.f ? "run" : "sprint";
		std::fprintf(F, "%s %.2f", Name, T);
		for (int i = 0; i < B_Count; ++i) { std::fprintf(F, " %.3f %.3f %.3f", O.Pos[i].X, O.Pos[i].Y, O.Pos[i].Z); }
		std::fprintf(F, "\n");
		++k;
	}
	std::fclose(F);
}

int main(int argc, char** argv)
{
	const FTPRig Rig = MakeMannequinRig();
	if (argc > 2 && std::strcmp(argv[1], "--dump") == 0) { Dump(Rig, argv[2]); return 0; }
	TestClipWeights();
	TestRates();
	TestProcedural(Rig);
	for (const auto& KV : GKinds) { std::printf("  провалов %6d: %s\n", KV.second, KV.first.c_str()); }
	std::printf("проверок: %d, провалов: %d\nИТОГ: %s\n", GChecks, GFails, GFails ? "есть ошибки" : "чисто");
	return GFails ? 1 : 0;
}

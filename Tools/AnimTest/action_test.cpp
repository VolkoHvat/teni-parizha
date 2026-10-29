// Тени Парижа — тест поз паркура и боя (TPAnimActions.h) без Unreal Engine.
// Сборка и запуск: Tools/AnimTest/run.sh. Режим «--dump файл» — кадры для листа PNG (в мировых осях сцены:
// стена и уступ неподвижны, корень движется по пути капсулы, как в UTPTraversalComponent).
#include "CoreMinimal.h"
#include "TPAnimActions.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <cstdlib>
#include <string>

using namespace TPAnim;

static int GFail = 0;
static long GChecks = 0;
#include <map>
static std::map<std::string, int> GSeen;
static void Check(bool bOk, const char* What, const char* Name, float U, float V)
{
	++GChecks;
	if (bOk) { return; }
	++GFail;
	int& Count = GSeen[std::string(Name) + "|" + What];
	if (++Count <= 2) { std::printf("  ОШИБКА %s: %s, U=%.3f, значение %.3f\n", Name, What, U, V); }
}

static float Sm(float U) { U = FMath::Clamp(U, 0.f, 1.f); return U * U * (3.f - 2.f * U); }
static FVector Staged(const FVector& A, const FVector& B, float U, float ZEnd, float XStart, float Bump)
{
	const float Zu = Sm(U / ZEnd), Xu = Sm((U - XStart) / (1.f - XStart));
	return FVector(A.X + (B.X - A.X) * Xu, A.Y + (B.Y - A.Y) * Xu, A.Z + (B.Z - A.Z) * Zu + Bump * std::sin(PI * U));
}

// Длительность действия, с (как в UTPTraversalComponent / UTPCombatComponent); у циклов — длина цикла.
static float Duration(int32 A)
{
	switch (A)
	{
	case A_Hang: return 1.6f; case A_Climb: return 1.1f; case A_WallRun: return 0.42f; case A_Guard: return 3.f; case A_Block: return 1.5f;
	case A_ClimbUp: return 0.9f; case A_Mantle: return 0.6f; case A_Vault: return 0.6f; case A_Roll: return 0.7f; case A_Turn: return 0.45f;
	case A_Drop: return 0.7f; case A_Hop: return 0.75f; case A_LandHard: return 0.7f;
	case A_Attack1: case A_Attack2: return 0.38f; case A_Attack3: return 0.55f; case A_Counter: return 0.5f; case A_Dodge: return 0.4f;
	case A_WindUp: return 0.85f; case A_Strike: return 0.32f; case A_HitReact: return 0.3f; case A_Stagger: return 0.6f;
	default: return 1.f;
	}
}

// Сцена действия: положение корня в мире и контекст (в осях персонажа) в момент U.
struct FScene { FVector Root; FTPActionContext C; float Shimmy; };
static FScene Scene(int32 A, float U, float Shimmy)
{
	FScene S; S.Root = FVector::ZeroVector; S.Shimmy = Shimmy;
	FTPActionContext& C = S.C;
	C.Reset();
	C.Shimmy = Shimmy;
	const FVector Edge(40.f, 0.f, 207.f);   // кромка в мире; стена — плоскость X = 40
	if (A == A_ClimbUp || A == A_Mantle)
	{
		const bool bHang = A == A_ClimbUp;
		const FVector E = bHang ? Edge : FVector(40.f, 0.f, 150.f);
		S.Root = Staged(bHang ? FVector(0.f, 0.f, 0.f) : FVector::ZeroVector, FVector(90.f, 0.f, E.Z + 2.f), U, bHang ? .65f : .6f, bHang ? .45f : .3f, 10.f);
		C.Ledge = E - S.Root;
		C.Hand[0] = E + FVector(4.f, -22.f, 2.f) - S.Root;
		C.Hand[1] = E + FVector(4.f, 22.f, 2.f) - S.Root;
	}
	else if (A == A_Vault)
	{
		// забор высотой 1 м и толщиной 15 см; дуга над ним — на 25 см выше кромки
		const FVector E(40.f, 0.f, 100.f);
		S.Root = FVector(130.f * Sm(U), 0.f, 125.f * std::sin(PI * U));
		C.Ledge = E - S.Root;
	}
	else if (A == A_Hop)
	{
		// перескок на уступ в 1.2 м правее
		const FVector From(0.f, 0.f, 0.f), To(0.f, 120.f, 0.f);
		S.Root = From + (To - From) * Sm(U) + FVector(0.f, 0.f, 25.f * std::sin(PI * U));
		const FVector E2 = Edge + To;
		C.Ledge = E2 - S.Root;
		C.Hand[0] = E2 + FVector(4.f, -22.f, 2.f) - S.Root;
		C.Hand[1] = E2 + FVector(4.f, 22.f, 2.f) - S.Root;
	}
	else if (A == A_Drop)
	{
		// шаг за край крыши (кромка позади, высота 207 под ногами) и вниз в вис; персонаж разворачивается к стене —
		// для простоты лист показывает конечную ориентацию, корень опускается по пути капсулы
		const FVector RFrom(-10.f, 0.f, 207.f), RTo(0.f, 0.f, 0.f);
		const float Xu = Sm(U / .5f), Zu = Sm((U - .25f) / .75f);
		S.Root = FVector(RFrom.X + (RTo.X - RFrom.X) * Xu, 0.f, RFrom.Z + (RTo.Z - RFrom.Z) * Zu);
		C.Ledge = Edge - S.Root;
		C.Hand[0] = Edge + FVector(4.f, -22.f, 2.f) - S.Root;
		C.Hand[1] = Edge + FVector(4.f, 22.f, 2.f) - S.Root;
	}
	else if (A == A_Climb)
	{
		// ладони на стене: центр капсулы на 40 см от стены, кисти на 105 см выше центра
		C.Hand[0] = FVector(40.f, -22.f, 92.f + 105.f);
		C.Hand[1] = FVector(40.f, 22.f, 92.f + 105.f);
		C.WallX = 40.f;
	}
	else if (A == A_WallRun)
	{
		C.WallX = 45.f;
	}
	return S;
}

static const char* SceneTag(int32 A)
{
	switch (A)
	{
	case A_Hang: case A_ClimbUp: case A_Drop: case A_Climb: return "wall";
	case A_Hop: return "wall2";
	case A_Mantle: return "low";
	case A_Vault: return "fence";
	case A_WallRun: return "wallrun";
	default: return "floor";
	}
}

static bool Finite(const FVector& V) { return std::isfinite(V.X) && std::isfinite(V.Y) && std::isfinite(V.Z); }

// Проверки одной решённой позы.
static void CheckPose(const FTPRig& R, int32 A, float U, const FTPLimbPose& L, const FTPPoseCS& O, const char* Name)
{
	for (int32 b = 0; b < B_Count; ++b)
	{
		Check(Finite(O.Pos[b]), "NaN в суставе", Name, U, (float)b);
		if (b > 0)
		{
			if (b == B_Pelvis) { continue; }   // таз смещается от корня — это не кость
			const float L0 = R.BoneLength(b), L1 = (O.Pos[b] - O.Pos[BoneParent(b)]).Size();
			Check(std::fabs(L0 - L1) < 0.05f, "длина кости", Name, U, b * 1000.f + (L1 - L0));
		}
	}
	for (int32 S = 0; S < 2; ++S)
	{
		// колено вперёд: сгиб ноги в сторону KneePole, а не назад
		const FVector Hip = O.Pos[ThighBone(S)], Knee = O.Pos[CalfBone(S)], Ank = O.Pos[FootBone(S)];
		const FVector Line = (Ank - Hip).GetSafeNormal();
		const FVector Bend = RejectFrom(Knee - Hip, Line);
		const FVector PoleDir = RejectFrom(L.KneePole[S] - Hip, Line);
		if (Bend.Size() > 3.f && PoleDir.Size() > 1.f)
		{
			Check(FVector::DotProduct(Bend, PoleDir) > 0.f, "колено гнётся не к pole", Name, U, (float)S);
		}
		// кисть на цели, когда IK полностью включён и цель досягаема
		if (L.HandIK[S] > 0.99f)
		{
			const float Reach = (L.Hand[S] - O.Pos[UpperArmBone(S)]).Size();
			const float Arm = R.BoneLength(LowerArmBone(S)) + R.BoneLength(HandBone(S));
			const float Err = (O.Pos[HandBone(S)] - L.Hand[S]).Size();
			if (Reach < Arm * 0.95f) { Check(Err < 1.f, "кисть не на цели", Name, U, Err); }
			// недосягаемая цель: кисть на вытянутой руке рядом с ней (касание пальцами — до 13 см; перемах — опора «на лету»)
			else { Check(Err < (A == A_Vault ? 30.f : 13.f), "кисть далеко от цели (недотяг)", Name, U, Err); }
		}
		// лодыжки не ниже пола там, где персонаж стоит на земле
		const bool bGround = A == A_Turn || A == A_LandHard || A == A_Counter || A == A_Stagger || A == A_Dodge || A == A_Roll
			|| A == A_Guard || (A >= A_Attack1 && A <= A_HitReact);
		if (bGround) { Check(Ank.Z > R.FootH * 0.6f, "лодыжка ниже пола", Name, U, Ank.Z); }
	}
}

static void TestActions(const FTPRig& R)
{
	std::printf("позы действий:\n");
	const int N = 480;
	for (int32 A = 1; A < A_Count; ++A)
	{
		const bool bLoop = ActionLoops(A);
		for (int Sh = 0; Sh < (A == A_Hang || A == A_Climb ? 3 : 1); ++Sh)
		{
			const float Shimmy = Sh == 0 ? 0.f : Sh == 1 ? 1.f : -1.f;
			std::string NameS = std::string(ActionName(A)) + (Sh == 1 ? "/вправо" : Sh == 2 ? "/влево" : "");
			const char* Name = NameS.c_str();
			FTPPoseCS Prev, First;
			FVector PrevRoot;
			float MaxV = 0.f, MaxStep = 0.f, MaxU = 0.f; int32 MaxB = 0;
			for (int i = 0; i <= N; ++i)
			{
				const float U = (float)i / N;
				FScene S = Scene(A, U, Shimmy);
				FTPLimbPose L;
				EvaluateAction(R, A, U, S.C, L);
				FTPPoseCS O;
				SolvePose(R, L, O);
				CheckPose(R, A, U, L, O, Name);
				if (i == 0) { First = O; }
				if (i > 0)
				{
					// скорость суставов в мире (с движением корня по пути капсулы)
					for (int32 b = 0; b < B_Count; ++b)
					{
						const float D = ((O.Pos[b] + S.Root) - (Prev.Pos[b] + PrevRoot)).Size();
						if (D > MaxStep) { MaxStep = D; MaxU = U; MaxB = b; }
						MaxV = FMath::Max(MaxV, D / (Duration(A) / N) / 100.f);
					}
				}
				Prev = O; PrevRoot = S.Root;
			}
			if (bLoop)
			{
				float Gap = 0.f;
				for (int32 b = 0; b < B_Count; ++b) { Gap = FMath::Max(Gap, (Prev.Pos[b] - First.Pos[b]).Size()); }
				Check(Gap < 0.5f, "разрыв на стыке цикла", Name, 1.f, Gap);
			}
			// предел скорости сустава: 16 м/с (быстрый удар шпагой — кисть 8–12 м/с)
			// предел 18 м/с (быстрый заскок за 0.6 с — колено до ~17 м/с); удары шпагой быстрее: кисть до ~15 м/с,
			// локоть с поворотом корпуса — до ~22 м/с
			const bool bStrike = A == A_Attack1 || A == A_Attack2 || A == A_Attack3 || A == A_Strike || A == A_Counter;
			Check(MaxV < (bStrike ? 23.f : 18.f), "слишком быстрый сустав (рывок)", Name, MaxU, MaxV);
			std::printf("  %-14s наибольшая скорость сустава %5.2f м/с (кость %d при U=%.3f)\n", Name, MaxV, MaxB, MaxU);
		}
	}
}

// Слой действий: одна и та же случайная последовательность действий (в т. ч. редкие в игре переходы,
// вроде «удар → вис»; со стены на землю — через паузу, как через полёт) при разной частоте кадров. Непрерывность: наибольший сдвиг сустава за кадр должен
// уменьшаться вместе с шагом кадра (разрыв позы от частоты кадров не зависит и сразу виден).
static float RunLayer(const FTPRig& R, float Fps)
{
	std::srand(7);
	FTPActionLayer Layer;
	FTPPoseCS Prev;
	bool bHave = false;
	float MaxJump = 0.f;
	int32 Action = A_None, PrevCur = A_None;
	float T = 0.f, Elapsed = 0.f, Dur = 1.f;
	const float Dt = 1.f / Fps;
	const int Frames = (int)(Fps * 120.f);
	for (int f = 0; f < Frames; ++f)
	{
		T += Dt; Elapsed += Dt;
		if (Elapsed >= Dur)
		{
			const int r = std::rand() % (A_Count + 4);
			int32 Next = r >= A_Count ? A_None : r;
			// со стены (вис, лазание) на землю в игре попадают только через полёт: вставляем паузу без действия
			auto Wall = [](int32 X) { return X == A_Hang || X == A_Climb || X == A_ClimbUp || X == A_Drop || X == A_Hop || X == A_WallRun; };
			if (Wall(Action) && Next != A_None && !Wall(Next)) { Next = A_None; }
			// цикл, выбранный повторно, продолжается (в игре фаза циклов не сбрасывается)
			if (!(Next == Action && ActionLoops(Next))) { T = 0.f; }
			Elapsed = 0.f;
			Action = Next;
			Dur = Action == A_None ? 0.5f : Duration(Action) * (ActionLoops(Action) ? 1.5f : 1.f);
		}
		const float U = Action == A_None ? 0.f : T / Duration(Action);
		Layer.Update(Action, U, Dt);
		FTPLimbPose Base;
		EvaluatePose(R, P_Idle, std::fmod(f * Dt / 3.5f, 1.f), Base);
		// контекст — как в игре для текущего действия (кромка, руки на уступе)
		const FTPActionContext C = Scene(Layer.Cur, Layer.U, 0.f).C;
		Layer.Evaluate(R, C);
		Layer.BlendInto(Base);
		FTPPoseCS O;
		SolvePose(R, Base, O);
		for (int32 b = 0; b < B_Count; ++b) { Check(Finite(O.Pos[b]), "NaN в слое", "слой", (float)f, (float)b); }
		if (bHave)
		{
			for (int32 b = 0; b < B_Count; ++b)
			{
				const float J = (O.Pos[b] - Prev.Pos[b]).Size();
				if (Fps > 200.f && J > 25.f && GSeen["dbg"]++ < 5) { std::printf("    кадр %d: %.1f см, кость %d, %s U=%.3f веса %.2f/%.2f переход %.2f (было %s) наклон таза из %.2f в %.2f, вход %s\n", f, J, b, ActionName(Layer.Cur), Layer.U, Layer.WLower, Layer.WUpper, Layer.XFade, ActionName(PrevCur), Layer.From.PelvisPitch, Layer.Pose.PelvisPitch, ActionName(Action)); }
				MaxJump = FMath::Max(MaxJump, J);
			}
		}
		PrevCur = Layer.Cur;
		Prev = O; bHave = true;
	}
	return MaxJump;
}

static void TestLayer(const FTPRig& R)
{
	std::printf("слой действий (случайные переходы):\n");
	const float J60 = RunLayer(R, 60.f), J240 = RunLayer(R, 240.f);
	std::printf("  наибольший сдвиг сустава за кадр: %.2f см при 60 к/с, %.2f см при 240 к/с\n", J60, J240);
	Check(J240 < J60 * 0.45f, "сдвиг за кадр не уменьшается с шагом кадра (разрыв)", "слой", 0.f, J240 / FMath::Max(J60, 1e-3f));
	Check(J60 < 60.f, "слишком резкий переход", "слой", 0.f, J60);
}

// Постобработка: IK стоп, наклон, взгляд.
static void TestPost(const FTPRig& R)
{
	std::printf("постобработка (IK стоп, наклон, взгляд):\n");
	auto Lengths = [&](const FTPBonesCS& O, const char* Name)
	{
		for (int32 b = 2; b < B_Count; ++b)
		{
			const float L1 = (O.Pos[b] - O.Pos[BoneParent(b)]).Size();
			Check(std::fabs(L1 - R.BoneLength(b)) < 0.05f, "длина кости", Name, 0.f, L1 - R.BoneLength(b));
			Check(Finite(O.Pos[b]), "NaN", Name, 0.f, (float)b);
		}
	};
	float MaxErr = 0.f;
	for (int Pose = 0; Pose < 3; ++Pose)
	{
		for (int i = 0; i < 40; ++i)
		{
			FTPLimbPose L;
			EvaluatePose(R, Pose == 0 ? P_Idle : Pose == 1 ? P_Walk : P_Run, i / 40.f, L);
			FTPPoseCS P;
			SolvePose(R, L, P);
			// IK стоп: левая на ступеньку +12 см, правая в яму −15 см, наклон поверхности 15°
			FTPBonesCS O;
			O.FromPose(R, P);
			const FVector A0 = O.Pos[B_FootL], A1 = O.Pos[B_FootR];
			FTPFootIKInput In;
			In.Offset[0] = 12.f; In.Offset[1] = -15.f; In.Weight = 1.f;
			In.Normal[0] = FVector(-0.2588f, 0.f, 0.9659f);
			const float Drop = ApplyFootIK(O, In);
			Check(std::fabs(Drop + 15.f) < 0.01f, "таз опущен не к нижней стопе", "IK стоп", i / 40.f, Drop);
			const float E0 = (O.Pos[B_FootL] - (A0 + FVector(0.f, 0.f, 12.f))).Size();
			const float E1 = (O.Pos[B_FootR] - (A1 + FVector(0.f, 0.f, -15.f))).Size();
			if (Pose == 0) { Check(E0 < 0.5f && E1 < 0.5f, "лодыжка не на цели", "IK стоп", i / 40.f, FMath::Max(E0, E1)); }
			MaxErr = FMath::Max(MaxErr, FMath::Max(E0, E1));
			Lengths(O, "IK стоп");
			// колено после IK всё ещё вперёд
			for (int32 S = 0; S < 2; ++S)
			{
				const FVector Line = (O.Pos[FootBone(S)] - O.Pos[ThighBone(S)]).GetSafeNormal();
				const FVector Bend = RejectFrom(O.Pos[CalfBone(S)] - O.Pos[ThighBone(S)], Line);
				if (Bend.Size() > 3.f) { Check(Bend.X > 0.f, "колено назад после IK", "IK стоп", i / 40.f, Bend.X); }
			}
			// наклон: стопы на месте
			FTPBonesCS Q;
			Q.FromPose(R, P);
			const FVector B0 = Q.Pos[B_FootL], B1 = Q.Pos[B_FootR];
			ApplyLean(Q, 0.3f);
			const float LE = FMath::Max((Q.Pos[B_FootL] - B0).Size(), (Q.Pos[B_FootR] - B1).Size());
			Check(LE < 0.5f, "стопа уехала при наклоне", "наклон", i / 40.f, LE);
			Check(Q.Pos[B_Head].Y > P.Pos[B_Head].Y + 5.f, "наклон вправо не сместил голову вправо", "наклон", i / 40.f, Q.Pos[B_Head].Y - P.Pos[B_Head].Y);
			Lengths(Q, "наклон");
			// взгляд вправо на 60° и вниз
			FTPBonesCS H;
			H.FromPose(R, P);
			ApplyLookAt(H, R, 1.047f, 0.3f, 1.f);
			const FVector F = (H.Rot[B_Head] * R.RestRot[B_Head].Inverse()).RotateVector(FVector(1.f, 0.f, 0.f));
			Check(F.Y > 0.5f, "голова не повернулась вправо", "взгляд", i / 40.f, F.Y);
			Check(F.Z < -0.1f, "голова не наклонилась вниз", "взгляд", i / 40.f, F.Z);
			Lengths(H, "взгляд");
		}
	}
	std::printf("  наибольшая ошибка лодыжки после IK стоп: %.3f см\n", MaxErr);
}

static void Dump(const FTPRig& R, const char* Path)
{
	FILE* F = std::fopen(Path, "w");
	if (!F) { return; }
	struct FItem { int32 A; float U; float Shimmy; };
	const FItem Items[] = {
		{ A_Hang, 0.f, 0.f }, { A_Hang, .25f, 1.f }, { A_ClimbUp, .15f, 0.f }, { A_ClimbUp, .35f, 0.f }, { A_ClimbUp, .5f, 0.f },
		{ A_ClimbUp, .65f, 0.f }, { A_ClimbUp, .85f, 0.f }, { A_Drop, .3f, 0.f }, { A_Drop, .7f, 0.f }, { A_Climb, 0.f, 0.f },
		{ A_Climb, .5f, 0.f }, { A_Hop, .4f, 0.f }, { A_Hop, .8f, 0.f },
		{ A_Mantle, .1f, 0.f }, { A_Mantle, .3f, 0.f }, { A_Mantle, .5f, 0.f }, { A_Vault, .2f, 0.f }, { A_Vault, .35f, 0.f },
		{ A_Vault, .5f, 0.f }, { A_Vault, .7f, 0.f }, { A_WallRun, 0.f, 0.f }, { A_WallRun, .5f, 0.f }, { A_Roll, .15f, 0.f },
		{ A_Roll, .35f, 0.f }, { A_Roll, .55f, 0.f }, { A_Roll, .8f, 0.f },
		{ A_Turn, .5f, 0.f }, { A_LandHard, .3f, 0.f }, { A_Guard, 0.f, 0.f }, { A_Attack1, .3f, 0.f }, { A_Attack1, .55f, 0.f },
		{ A_Attack2, .3f, 0.f }, { A_Attack2, .55f, 0.f }, { A_Attack3, .38f, 0.f }, { A_Attack3, .62f, 0.f }, { A_Counter, .18f, 0.f },
		{ A_Counter, .45f, 0.f }, { A_Block, 0.f, 0.f }, { A_Dodge, .4f, 0.f },
		{ A_WindUp, .6f, 0.f }, { A_Strike, .45f, 0.f }, { A_HitReact, .2f, 0.f }, { A_Stagger, .4f, 0.f } };
	for (const FItem& It : Items)
	{
		FScene S = Scene(It.A, It.U, It.Shimmy);
		FTPLimbPose L;
		EvaluateAction(R, It.A, It.U, S.C, L);
		FTPPoseCS O;
		SolvePose(R, L, O);
		std::fprintf(F, "%s:%s %.2f", ActionName(It.A), SceneTag(It.A), It.U);
		for (int32 b = 0; b < B_Count; ++b)
		{
			const FVector P = O.Pos[b] + S.Root;
			std::fprintf(F, " %.3f %.3f %.3f", P.X, P.Y, P.Z);
		}
		std::fprintf(F, "\n");
	}
	std::fclose(F);
}

int main(int argc, char** argv)
{
	const FTPRig R = MakeMannequinRig();
	if (argc > 2 && std::strcmp(argv[1], "--dump") == 0) { Dump(R, argv[2]); return 0; }
	TestActions(R);
	TestLayer(R);
	TestPost(R);
	std::printf("проверок: %ld, ошибок: %d\nИТОГ: %s\n", GChecks, GFail, GFail ? "есть ошибки" : "чисто");
	return GFail ? 1 : 0;
}

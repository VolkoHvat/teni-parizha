// Тени Парижа — проверка построителя одежды (TPOutfitGen.h) без Unreal Engine и выгрузка сцен для превью.
// Запуск: outfit_test                     — тесты, последняя строка «ИТОГ: чисто» / «ИТОГ: есть ошибки»
//         outfit_test --dump каталог      — сцены для render_outfit (lineup.bin, poses.bin + подписи *.txt)
#include "CoreMinimal.h"
#include "TPAnimPoseMath.h"
#include "TPAnimActions.h"
#include "TPOutfitGen.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>

using namespace TPAnim;
using namespace TPOutfit;

static int GErr = 0;
static void Fail(const char* What, const char* Who, float V = 0.f)
{
	if (GErr < 40) { std::printf("  ОШИБКА: %s [%s] %.3f\n", What, Who, V); }
	++GErr;
}

struct FNamed { const char* Name; FDesc D; };
static std::vector<FNamed> Presets()
{
	return { { "герой", Hero() }, { "стража", Guard() }, { "буржуа", CitizenMale(0) }, { "мастеровой", CitizenMale(1) },
		{ "лавочник", CitizenMale(2) }, { "работница", CitizenFemale(0) }, { "горожанка", CitizenFemale(1) } };
}

// часть в позе: p' = Pos[b] + Delta[b] (p - RestPos[b])
static FVector Posed(const FTPRig& R, const FTPPoseCS& O, int32 B, const FVector& P)
{
	return O.Pos[B] + O.Delta[B].RotateVector(P - R.RestPos[B]);
}

static int32 Draws(const FMesh& M)
{
	int32 N = 0;
	for (int32 B = 0; B < B_Count; ++B) for (int32 S = 0; S < S_Count; ++S) { N += M.Pieces[B].Idx[S].Num() ? 1 : 0; }
	return N;
}

static void CheckMesh(const FTPRig& R, const FMesh& M, const char* Who)
{
	const int32 Tris = M.NumTris();
	if (Tris > 15000) { Fail("бюджет 15 тыс. треугольников превышен", Who, (float)Tris); }
	if (M.NumPieces() > 17) { Fail("частей (компонентов) больше 17", Who, (float)M.NumPieces()); }
	if (Draws(M) > 26) { Fail("вызовов отрисовки (секций) больше 26", Who, (float)Draws(M)); }
	int32 Flip = 0, Thin = 0;
	for (int32 B = 0; B < B_Count; ++B)
	{
		const FPiece& P = M.Pieces[B];
		if (P.Pos.Num() != P.Nrm.Num() || P.Pos.Num() != P.UV.Num() || P.Pos.Num() != P.Col.Num()) { Fail("размеры буферов части не совпадают", Who, (float)B); }
		if (!P.IsEmpty() && (B == B_Root || B == B_Spine1 || B == B_ClavL || B == B_ClavR || B == B_BallL || B == B_BallR)) { Fail("часть на неожиданной кости", Who, (float)B); }
		for (int32 I = 0; I < P.Pos.Num(); ++I)
		{
			if (P.Pos[I].ContainsNaN() || P.Nrm[I].ContainsNaN() || !std::isfinite(P.UV[I].X) || !std::isfinite(P.UV[I].Y)) { Fail("NaN в вершине", Who, (float)B); break; }
			if (std::fabs(P.Nrm[I].Size() - 1.f) > 1e-3f) { Fail("нормаль не единичная", Who, P.Nrm[I].Size()); break; }
			// вершина не дальше 75 см от своего сустава (поймает часть не на той кости)
			const float Dist = (P.Pos[I] - R.RestPos[B]).Size();
			if (Dist > 75.f * R.Scale && B != B_Pelvis) { Fail("вершина далеко от своей кости", Who, Dist); break; }
			if (B == B_Pelvis && Dist > 105.f * R.Scale) { Fail("вершина таза далеко от кости", Who, Dist); break; }
		}
		for (int32 S = 0; S < S_Count; ++S)
		{
			const TArray<int32>& Ix = P.Idx[S];
			if (Ix.Num() % 3) { Fail("число индексов не кратно 3", Who); }
			for (int32 K = 0; K + 2 < Ix.Num(); K += 3)
			{
				const int32 A = Ix[K], Bq = Ix[K + 1], C = Ix[K + 2];
				if (!P.Pos.IsValidIndex(A) || !P.Pos.IsValidIndex(Bq) || !P.Pos.IsValidIndex(C)) { Fail("индекс вне буфера", Who); break; }
				const FVector Fn = FVector::CrossProduct(P.Pos[Bq] - P.Pos[A], P.Pos[C] - P.Pos[A]);
				if (Fn.SizeSquared() < 1e-12f) { ++Thin; continue; }
				// лицевая сторона в UE — по часовой стрелке снаружи: нормаль треугольника против нормалей вершин
				if (FVector::DotProduct(Fn, P.Nrm[A] + P.Nrm[Bq] + P.Nrm[C]) >= 0.f) { ++Flip; }
			}
		}
	}
	if (Flip) { Fail("треугольники с обходом против нормалей", Who, (float)Flip); }
	if (Thin > Tris / 50) { Fail("много вырожденных треугольников", Who, (float)Thin); }
}

// Нормали «наружу»: для замкнутых частей конечностей нормаль смотрит от оси кости (доля вершин с обратной нормалью мала).
static void CheckOutward(const FTPRig& R, const FMesh& M, const char* Who)
{
	const int32 Limb[] = { B_UpperArmL, B_LowerArmL, B_UpperArmR, B_LowerArmR, B_ThighL, B_CalfL, B_ThighR, B_CalfR, B_Neck };
	for (int32 B : Limb)
	{
		const FPiece& P = M.Pieces[B];
		if (P.IsEmpty()) { continue; }
		const int32 Child = (B == B_Neck) ? B_Head : B + 1;
		const FVector A = R.RestPos[B], Dir = (R.RestPos[Child] - A).GetSafeNormal();
		int32 Bad = 0, Used = 0;
		std::vector<char> Cloth(P.Pos.Num(), 0);   // мелкие гладкие детали (пуговицы) не в счёт
		for (int32 K = 0; K < P.Idx[S_Cloth].Num(); ++K) { Cloth[P.Idx[S_Cloth][K]] = 1; }
		for (int32 I = 0; I < P.Pos.Num(); ++I)
		{
			if (!Cloth[I]) { continue; }
			++Used;
			const FVector Rad = RejectFrom(P.Pos[I] - A, Dir);
			if (Rad.Size() < 1.5f) { continue; }
			if (FVector::DotProduct(Rad.GetSafeNormal(), P.Nrm[I]) < -0.3f) { ++Bad; }
		}
		if (Bad > Used / 20) { Fail("нормали части смотрят внутрь", Who, (float)B); }
	}
}

// Ткань закрывает тело: точки «скелета» (оси костей) лежат внутри одежды — луч от оси наружу пересекает свою часть.
// Проверяем стыки в позах: точки на поверхности воображаемого тела (радиус 4 см вокруг оси кости) должны быть закрыты
// хотя бы одной частью (ближайшая вершина одежды не дальше 6 см по направлению наружу).
static void CheckPoses(const FTPRig& R, const FMesh& M, const char* Who)
{
	struct FPoseCase { const char* Name; int32 Pose; int32 Action; float U; };
	const FPoseCase Cases[] = { { "стойка", P_Idle, A_None, 0.f }, { "шаг", P_Walk, A_None, .25f }, { "бег", P_Run, A_None, .25f },
		{ "спринт", P_Sprint, A_None, .75f }, { "присед", P_CrouchIdle, A_None, 0.f }, { "вис", P_Hang, A_None, .5f },
		{ "удар", P_Idle, A_Attack1, .45f }, { "подъём", P_Idle, A_ClimbUp, .5f } };
	for (const FPoseCase& Cs : Cases)
	{
		FTPLimbPose L;
		EvaluatePose(R, Cs.Pose, Cs.U, L);
		if (Cs.Action != A_None) { FTPActionContext C; EvaluateAction(R, Cs.Action, Cs.U, C, L); }
		FTPPoseCS O;
		SolvePose(R, L, O);
		// стопы не уходят в пол в стойке
		if (Cs.Pose == P_Idle && Cs.Action == A_None)
		{
			float MinZ = 1e9f;
			for (int32 B : { (int32)B_FootL, (int32)B_FootR })
				for (const FVector& P : M.Pieces[B].Pos) { MinZ = FMath::Min(MinZ, Posed(R, O, B, P).Z); }
			if (MinZ < -1.5f || MinZ > 2.5f) { Fail("подошва не на полу в стойке", Who, MinZ); }
		}
		// щели в суставах: середины суставов рук и ног закрыты (есть вершины одежды на расстоянии 3..11 см во все стороны)
		const int32 Joints[] = { B_UpperArmL, B_LowerArmL, B_HandL, B_UpperArmR, B_LowerArmR, B_HandR, B_CalfL, B_CalfR, B_FootL, B_FootR };
		for (int32 J : Joints)
		{
			const FVector C = O.Pos[J];
			int32 Covered = 0, Total = 0;
			for (int32 K = 0; K < 26; ++K)
			{
				const float Ph = PI * (K + 0.5f) / 26.f * 1.f, Th = K * 2.39996f;
				const FVector Dir(FMath::Sin(Ph) * FMath::Cos(Th), FMath::Sin(Ph) * FMath::Sin(Th), FMath::Cos(Ph));
				++Total;
				bool bHit = false;
				for (int32 B = 0; B < B_Count && !bHit; ++B)
				{
					for (const FVector& P : M.Pieces[B].Pos)
					{
						const FVector Q = Posed(R, O, B, P) - C;
						const float Dd = FVector::DotProduct(Q, Dir);
						if (Dd > 2.5f && Dd < 11.f && (Q - Dir * Dd).Size() < 2.5f) { bHit = true; break; }
					}
				}
				Covered += bHit ? 1 : 0;
			}
			if (Covered < Total - 2)
			{
				char Buf[160]; std::snprintf(Buf, sizeof Buf, "%s/%s: сустав %s открыт", Who, Cs.Name, BoneName(J));
				Fail(Buf, "щель", (float)(Total - Covered));
			}
		}
	}
}

// ---------------- выгрузка для превью ----------------
struct FOut { FILE* F; FILE* T; };
static void DumpPosed(FOut& Out, const FTPRig& R, const FMesh& M, const FTPPoseCS& O, const FVector& Offset, float Yaw)
{
	const FQuat Q = YawQ(Yaw);
	for (int32 B = 0; B < B_Count; ++B)
	{
		const FPiece& P = M.Pieces[B];
		for (int32 S = 0; S < S_Count; ++S)
		{
			const TArray<int32>& Ix = P.Idx[S];
			if (Ix.Num() == 0) { continue; }
			int32 Hdr[3] = { S, P.Pos.Num(), Ix.Num() };
			std::fwrite(Hdr, 4, 3, Out.F);
			for (int32 I = 0; I < P.Pos.Num(); ++I)
			{
				const FVector W = Q.RotateVector(Posed(R, O, B, P.Pos[I])) + Offset;
				const FVector N = Q.RotateVector(O.Delta[B].RotateVector(P.Nrm[I]));
				// UE (см) → Lutèce (м): x = -Y, y = Z, z = X
				const float D[8] = { -W.Y / 100.f, W.Z / 100.f, W.X / 100.f, -N.Y, N.Z, N.X, P.UV[I].X, P.UV[I].Y };
				const unsigned char C[4] = { P.Col[I].R, P.Col[I].G, P.Col[I].B, 255 };
				std::fwrite(D, 4, 8, Out.F);
				std::fwrite(C, 1, 4, Out.F);
			}
			std::fwrite(&Ix[0], 4, Ix.Num(), Out.F);
		}
	}
}

static void Label(FOut& Out, const FVector& At, const char* Text)
{
	std::fprintf(Out.T, "%.3f %.3f %.3f %s\n", -At.Y / 100.f, At.Z / 100.f, At.X / 100.f, Text);
}

static void Dump(const FTPRig& R, const char* Dir)
{
	const std::vector<FNamed> Ps = Presets();
	{
		// ряд нарядов: спереди (3/4) — рендер с разных камер
		FOut Out{ std::fopen((std::string(Dir) + "/lineup.bin").c_str(), "wb"), std::fopen((std::string(Dir) + "/lineup.txt").c_str(), "w") };
		FTPLimbPose L; EvaluatePose(R, P_Idle, 0.f, L);
		FTPPoseCS O; SolvePose(R, L, O);
		for (int32 I = 0; I < (int32)Ps.size(); ++I)
		{
			FMesh M; Build(R, Ps[I].D, M);
			const FVector Off(0.f, (I - (Ps.size() - 1) * 0.5f) * 85.f, 0.f);
			DumpPosed(Out, R, M, O, Off, 0.f);
			char T[96]; std::snprintf(T, sizeof T, "%s %d", Ps[I].Name, M.NumTris());
			Label(Out, Off + FVector(0.f, 0.f, 205.f), T);
		}
		std::fclose(Out.F); std::fclose(Out.T);
	}
	{
		// герой и стражник в позах паркура и боя
		struct FPc { const char* Name; int32 Pose; int32 Action; float U; };
		const FPc Cs[] = { { "шаг", P_Walk, A_None, .25f }, { "бег", P_Run, A_None, .25f }, { "спринт", P_Sprint, A_None, .75f },
			{ "присед", P_CrouchWalk, A_None, .25f }, { "прыжок", P_JumpAir, A_None, .5f }, { "вис", P_Hang, A_None, .5f },
			{ "подъём", P_Idle, A_ClimbUp, .55f }, { "перелёт", P_Run, A_Vault, .5f }, { "кувырок", P_Run, A_Roll, .4f },
			{ "удар", P_Idle, A_Attack1, .45f }, { "уклон", P_Idle, A_Dodge, .4f } };
		const int32 N = (int32)(sizeof(Cs) / sizeof(Cs[0]));
		for (int32 Who = 0; Who < 2; ++Who)
		{
			const std::string Base = std::string(Dir) + (Who == 0 ? "/poses_hero" : "/poses_guard");
			FOut Out{ std::fopen((Base + ".bin").c_str(), "wb"), std::fopen((Base + ".txt").c_str(), "w") };
			FMesh M; Build(R, Who == 0 ? Hero() : Guard(), M);
			for (int32 I = 0; I < N; ++I)
			{
				FTPLimbPose L; EvaluatePose(R, Cs[I].Pose, Cs[I].U, L);
				if (Cs[I].Action != A_None) { FTPActionContext C; EvaluateAction(R, Cs[I].Action, Cs[I].U, C, L); }
				FTPPoseCS O; SolvePose(R, L, O);
				// корень позы держим в своей клетке (корневое движение клипов не накапливаем)
				const FVector Root(O.Pos[B_Pelvis].X - R.RestPos[B_Pelvis].X, O.Pos[B_Pelvis].Y, 0.f);
				const FVector Off = FVector(0.f, (I - (N - 1) * 0.5f) * 95.f, 0.f) - Root;
				DumpPosed(Out, R, M, O, Off, 0.f);
				Label(Out, Off + Root + FVector(0.f, 0.f, 215.f), Cs[I].Name);
			}
			std::fclose(Out.F); std::fclose(Out.T);
		}
		// вид сбоку: шаг, бег, спринт, присед, перелёт — друг за другом вдоль X (полы и ноги в профиль)
		for (int32 Who = 0; Who < 2; ++Who)
		{
			const std::string Base = std::string(Dir) + (Who == 0 ? "/side_hero" : "/side_guard");
			FOut Out{ std::fopen((Base + ".bin").c_str(), "wb"), std::fopen((Base + ".txt").c_str(), "w") };
			FMesh M; Build(R, Who == 0 ? Hero() : Guard(), M);
			const int32 Pick[] = { 0, 1, 2, 3, 7 };
			for (int32 K = 0; K < 5; ++K)
			{
				const FPc& Pc = Cs[Pick[K]];
				FTPLimbPose L; EvaluatePose(R, Pc.Pose, Pc.U, L);
				if (Pc.Action != A_None) { FTPActionContext C; EvaluateAction(R, Pc.Action, Pc.U, C, L); }
				FTPPoseCS O; SolvePose(R, L, O);
				const FVector Root(O.Pos[B_Pelvis].X - R.RestPos[B_Pelvis].X, O.Pos[B_Pelvis].Y, 0.f);
				const FVector Off = FVector((K - 2) * 130.f, 0.f, 0.f) - Root;
				DumpPosed(Out, R, M, O, Off, 0.f);
				Label(Out, Off + Root + FVector(0.f, 0.f, 215.f), Pc.Name);
			}
			std::fclose(Out.F); std::fclose(Out.T);
		}
	}
}

int main(int argc, char** argv)
{
	const FTPRig Rig = MakeMannequinRig();
	if (argc > 2 && std::strcmp(argv[1], "--dump") == 0) { Dump(Rig, argv[2]); return 0; }
	for (const FNamed& P : Presets())
	{
		FMesh M; Build(Rig, P.D, M);
		std::printf("%-12s треугольников %5d, частей %2d, вызовов %2d\n", P.Name, M.NumTris(), M.NumPieces(), Draws(M));
		CheckMesh(Rig, M, P.Name);
		{ FMesh Raw; Build(Rig, P.D, Raw, false); CheckOutward(Rig, Raw, P.Name); }
		CheckPoses(Rig, M, P.Name);
	}
	// другой рост и полнота: без NaN, в бюджете
	for (float Sc : { 0.88f, 1.12f })
	{
		FTPRig R2 = Rig;
		for (int32 B = 0; B < B_Count; ++B) { R2.RestPos[B] = R2.RestPos[B] * Sc; }
		R2.Finalize();
		for (int32 V = 0; V < 3; ++V)
		{
			FDesc D = CitizenMale(V); D.Bulk = Sc > 1.f ? 1.2f : 0.9f;
			FMesh M; Build(R2, D, M);
			CheckMesh(R2, M, Sc > 1.f ? "рост 1.12" : "рост 0.88");
		}
	}
	// ключ кэша различает наряды и совпадает у одинаковых
	if (HashDesc(Hero()) == HashDesc(Guard()) || HashDesc(CitizenMale(0)) != HashDesc(CitizenMale(3))) { Fail("ключ кэша", "HashDesc"); }
	std::printf("ИТОГ: %s\n", GErr ? "есть ошибки" : "чисто");
	return GErr ? 1 : 0;
}

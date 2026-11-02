// Тени Парижа — тесты библиотеки архитектурных элементов (TPCityElements) и построителя квартала обычным g++.
// Проверяет: профили (обход, монотонность), триангуляцию, ориентацию граней (поток площади и объём через теорему
// Гаусса), проёмы и арки, руст, отсутствие NaN и вырожденных нормалей; печатает число треугольников элементов.
// Запуск: Tools/CityTest/run.sh
#include "TPCityElements.h"
#include <cstdio>
#include <cstdlib>
#include <string>

using namespace TPCity;

static int GFails = 0, GChecks = 0;
#define EXPECT(cond, ...) do { ++GChecks; if (!(cond)) { ++GFails; if (GFails < 40) { printf("ОШИБКА %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } } while (0)

static FV3 Cr(const FV3& A, const FV3& B) { return FV3(A.Y * B.Z - A.Z * B.Y, A.Z * B.X - A.X * B.Z, A.X * B.Y - A.Y * B.X); }
static float Dt(const FV3& A, const FV3& B) { return A.X * B.X + A.Y * B.Y + A.Z * B.Z; }
static float Ln(const FV3& A) { return std::sqrt(Dt(A, A)); }

/** сумма векторов площади (лицевых) и объём по Гауссу относительно точки O */
static void Flux(const FMeshBuf& M, int32 TriFrom, const FV3& O, FV3& AreaVec, float& Vol)
{
	AreaVec = FV3(0, 0, 0); Vol = 0.f;
	for (int32 K = TriFrom * 3; K + 2 < M.Idx.Num(); K += 3)
	{
		const FV3 A = M.Pos[M.Idx[K]] - O, B = M.Pos[M.Idx[K + 1]] - O, C = M.Pos[M.Idx[K + 2]] - O;
		const FV3 N = Cr(B - A, C - A) * 0.5f;
		AreaVec = AreaVec + N;
		Vol += Dt(A, Cr(B, C)) / 6.f;
	}
}

/** вершины без NaN, нормали единичные и согласованы с обходом треугольника */
static void CheckMesh(const FMeshBuf& M, const char* Name)
{
	int32 Bad = 0, BadN = 0;
	for (int32 I = 0; I < M.Pos.Num(); ++I)
	{
		const FV3 P = M.Pos[I], N = M.Nrm[I];
		if (!std::isfinite(P.X) || !std::isfinite(P.Y) || !std::isfinite(P.Z)) ++Bad;
		if (std::fabs(Ln(N) - 1.f) > 1e-3f) ++BadN;
	}
	EXPECT(Bad == 0, "%s: %d вершин с NaN", Name, Bad);
	EXPECT(BadN == 0, "%s: %d неединичных нормалей", Name, BadN);
	int32 Flip = 0;
	for (int32 K = 0; K + 2 < M.Idx.Num(); K += 3)
	{
		const FV3 A = M.Pos[M.Idx[K]], B = M.Pos[M.Idx[K + 1]], C = M.Pos[M.Idx[K + 2]];
		const FV3 G = Cr(B - A, C - A);
		if (Ln(G) < 1e-9f) continue;
		if (Dt(G, M.Nrm[M.Idx[K]]) <= 0.f) ++Flip;
	}
	EXPECT(Flip == 0, "%s: %d треугольников с обходом против нормали", Name, Flip);
}

static float PolyArea(const FV2* P, int32 N)
{
	float A = 0.f;
	for (int32 I = 0; I < N; ++I) A += P[I].X * P[(I + 1) % N].Y - P[(I + 1) % N].X * P[I].Y;
	return A * 0.5f;
}

static int32 Tris(const FMeshBuf& M) { return M.Idx.Num() / 3; }

int main()
{
	// 1. профили
	const char* Names[] = { "карниз", "тяга", "подоконник", "наличник", "база", "капитель", "цоколь", "рама", "кронштейн" };
	for (int32 P = 0; P < (int32)EProfile::Num; ++P)
	{
		const FProfile Pr = GetProfile((EProfile)P);
		EXPECT(Pr.Num >= 3, "профиль %s пуст", Names[P]);
		EXPECT(ProfileIsCCW(Pr), "профиль %s не против часовой стрелки", Names[P]);
		// горизонтальные обломы: h не убывает, начало и конец у стены (o = 0)
		const bool bHoriz = P != (int32)EProfile::Architrave && P != (int32)EProfile::Sash;
		if (bHoriz)
		{
			for (int32 I = 1; I < Pr.Num; ++I) EXPECT(Pr.Pts[I].Y >= Pr.Pts[I - 1].Y - 1e-6f, "профиль %s: h убывает в точке %d", Names[P], I);
			EXPECT(Pr.Pts[0].X == 0.f && Pr.Pts[Pr.Num - 1].X == 0.f, "профиль %s не начинается/кончается у стены", Names[P]);
		}
		// триангуляция покрывает площадь
		TArray<int32> T;
		TriangulatePolygon(Pr.Pts, Pr.Num, T);
		float A = 0.f;
		for (int32 I = 0; I + 2 < T.Num(); I += 3)
		{
			const FV2 Q[3] = { Pr.Pts[T[I]], Pr.Pts[T[I + 1]], Pr.Pts[T[I + 2]] };
			const float At = PolyArea(Q, 3);
			EXPECT(At >= -1e-7f, "профиль %s: треугольник %d вывернут", Names[P], I / 3);
			A += At;
		}
		EXPECT(std::fabs(A - PolyArea(Pr.Pts, Pr.Num)) < 1e-5f, "профиль %s: площадь триангуляции %.6f ≠ %.6f", Names[P], A, PolyArea(Pr.Pts, Pr.Num));
	}

	// 2. облом вдоль фасада с торцами: объём = площадь профиля × длина, поток = площадь прилегания × нормаль стены
	const FFacadeFrame F0(FV3(1.f, 2.f, 3.f), FV3(1.f, 0.f, 0.f), FV3(0.f, 0.f, 1.f));          // «правая» рамка
	const FFacadeFrame F1(FV3(-4.f, 0.f, 7.f), FV3(0.f, 0.f, 1.f), FV3(-1.f, 0.f, 0.f));       // поворот
	const FFacadeFrame FM(FV3(0.f, 0.f, 0.f), FV3(-1.f, 0.f, 0.f), FV3(0.f, 0.f, 1.f));        // зеркальная
	const FFacadeFrame Frames[3] = { F0, F1, FM };
	const EProfile HP[] = { EProfile::CrownCornice, EProfile::StringCourse, EProfile::Sill, EProfile::Plinth, EProfile::Console };
	for (int32 Fi = 0; Fi < 3; ++Fi)
	{
		for (EProfile P : HP)
		{
			const FProfile Pr = GetProfile(P);
			FMeshBuf M;
			const float L = 3.f, S = 1.3f, V = 2.f;
			Moulding(M, Frames[Fi], 0.5f, 0.5f + L, V, P, S, true, FColor(255, 255, 255));
			CheckMesh(M, Names[(int32)P]);
			FV3 AV; float Vol;
			Flux(M, 0, Frames[Fi].O, AV, Vol);
			const float Expect = PolyArea(Pr.Pts, Pr.Num) * S * S * L;
			EXPECT(std::fabs(Vol - Expect) < 1e-3f * std::max(1.f, Expect) + 1e-5f, "облом %s, рамка %d: объём %.5f ≠ %.5f", Names[(int32)P], Fi, Vol, Expect);
			// недостающая грань прилегания смотрит в стену (−N) с площадью L × высота профиля → сумма = +N × L × H
			const float HProf = (Pr.Pts[Pr.Num - 1].Y - Pr.Pts[0].Y) * S;
			const FV3 Want = Frames[Fi].N * (L * HProf);
			EXPECT(Ln(AV - Want) < 1e-3f, "облом %s, рамка %d: поток площади (%.4f %.4f %.4f) ≠ (%.4f %.4f %.4f)", Names[(int32)P], Fi, AV.X, AV.Y, AV.Z, Want.X, Want.Y, Want.Z);
		}
	}

	// 3. кольцо вокруг дома: поток площади = 0 (замкнутое по периметру, прилегание — вертикальные грани, взаимно гасятся)
	{
		FMeshBuf M;
		MouldingLoop(M, 2.f, 3.f, 14.f, 11.f, 9.f, EProfile::CrownCornice, 1.f, FColor(255, 255, 255));
		CheckMesh(M, "кольцо карниза");
		FV3 AV; float Vol;
		Flux(M, 0, FV3(0, 0, 0), AV, Vol);
		EXPECT(Ln(AV) < 1e-3f, "кольцо: поток площади %.5f", Ln(AV));
		// все лицевые нормали смотрят от дома или вверх/вниз (не внутрь)
		int32 In = 0;
		for (int32 K = 0; K + 2 < M.Idx.Num(); K += 3)
		{
			const FV3 A = M.Pos[M.Idx[K]], N = M.Nrm[M.Idx[K]];
			const FV3 C(8.f, A.Y, 7.f);
			if (std::fabs(N.Y) < 0.99f && Dt(N, A - C) < 0.f && Dt(FV3(N.X, 0, N.Z), FV3(N.X, 0, N.Z)) > 0.5f) ++In;
		}
		EXPECT(In == 0, "кольцо: %d граней смотрят внутрь дома", In);
	}

	// 4. проёмы: арки, контур, откосы
	{
		const float Rises[] = { 0.f, 0.16f, 0.35f, 0.54f };
		for (float Rise : Rises)
		{
			FOpening O; O.UC = 2.f; O.V0 = 1.f; O.W = 1.08f; O.H = 1.6f; O.Rise = Rise;
			TArray<FV2> Arc;
			O.ArcPoints(Arc);
			EXPECT(std::fabs(Arc[0].X - O.U0()) < 1e-5f && std::fabs(Arc[0].Y - O.Spring()) < 1e-5f, "арка %.2f: левая пята", Rise);
			EXPECT(std::fabs(Arc.Last().X - O.U1()) < 1e-5f, "арка %.2f: правая пята", Rise);
			EXPECT(std::fabs(O.TopAt(O.UC) - O.Top()) < 1e-4f, "арка %.2f: верх %.4f ≠ %.4f", Rise, O.TopAt(O.UC), O.Top());
			for (int32 I = 1; I < Arc.Num(); ++I) EXPECT(Arc[I].X > Arc[I - 1].X, "арка %.2f: точки не слева направо", Rise);
			if (O.IsArched())
			{
				const float R = O.ArcRadius(), CV = O.ArcCenterV();
				for (const FV2& P : Arc) EXPECT(std::fabs(std::sqrt((P.X - O.UC) * (P.X - O.UC) + (P.Y - CV) * (P.Y - CV)) - R) < 1e-4f, "арка %.2f: точка не на окружности", Rise);
			}
			TArray<FV2> L;
			O.Outline(L);
			EXPECT(PolyArea(L.GetData(), L.Num()) < 0.f, "контур проёма %.2f не по часовой стрелке", Rise);
			// откосы смотрят внутрь проёма
			const FFacadeFrame& F = Frames[(int32)(Rise * 10.f) % 3];
			FMeshBuf M;
			Reveal(M, F, O, 0.24f, 0.5f, true);
			CheckMesh(M, "откосы");
			const FV3 Ctr = F.P(O.UC, O.V0 + (O.Top() - O.V0) * 0.5f, -0.12f);
			int32 Out = 0;
			for (int32 K = 0; K + 2 < M.Idx.Num(); K += 3)
			{
				const FV3 A = M.Pos[M.Idx[K]], B = M.Pos[M.Idx[K + 1]], C = M.Pos[M.Idx[K + 2]];
				const FV3 G = (A + B + C) * (1.f / 3.f);
				if (Dt(M.Nrm[M.Idx[K]], Ctr - G) <= 0.f) ++Out;
			}
			EXPECT(Out == 0, "откосы арки %.2f: %d граней смотрят наружу", Rise, Out);
			// стена с проёмом + заполнение = площадь прямоугольника стены
			FMeshBuf W, G;
			WallAroundOpening(W, F, O, 0.f, 4.f, 0.f, 3.4f, 0.45f);
			OpeningFill(G, F, O, 0.f, 1.f);
			CheckMesh(W, "стена с проёмом");
			FV3 AW, AG; float VW, VG;
			Flux(W, 0, F.O, AW, VW);
			Flux(G, 0, F.O, AG, VG);
			EXPECT(std::fabs(Dt(AW + AG, F.N) - 4.f * 3.4f) < 1e-3f, "стена+проём %.2f: площадь %.4f ≠ %.4f", Rise, Dt(AW + AG, F.N), 4.f * 3.4f);
			EXPECT(Ln(AW + AG - F.N * (4.f * 3.4f)) < 1e-3f, "стена+проём %.2f: грани не в плоскости фасада", Rise);
			// наличник: замкнутый (окно) — поток равен площади прилегания × (−N)·(−1)
			FMeshBuf S;
			Surround(S, F, O, 1.f, true, FColor(255, 255, 255));
			CheckMesh(S, "наличник");
			FMeshBuf Sh;
			Sash(Sh, F, O, -0.24f, 3, 2, false, FColor(255, 255, 255));
			CheckMesh(Sh, "переплёт");
			FMeshBuf K;
			Keystone(K, F, O, 0.25f, 0.09f, FColor(255, 255, 255));
			CheckMesh(K, "замок");
			FV3 AK; float VK;
			Flux(K, 0, F.O, AK, VK);
			EXPECT(VK > 0.f, "замок: объём %.5f не положителен", VK);
		}
	}

	// 5. руст: профиль от стены к стене, вынос в пределах, объём ≈ (длина × площадь)
	{
		const float Ranges[][2] = { { 0.f, 3.6f }, { 0.3f, 2.f }, { 1.13f, 1.13f + 0.45f * 3.f }, { 2.71f, 3.6f } };
		for (auto& R : Ranges)
		{
			FMeshBuf M;
			M.Color = FColor(255, 255, 255);
			Rustication(M, F0, 0.f, 2.f, R[0], R[1], 0.f, 0.45f, 0.04f, 0.035f, true, true, 0.45f);
			CheckMesh(M, "руст");
			FV3 AV; float Vol;
			Flux(M, 0, F0.O, AV, Vol);
			// вынос ≤ 4 см → объём ≤ 2 м × высота × 4 см и > половины этого
			const float Max = 2.f * (R[1] - R[0]) * 0.04f;
			EXPECT(Vol <= Max + 1e-4f && Vol > Max * 0.5f, "руст %.2f..%.2f: объём %.5f вне (%.5f, %.5f]", R[0], R[1], Vol, Max * 0.5f, Max);
			float MinV = 1e9f, MaxV = -1e9f, MaxD = -1e9f, MinD = 1e9f;
			for (const FV3& P : M.Pos)
			{
				const FV3 L = P - F0.O;
				MinV = std::min(MinV, L.Y); MaxV = std::max(MaxV, L.Y);
				MaxD = std::max(MaxD, Dt(L, F0.N)); MinD = std::min(MinD, Dt(L, F0.N));
			}
			EXPECT(std::fabs(MinV - R[0]) < 1e-4f && std::fabs(MaxV - R[1]) < 1e-4f, "руст %.2f..%.2f: высоты %.4f..%.4f", R[0], R[1], MinV, MaxV);
			EXPECT(MinD > -1e-5f && MaxD < 0.04f + 1e-5f, "руст: глубина %.4f..%.4f", MinD, MaxD);
		}
	}

	// 6. прочие элементы: без NaN и вывернутых граней
	{
		FMeshBuf M;
		Pilaster(M, F1, 1.f, 0.6f, 0.08f, 3.75f, 12.f, FColor(255, 255, 255));
		CheckMesh(M, "пилястра");
		FMeshBuf S;
		const FFacadeFrame SF(FV3(0, 1, 0), FV3(0.8f, 0, 0.6f), FV3(-0.6f, 0, 0.8f));
		Shutter(S, SF, 0.54f, 1.7f, 0.02f, true);
		CheckMesh(S, "ставня");
		FMeshBuf D;
		Dentils(D, FM, 0.2f, 9.8f, 10.f, 10.06f, 0.22f, 0.07f, 0.15f);
		CheckMesh(D, "сухарики");
		FMeshBuf C;
		Console(C, F0, 1.f, 0.24f, 6.f, 0.9f, FColor(255, 255, 255));
		CheckMesh(C, "кронштейн");
		FMeshBuf RW;
		RW.Color = FColor(255, 255, 255);
		FOpening O; O.UC = 1.35f; O.V0 = 0.f; O.W = 1.6f; O.H = 2.3f; O.Rise = 0.8f;
		RusticatedWall(RW, F0, &O, 0.f, 2.7f, 0.f, 3.6f, 0.f, 0.45f, 0.45f);
		CheckMesh(RW, "руст с аркой");

		// число треугольников элементов (для бюджета: этап 9 — модули и HISM)
		FMeshBuf E[8];
		FOpening W; W.UC = 1.35f; W.V0 = 0.8f; W.W = 1.08f; W.H = 1.55f; W.Rise = 0.16f;
		Surround(E[0], F0, W, 1.f, true, FColor(255, 255, 255));
		Sash(E[1], F0, W, -0.24f, 3, 1, false, FColor(255, 255, 255));
		Reveal(E[2], F0, W, 0.24f, 0.5f, true);
		Shutter(E[3], SF, 0.54f, 1.7f, 0.02f, true);
		E[4].Color = FColor(255, 255, 255);
		RusticatedWall(E[4], F0, &O, 0.f, 2.7f, 0.f, 3.6f, 0.f, 0.45f, 0.45f);
		MouldingLoop(E[5], 0.f, 0.f, 12.f, 10.f, 10.f, EProfile::CrownCornice, 1.f, FColor(255, 255, 255));
		Dentils(E[6], F0, 0.2f, 11.8f, 10.1f, 10.16f, 0.22f, 0.07f, 0.15f);
		Pilaster(E[7], F0, 0.4f, 0.6f, 0.08f, 3.75f, 12.f, FColor(255, 255, 255));
		printf("треугольники: наличник лучковый %d, переплёт %d, откосы %d, ставня %d, руст пролёта с аркой %d, карниз-кольцо 12×10 м %d, сухарики 12 м %d, пилястра %d\n",
			Tris(E[0]), Tris(E[1]), Tris(E[2]), Tris(E[3]), Tris(E[4]), Tris(E[5]), Tris(E[6]), Tris(E[7]));
	}

	// 7. квартал целиком: без NaN, ориентация согласована
	{
		FBuilder B;
		B.BuildLevel(1789);
		int32 Bad = 0;
		for (const FMeshBuf& M : B.Bufs)
		{
			for (int32 I = 0; I < M.Pos.Num(); ++I)
			{
				if (!std::isfinite(M.Pos[I].X + M.Pos[I].Y + M.Pos[I].Z) || std::fabs(Ln(M.Nrm[I]) - 1.f) > 1e-3f) ++Bad;
			}
		}
		EXPECT(Bad == 0, "квартал: %d плохих вершин", Bad);
		printf("квартал: домов %d, треугольников %d\n", B.NumBuildings, B.CountTriangles());
	}

	printf("проверок %d, ошибок %d\nИТОГ: %s\n", GChecks, GFails, GFails ? "есть ошибки" : "чисто");
	return GFails ? 1 : 0;
}

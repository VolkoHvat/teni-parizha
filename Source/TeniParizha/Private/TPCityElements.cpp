// Тени Парижа — библиотека архитектурных элементов (см. TPCityElements.h). Координаты Lutèce: метры, Y вверх.
// Владелец модуля: агент «город».
#include "TPCityElements.h"

namespace TPCityElemImpl
{
	using TPCity::FV2;
	using TPCity::FV3;

	static constexpr float ElemPi = 3.14159265358979f;

	static inline float Dot3(const FV3& A, const FV3& B) { return A.X * B.X + A.Y * B.Y + A.Z * B.Z; }
	static inline FV3 Cross3(const FV3& A, const FV3& B) { return FV3(A.Y * B.Z - A.Z * B.Y, A.Z * B.X - A.X * B.Z, A.X * B.Y - A.Y * B.X); }
	static inline float Len3(const FV3& A) { return FMath::Sqrt(Dot3(A, A)); }
	static inline FV3 Norm3(const FV3& A) { const float L = Len3(A); return L > 1e-12f ? A * (1.f / L) : FV3(0.f, 0.f, 0.f); }
	static inline float Cross2(const FV2& A, const FV2& B) { return A.X * B.Y - A.Y * B.X; }
	static inline FV2 Sub2(const FV2& A, const FV2& B) { return FV2(A.X - B.X, A.Y - B.Y); }
	static inline float Len2(const FV2& A) { return FMath::Sqrt(A.X * A.X + A.Y * A.Y); }
	static const FV3 ElemUp(0.f, 1.f, 0.f);

	// ---- профили (o, h), против часовой стрелки вокруг тела; замыкающий отрезок — прилегание к стене ----
	static const FV2 PCrown[] = {
		FV2(0.f, 0.f), FV2(.03f, 0.f), FV2(.03f, .03f), FV2(.05f, .04f), FV2(.07f, .06f), FV2(.08f, .08f), FV2(.09f, .10f),   // полочка и четвертной вал
		FV2(.12f, .10f), FV2(.12f, .14f), FV2(.14f, .15f),                                                                   // сухариковый пояс
		FV2(.36f, .17f), FV2(.36f, .28f), FV2(.38f, .28f), FV2(.38f, .30f),                                                   // слезник с полочкой
		FV2(.395f, .32f), FV2(.41f, .345f), FV2(.425f, .37f), FV2(.437f, .39f), FV2(.445f, .41f),                             // гусёк
		FV2(.45f, .41f), FV2(.45f, .45f), FV2(0.f, .45f) };
	static const FV2 PString[] = {
		FV2(0.f, 0.f), FV2(.08f, 0.f), FV2(.08f, .02f), FV2(.11f, .03f), FV2(.14f, .05f), FV2(.16f, .08f), FV2(.17f, .10f),
		FV2(.20f, .10f), FV2(.20f, .17f), FV2(.17f, .20f), FV2(0.f, .20f) };
	static const FV2 PSill[] = { FV2(0.f, 0.f), FV2(.10f, 0.f), FV2(.10f, .015f), FV2(.12f, .03f), FV2(.12f, .07f), FV2(0.f, .10f) };
	static const FV2 PArchitrave[] = {
		FV2(.16f, 0.f), FV2(.16f, .03f), FV2(.15f, .035f), FV2(.13f, .04f), FV2(.12f, .045f), FV2(.07f, .05f), FV2(.06f, .06f),
		FV2(0.f, .06f), FV2(0.f, 0.f) };
	static const FV2 PPilBase[] = {
		FV2(0.f, 0.f), FV2(.06f, 0.f), FV2(.06f, .07f), FV2(.05f, .08f), FV2(.045f, .09f), FV2(.045f, .11f), FV2(.035f, .125f),
		FV2(.02f, .135f), FV2(.01f, .15f), FV2(0.f, .16f) };
	static const FV2 PPilCap[] = {
		FV2(0.f, 0.f), FV2(.015f, 0.f), FV2(.015f, .025f), FV2(.02f, .04f), FV2(.035f, .07f), FV2(.045f, .10f), FV2(.06f, .12f),
		FV2(.06f, .20f), FV2(0.f, .20f) };
	static const FV2 PPlinth[] = { FV2(0.f, 0.f), FV2(.08f, 0.f), FV2(.08f, .45f), FV2(.04f, .53f), FV2(0.f, .55f) };
	static const FV2 PSash[] = { FV2(0.f, .06f), FV2(-.06f, .06f), FV2(-.06f, 0.f) };
	static const FV2 PConsole[] = {
		FV2(0.f, 0.f), FV2(.06f, 0.f), FV2(.08f, .04f), FV2(.10f, .10f), FV2(.14f, .17f), FV2(.20f, .24f), FV2(.30f, .31f),
		FV2(.42f, .36f), FV2(.56f, .40f), FV2(.66f, .42f), FV2(.72f, .44f), FV2(.75f, .47f), FV2(.75f, .50f), FV2(0.f, .50f) };

	template<int32 N> static TPCity::FProfile MakeProfile(const FV2 (&A)[N]) { TPCity::FProfile P; P.Pts = A; P.Num = N; return P; }

	/** прямоугольник в плоскости фасада на глубине D, лицом наружу */
	static void FacadeRect(TPCity::FMeshBuf& M, const TPCity::FFacadeFrame& F, float U0, float U1, float V0, float V1, float D, float UVS)
	{
		if (U1 - U0 < 1e-4f || V1 - V0 < 1e-4f) return;
		TPCity::OrientedQuad(M, F.P(U0, V0, D), F.P(U1, V0, D), F.P(U1, V1, D), F.P(U0, V1, D),
			FV2(U0 * UVS, V0 * UVS), FV2(U1 * UVS, V0 * UVS), FV2(U1 * UVS, V1 * UVS), FV2(U0 * UVS, V1 * UVS), F.N);
	}

	/** треугольник с ожидаемой нормалью */
	static void OrientedTri(TPCity::FMeshBuf& M, const FV3& A, const FV3& B, const FV3& C, FV2 UA, FV2 UB, FV2 UC, const FV3& Expected)
	{
		FV3 N = Cross3(B - A, C - A);
		const float L = Len3(N);
		if (L < 1e-10f) return;
		N = N * (1.f / L);
		if (Dot3(N, Expected) < 0.f)
		{
			N = -N;
			const int32 I0 = M.Vertex(A, N, UA), I1 = M.Vertex(C, N, UC), I2 = M.Vertex(B, N, UB);
			M.Tri(I0, I1, I2);
		}
		else
		{
			const int32 I0 = M.Vertex(A, N, UA), I1 = M.Vertex(B, N, UB), I2 = M.Vertex(C, N, UC);
			M.Tri(I0, I1, I2);
		}
	}

	/** брусок в плоскости фасада между точками P0, P1 (u, v) шириной W, от глубины D0 до D1: лицо и два бока */
	static void PlaneBar(TPCity::FMeshBuf& M, const TPCity::FFacadeFrame& F, FV2 P0, FV2 P1, float W, float D0, float D1, float UVS)
	{
		const FV2 T = Sub2(P1, P0);
		const float L = Len2(T);
		if (L < 1e-3f) return;
		const FV2 Tn(T.X / L, T.Y / L), Nn(-Tn.Y, Tn.X);
		const float H = W * 0.5f;
		const FV2 A0(P0.X - Nn.X * H, P0.Y - Nn.Y * H), A1(P1.X - Nn.X * H, P1.Y - Nn.Y * H);
		const FV2 B0(P0.X + Nn.X * H, P0.Y + Nn.Y * H), B1(P1.X + Nn.X * H, P1.Y + Nn.Y * H);
		const FV3 SideB = F.R * Nn.X + ElemUp * Nn.Y;   // наружная нормаль бока B
		TPCity::OrientedQuad(M, F.P(A0.X, A0.Y, D1), F.P(A1.X, A1.Y, D1), F.P(B1.X, B1.Y, D1), F.P(B0.X, B0.Y, D1),
			FV2(0.f, 0.f), FV2(L * UVS, 0.f), FV2(L * UVS, W * UVS), FV2(0.f, W * UVS), F.N);
		TPCity::OrientedQuad(M, F.P(B0.X, B0.Y, D0), F.P(B1.X, B1.Y, D0), F.P(B1.X, B1.Y, D1), F.P(B0.X, B0.Y, D1),
			FV2(0.f, 0.f), FV2(L * UVS, 0.f), FV2(L * UVS, (D1 - D0) * UVS), FV2(0.f, (D1 - D0) * UVS), SideB);
		TPCity::OrientedQuad(M, F.P(A0.X, A0.Y, D0), F.P(A1.X, A1.Y, D0), F.P(A1.X, A1.Y, D1), F.P(A0.X, A0.Y, D1),
			FV2(0.f, 0.f), FV2(L * UVS, 0.f), FV2(L * UVS, (D1 - D0) * UVS), FV2(0.f, (D1 - D0) * UVS), -SideB);
	}

	/** точки ленточного руста (o, v) для полосы [V0, V1]: от стены внизу, по лицу камней, к стене вверху */
	static void RustProfile(TArray<FV2>& Out, float V0, float V1, float Base, float Course, float Proj, float Chamfer)
	{
		Out.Reset();
		if (V1 - V0 < 1e-3f || Course < 0.05f) return;
		const float C = FMath::Min(Chamfer, Course * 0.3f);
		auto Push = [&Out](float O, float V)
		{
			if (Out.Num() > 0 && FMath::Abs(Out.Last().X - O) < 1e-5f && FMath::Abs(Out.Last().Y - V) < 1e-5f) return;
			Out.Add(FV2(O, V));
		};
		// вынос камня как функция высоты: фаска снизу, плоскость, фаска сверху (на шве — 0)
		auto OAt = [Base, Course, Proj, C](float V)
		{
			float L = V - (Base + FMath::FloorToFloat((V - Base) / Course) * Course);
			L = FMath::Clamp(L, 0.f, Course);
			if (L < C) return Proj * L / C;
			if (L > Course - C) return Proj * (Course - L) / C;
			return Proj;
		};
		Push(0.f, V0);
		Push(OAt(V0), V0);
		const int32 K0 = FMath::FloorToInt((V0 - Base) / Course), K1 = FMath::FloorToInt((V1 - Base) / Course);
		for (int32 K = K0; K <= K1; ++K)
		{
			const float VB = Base + (float)K * Course;
			const float Brk[3] = { VB + C, VB + Course - C, VB + Course };
			const float Ob[3] = { Proj, Proj, 0.f };
			for (int32 I = 0; I < 3; ++I)
			{
				if (Brk[I] > V0 + 1e-4f && Brk[I] < V1 - 1e-4f) Push(Ob[I], Brk[I]);
			}
		}
		Push(OAt(V1 - 1e-6f), V1);
		Push(0.f, V1);
	}
}

namespace TPCity
{
using namespace TPCityElemImpl;

FProfile GetProfile(EProfile P)
{
	switch (P)
	{
	case EProfile::CrownCornice: return MakeProfile(PCrown);
	case EProfile::StringCourse: return MakeProfile(PString);
	case EProfile::Sill: return MakeProfile(PSill);
	case EProfile::Architrave: return MakeProfile(PArchitrave);
	case EProfile::PilasterBase: return MakeProfile(PPilBase);
	case EProfile::PilasterCapital: return MakeProfile(PPilCap);
	case EProfile::Plinth: return MakeProfile(PPlinth);
	case EProfile::Sash: return MakeProfile(PSash);
	case EProfile::Console: return MakeProfile(PConsole);
	default: return FProfile();
	}
}

bool ProfileIsCCW(const FProfile& P)
{
	float A = 0.f;
	for (int32 I = 0; I < P.Num; ++I) A += Cross2(P.Pts[I], P.Pts[(I + 1) % P.Num]);
	return A > 0.f;
}

void OrientedQuad(FMeshBuf& M, const FV3& A, const FV3& B, const FV3& C, const FV3& D, FV2 UA, FV2 UB, FV2 UC, FV2 UD, const FV3& Expected)
{
	FV3 N = Cross3(C - A, D - B);   // для обхода A-B-C-D против часовой стрелки смотрит на зрителя
	const float L = Len3(N);
	if (L < 1e-10f) return;
	N = N * (1.f / L);
	if (Dot3(N, Expected) < 0.f)
	{
		N = -N;
		const int32 I0 = M.Vertex(A, N, UA), I1 = M.Vertex(D, N, UD), I2 = M.Vertex(C, N, UC), I3 = M.Vertex(B, N, UB);
		M.Tri(I0, I1, I2); M.Tri(I0, I2, I3);
	}
	else
	{
		const int32 I0 = M.Vertex(A, N, UA), I1 = M.Vertex(B, N, UB), I2 = M.Vertex(C, N, UC), I3 = M.Vertex(D, N, UD);
		M.Tri(I0, I1, I2); M.Tri(I0, I2, I3);
	}
}

void TriangulatePolygon(const FV2* P, int32 N, TArray<int32>& Out)
{
	Out.Reset();
	if (N < 3) return;
	TArray<int32> V;
	for (int32 I = 0; I < N; ++I) V.Add(I);
	int32 Guard = 4 * N * N;
	while (V.Num() > 3 && Guard-- > 0)
	{
		const int32 Cnt = V.Num();
		bool bFound = false;
		int32 Collinear = -1;
		for (int32 I = 0; I < Cnt && !bFound; ++I)
		{
			const int32 IA = V[(I + Cnt - 1) % Cnt], IB = V[I], IC = V[(I + 1) % Cnt];
			const FV2 A = P[IA], B = P[IB], C = P[IC];
			const float Cr = Cross2(Sub2(B, A), Sub2(C, B));
			if (Cr <= 1e-9f)
			{
				if (FMath::Abs(Cr) <= 1e-9f && Collinear < 0) Collinear = I;
				continue;   // вогнутая или вырожденная вершина
			}
			bool bInside = false;
			for (int32 K = 0; K < Cnt && !bInside; ++K)
			{
				const int32 IK = V[K];
				if (IK == IA || IK == IB || IK == IC) continue;
				const FV2 Q = P[IK];
				if ((Q.X == A.X && Q.Y == A.Y) || (Q.X == B.X && Q.Y == B.Y) || (Q.X == C.X && Q.Y == C.Y)) continue;
				const float D0 = Cross2(Sub2(B, A), Sub2(Q, A)), D1 = Cross2(Sub2(C, B), Sub2(Q, B)), D2 = Cross2(Sub2(A, C), Sub2(Q, C));
				bInside = D0 >= -1e-9f && D1 >= -1e-9f && D2 >= -1e-9f;
			}
			if (bInside) continue;
			Out.Add(IA); Out.Add(IB); Out.Add(IC);
			V.RemoveAt(I);
			bFound = true;
		}
		if (!bFound)
		{
			if (Collinear >= 0) { V.RemoveAt(Collinear); continue; }   // точка на прямой — без треугольника
			break;
		}
	}
	if (V.Num() == 3)
	{
		const float Cr = Cross2(Sub2(P[V[1]], P[V[0]]), Sub2(P[V[2]], P[V[1]]));
		if (FMath::Abs(Cr) > 1e-9f) { Out.Add(V[0]); Out.Add(V[1]); Out.Add(V[2]); }
	}
	else if (V.Num() > 3)
	{
		for (int32 I = 1; I + 1 < V.Num(); ++I) { Out.Add(V[0]); Out.Add(V[I]); Out.Add(V[I + 1]); }   // запасной веер
	}
}

void SweepProfile(FMeshBuf& M, const FSweepBasis& B, const FV2* Path, int32 NPath, const FV2* Prof, int32 NProf, const FSweepOpts& O)
{
	if (NPath < 2 || NProf < 2 || !Path || !Prof) return;
	const bool bClosed = O.bClosed && NPath >= 3;
	const int32 NSeg = bClosed ? NPath : NPath - 1;
	TArray<FV2> SegT, SegN;
	TArray<float> PathU;
	SegT.SetNum(NSeg); SegN.SetNum(NSeg); PathU.SetNum(NSeg + 1);
	PathU[0] = 0.f;
	for (int32 S = 0; S < NSeg; ++S)
	{
		const FV2 D = Sub2(Path[(S + 1) % NPath], Path[S]);
		const float L = Len2(D);
		SegT[S] = L > 1e-6f ? FV2(D.X / L, D.Y / L) : (S > 0 ? SegT[S - 1] : FV2(1.f, 0.f));
		SegN[S] = FV2(-SegT[S].Y, SegT[S].X);   // слева от направления — наружу
		PathU[S + 1] = PathU[S] + L;
	}
	// усовые векторы в вершинах пути
	TArray<FV2> Miter;
	Miter.SetNum(NPath);
	for (int32 K = 0; K < NPath; ++K)
	{
		const bool bHasPrev = bClosed || K > 0, bHasNext = bClosed || K < NPath - 1;
		const FV2 NP = bHasPrev ? SegN[(K + NSeg - 1) % NSeg] : SegN[0];
		const FV2 NN = bHasNext ? SegN[K % NSeg] : SegN[NSeg - 1];
		const float Den = FMath::Max(1.f + NP.X * NN.X + NP.Y * NN.Y, 0.2f);   // острые углы — с ограничением
		Miter[K] = FV2((NP.X + NN.X) / Den, (NP.Y + NN.Y) / Den);
	}
	// профиль в масштабе и его длина
	TArray<FV2> Q;
	TArray<float> ProfV;
	Q.SetNum(NProf); ProfV.SetNum(NProf);
	for (int32 J = 0; J < NProf; ++J)
	{
		Q[J] = FV2(Prof[J].X * O.ScaleO, Prof[J].Y * O.ScaleH + O.OffsetH);
		ProfV[J] = J == 0 ? 0.f : ProfV[J - 1] + Len2(Sub2(Q[J], Q[J - 1]));
	}
	auto World = [&B, Path, &Miter, &Q](int32 K, int32 J)
	{
		const float PX = Path[K].X + Miter[K].X * Q[J].X, PY = Path[K].Y + Miter[K].Y * Q[J].X;
		return B.Origin + B.AxX * PX + B.AxY * PY + B.AxH * Q[J].Y;
	};
	M.Color = O.Col;
	const float S = O.UVS;
	for (int32 Sg = 0; Sg < NSeg; ++Sg)
	{
		const int32 K0 = Sg, K1 = (Sg + 1) % NPath;
		if (PathU[Sg + 1] - PathU[Sg] < 1e-6f) continue;
		const FV3 NW = B.AxX * SegN[Sg].X + B.AxY * SegN[Sg].Y;
		for (int32 J = 0; J + 1 < NProf; ++J)
		{
			const float DO = Q[J + 1].X - Q[J].X, DH = Q[J + 1].Y - Q[J].Y;
			if (FMath::Abs(DO) + FMath::Abs(DH) < 1e-6f) continue;
			const FV3 Expected = NW * DH - B.AxH * DO;
			const float VA = (O.bPlanarUV ? Q[J].Y : ProfV[J]) * S, VB = (O.bPlanarUV ? Q[J + 1].Y : ProfV[J + 1]) * S;
			OrientedQuad(M, World(K0, J), World(K1, J), World(K1, J + 1), World(K0, J + 1),
				FV2(PathU[Sg] * S, VA), FV2(PathU[Sg + 1] * S, VA), FV2(PathU[Sg + 1] * S, VB), FV2(PathU[Sg] * S, VB),
				Expected);
		}
	}
	if (bClosed || (!O.bCapStart && !O.bCapEnd)) return;
	TArray<int32> Tris;
	TriangulatePolygon(Q.GetData(), NProf, Tris);
	for (int32 E = 0; E < 2; ++E)
	{
		if ((E == 0 && !O.bCapStart) || (E == 1 && !O.bCapEnd)) continue;
		const int32 K = E == 0 ? 0 : NPath - 1;
		const FV2 T = E == 0 ? SegT[0] : SegT[NSeg - 1];
		const FV3 TW = B.AxX * T.X + B.AxY * T.Y;
		const FV3 Expected = E == 0 ? -TW : TW;
		for (int32 I = 0; I + 2 < Tris.Num(); I += 3)
		{
			const int32 A = Tris[I], Bi = Tris[I + 1], C = Tris[I + 2];
			OrientedTri(M, World(K, A), World(K, Bi), World(K, C), FV2(Q[A].X * S, Q[A].Y * S), FV2(Q[Bi].X * S, Q[Bi].Y * S), FV2(Q[C].X * S, Q[C].Y * S), Expected);
		}
	}
}

void FacadeBox(FMeshBuf& M, const FFacadeFrame& F, float U0, float U1, float V0, float V1, float D0, float D1, float UVS, int32 Faces)
{
	FV3 P[8];
	for (int32 I = 0; I < 8; ++I) P[I] = F.P((I & 1) ? U1 : U0, (I & 2) ? V1 : V0, (I & 4) ? D1 : D0);
	const float SU0 = U0 * UVS, SU1 = U1 * UVS, SV0 = V0 * UVS, SV1 = V1 * UVS, SD0 = D0 * UVS, SD1 = D1 * UVS;
	if (Faces & BF_Front) OrientedQuad(M, P[4], P[5], P[7], P[6], FV2(SU0, SV0), FV2(SU1, SV0), FV2(SU1, SV1), FV2(SU0, SV1), F.N);
	if (Faces & BF_Back) OrientedQuad(M, P[0], P[1], P[3], P[2], FV2(SU0, SV0), FV2(SU1, SV0), FV2(SU1, SV1), FV2(SU0, SV1), -F.N);
	if (Faces & BF_Top) OrientedQuad(M, P[2], P[3], P[7], P[6], FV2(SU0, SD0), FV2(SU1, SD0), FV2(SU1, SD1), FV2(SU0, SD1), ElemUp);
	if (Faces & BF_Bottom) OrientedQuad(M, P[0], P[1], P[5], P[4], FV2(SU0, SD0), FV2(SU1, SD0), FV2(SU1, SD1), FV2(SU0, SD1), -ElemUp);
	if (Faces & BF_Left) OrientedQuad(M, P[0], P[4], P[6], P[2], FV2(SD0, SV0), FV2(SD1, SV0), FV2(SD1, SV1), FV2(SD0, SV1), -F.R);
	if (Faces & BF_Right) OrientedQuad(M, P[1], P[5], P[7], P[3], FV2(SD0, SV0), FV2(SD1, SV0), FV2(SD1, SV1), FV2(SD0, SV1), F.R);
}

// ---------------------------------------------------------------- проём

float FOpening::ArcRadius() const
{
	if (!IsArched()) return 0.f;
	const float HW = W * 0.5f;
	return (HW * HW + Rise * Rise) / (2.f * Rise);
}

float FOpening::ArcCenterV() const
{
	return Spring() + Rise - ArcRadius();
}

float FOpening::TopAt(float U) const
{
	if (!IsArched() || U <= U0() || U >= U1()) return Spring();
	const float R = ArcRadius(), X = U - UC;
	return ArcCenterV() + FMath::Sqrt(FMath::Max(R * R - X * X, 0.f));
}

int32 FOpening::ArcSegments() const
{
	if (!IsArched()) return 1;
	const float R = ArcRadius();
	const float Half = FMath::Asin(FMath::Min(W * 0.5f / R, 1.f));
	const float Span = Rise > R ? 2.f * ElemPi - 2.f * Half : 2.f * Half;   // стрела больше радиуса не бывает при Rise ≤ W/2
	return FMath::Clamp(FMath::CeilToInt(Span / (ElemPi / 12.f)), 4, 16);
}

void FOpening::ArcPoints(TArray<FV2>& Out) const
{
	Out.Reset();
	if (!IsArched())
	{
		Out.Add(FV2(U0(), Spring()));
		Out.Add(FV2(U1(), Spring()));
		return;
	}
	const float R = ArcRadius(), CV = ArcCenterV();
	const float Half = FMath::Asin(FMath::Min(W * 0.5f / R, 1.f));
	const int32 Seg = ArcSegments();
	for (int32 I = 0; I <= Seg; ++I)
	{
		const float A = ElemPi * 0.5f + Half - 2.f * Half * (float)I / (float)Seg;
		Out.Add(FV2(UC + R * FMath::Cos(A), CV + R * FMath::Sin(A)));
	}
	Out[0] = FV2(U0(), Spring());
	Out.Last() = FV2(U1(), Spring());
}

void FOpening::Outline(TArray<FV2>& Out) const
{
	TArray<FV2> Arc;
	ArcPoints(Arc);
	Out.Reset();
	Out.Add(FV2(U0(), V0));
	for (const FV2& P : Arc) Out.Add(P);
	Out.Add(FV2(U1(), V0));
}

void FOpening::OutlineOpen(TArray<FV2>& Out) const
{
	Outline(Out);   // тот же список точек; разница — путь не замыкается
}

// ---------------------------------------------------------------- элементы фасада

void Moulding(FMeshBuf& M, const FFacadeFrame& F, float U0, float U1, float V, EProfile P, float Scale, bool bCaps, FColor Col, float UVS)
{
	const FProfile Pr = GetProfile(P);
	const FV2 Path[2] = { FV2(U0, 0.f), FV2(U1, 0.f) };
	FSweepOpts O;
	O.ScaleO = O.ScaleH = Scale; O.OffsetH = V; O.UVS = UVS; O.bCapStart = O.bCapEnd = bCaps; O.Col = Col;
	SweepProfile(M, FSweepBasis(F.O, F.R, F.N, ElemUp), Path, 2, Pr.Pts, Pr.Num, O);
}

void MouldingLoop(FMeshBuf& M, float X0, float Z0, float X1, float Z1, float Y, EProfile P, float Scale, FColor Col, float UVS)
{
	const FProfile Pr = GetProfile(P);
	// обход так, чтобы снаружи было слева: X0 → +Z → X1 → −Z
	const FV2 Path[4] = { FV2(X0, Z0), FV2(X0, Z1), FV2(X1, Z1), FV2(X1, Z0) };
	FSweepOpts O;
	O.ScaleO = O.ScaleH = Scale; O.OffsetH = Y; O.UVS = UVS; O.bClosed = true; O.Col = Col;
	SweepProfile(M, FSweepBasis(FV3(0.f, 0.f, 0.f), FV3(1.f, 0.f, 0.f), FV3(0.f, 0.f, 1.f), ElemUp), Path, 4, Pr.Pts, Pr.Num, O);
}

void WallAroundOpening(FMeshBuf& M, const FFacadeFrame& F, const FOpening& O, float WU0, float WU1, float WV0, float WV1, float UVS)
{
	FacadeRect(M, F, WU0, O.U0(), WV0, WV1, 0.f, UVS);
	FacadeRect(M, F, O.U1(), WU1, WV0, WV1, 0.f, UVS);
	FacadeRect(M, F, O.U0(), O.U1(), WV0, O.V0, 0.f, UVS);
	if (!O.IsArched())
	{
		FacadeRect(M, F, O.U0(), O.U1(), O.Spring(), WV1, 0.f, UVS);
		return;
	}
	TArray<FV2> Arc;
	O.ArcPoints(Arc);
	for (int32 I = 0; I + 1 < Arc.Num(); ++I)
	{
		const FV2 A = Arc[I], B = Arc[I + 1];
		OrientedQuad(M, F.P(A.X, A.Y, 0.f), F.P(B.X, B.Y, 0.f), F.P(B.X, WV1, 0.f), F.P(A.X, WV1, 0.f),
			FV2(A.X * UVS, A.Y * UVS), FV2(B.X * UVS, B.Y * UVS), FV2(B.X * UVS, WV1 * UVS), FV2(A.X * UVS, WV1 * UVS), F.N);
	}
}

void Reveal(FMeshBuf& M, const FFacadeFrame& F, const FOpening& O, float Depth, float UVS, bool bBottom)
{
	TArray<FV2> L;
	O.Outline(L);
	float Acc = 0.f;
	const int32 N = L.Num();
	for (int32 I = 0; I < N; ++I)
	{
		const bool bBottomSeg = I == N - 1;   // замыкающий отрезок (U1, V0) → (U0, V0)
		if (bBottomSeg && !bBottom) continue;
		const FV2 A = L[I], B = L[(I + 1) % N];
		const FV2 D = Sub2(B, A);
		const float Len = Len2(D);
		if (Len < 1e-4f) continue;
		const FV2 Inward(D.Y / Len, -D.X / Len);   // справа от направления — внутрь проёма
		const FV3 Expected = F.R * Inward.X + ElemUp * Inward.Y;
		OrientedQuad(M, F.P(A.X, A.Y, 0.f), F.P(B.X, B.Y, 0.f), F.P(B.X, B.Y, -Depth), F.P(A.X, A.Y, -Depth),
			FV2(Acc * UVS, 0.f), FV2((Acc + Len) * UVS, 0.f), FV2((Acc + Len) * UVS, Depth * UVS), FV2(Acc * UVS, Depth * UVS), Expected);
		Acc += Len;
	}
}

void OpeningFill(FMeshBuf& M, const FFacadeFrame& F, const FOpening& O, float D, float UVS)
{
	FacadeRect(M, F, O.U0(), O.U1(), O.V0, O.Spring(), D, UVS);
	if (!O.IsArched()) return;
	TArray<FV2> Arc;
	O.ArcPoints(Arc);
	const FV2 C(O.UC, O.Spring());
	for (int32 I = 0; I + 1 < Arc.Num(); ++I)
	{
		const FV2 A = Arc[I], B = Arc[I + 1];
		OrientedTri(M, F.P(C.X, C.Y, D), F.P(A.X, A.Y, D), F.P(B.X, B.Y, D), FV2(C.X * UVS, C.Y * UVS), FV2(A.X * UVS, A.Y * UVS), FV2(B.X * UVS, B.Y * UVS), F.N);
	}
}

void Surround(FMeshBuf& M, const FFacadeFrame& F, const FOpening& O, float Scale, bool bBottom, FColor Col, float UVS)
{
	TArray<FV2> L;
	O.Outline(L);
	const FProfile Pr = GetProfile(EProfile::Architrave);
	FSweepOpts Op;
	Op.ScaleO = Op.ScaleH = Scale; Op.UVS = UVS; Op.Col = Col;
	Op.bClosed = bBottom;
	Op.bCapStart = Op.bCapEnd = !bBottom;
	SweepProfile(M, FSweepBasis(F.O, F.R, ElemUp, F.N), L, Pr, Op);
}

void Keystone(FMeshBuf& M, const FFacadeFrame& F, const FOpening& O, float Height, float Proj, FColor Col, float UVS)
{
	M.Color = Col;
	const float VB = O.Top() - 0.03f, VT = VB + Height;
	const float WB = 0.09f, WT = 0.13f;   // полуширины снизу и сверху
	const FV3 B0 = F.P(O.UC - WB, VB, 0.f), B1 = F.P(O.UC + WB, VB, 0.f), T1 = F.P(O.UC + WT, VT, 0.f), T0 = F.P(O.UC - WT, VT, 0.f);
	const FV3 FB0 = F.P(O.UC - WB, VB, Proj), FB1 = F.P(O.UC + WB, VB, Proj), FT1 = F.P(O.UC + WT, VT, Proj), FT0 = F.P(O.UC - WT, VT, Proj);
	const FV2 Z(0.f, 0.f);
	const float S = UVS;
	OrientedQuad(M, FB0, FB1, FT1, FT0, FV2(-WB * S, 0.f), FV2(WB * S, 0.f), FV2(WT * S, Height * S), FV2(-WT * S, Height * S), F.N);
	OrientedQuad(M, B0, B1, FB1, FB0, Z, FV2(2.f * WB * S, 0.f), FV2(2.f * WB * S, Proj * S), FV2(0.f, Proj * S), -ElemUp);
	OrientedQuad(M, T0, T1, FT1, FT0, Z, FV2(2.f * WT * S, 0.f), FV2(2.f * WT * S, Proj * S), FV2(0.f, Proj * S), ElemUp);
	const FV3 SideR = Norm3(F.R * Height - ElemUp * (WT - WB));
	const FV3 SideL = Norm3(-F.R * Height - ElemUp * (WT - WB));
	OrientedQuad(M, B1, T1, FT1, FB1, Z, FV2(Height * S, 0.f), FV2(Height * S, Proj * S), FV2(0.f, Proj * S), SideR);
	OrientedQuad(M, B0, T0, FT0, FB0, Z, FV2(Height * S, 0.f), FV2(Height * S, Proj * S), FV2(0.f, Proj * S), SideL);
}

void Sash(FMeshBuf& M, const FFacadeFrame& F, const FOpening& O, float D, int32 Rows, int32 Cols, bool bDoor, FColor Col)
{
	M.Color = Col;
	TArray<FV2> L;
	O.Outline(L);
	const FProfile Pr = GetProfile(EProfile::Sash);
	FSweepOpts Op;
	Op.UVS = 1.f; Op.Col = Col; Op.bClosed = true;
	SweepProfile(M, FSweepBasis(F.O + F.N * D, F.R, ElemUp, F.N), L, Pr, Op);
	if (bDoor) return;
	const float FW = 0.06f, BW = 0.03f, BD = 0.035f, MW = 0.06f;   // рама, горбылёк, его глубина, средник
	const float IU0 = O.U0() + FW, IU1 = O.U1() - FW, IV0 = O.V0 + FW;
	M.Color = Col;
	// верх створок: импост на пяте арки или фрамуга у высоких прямоугольных окон
	float LeafTop = O.IsArched() ? O.Spring() : O.Spring() - FW;
	bool bTransom = O.IsArched();
	if (!O.IsArched() && O.H > 2.3f)
	{
		LeafTop = O.Spring() - FMath::Min(0.7f, O.H * 0.25f);
		bTransom = true;
	}
	if (bTransom) PlaneBar(M, F, FV2(IU0, LeafTop), FV2(IU1, LeafTop), FW, D, D + 0.06f, 1.f);
	PlaneBar(M, F, FV2(O.UC, IV0), FV2(O.UC, bTransom ? LeafTop : O.Spring() - FW), MW, D, D + 0.055f, 1.f);   // средник
	// горбыльки: ряды стёкол в створках
	const int32 NR = FMath::Max(Rows, 1);
	for (int32 K = 1; K < NR; ++K)
	{
		const float V = IV0 + (LeafTop - IV0) * (float)K / (float)NR;
		PlaneBar(M, F, FV2(IU0, V), FV2(IU1, V), BW, D, D + BD, 1.f);
	}
	if (Cols > 1)
	{
		for (int32 Leaf = 0; Leaf < 2; ++Leaf)
		{
			const float LU0 = Leaf ? O.UC + MW * 0.5f : IU0, LU1 = Leaf ? IU1 : O.UC - MW * 0.5f;
			for (int32 C = 1; C < Cols; ++C)
			{
				const float U = LU0 + (LU1 - LU0) * (float)C / (float)Cols;
				PlaneBar(M, F, FV2(U, IV0), FV2(U, LeafTop), BW, D, D + BD, 1.f);
			}
		}
	}
	if (!bTransom) return;
	if (O.IsArched())
	{
		// веер в арке: лучи из середины импоста
		const FV2 C(O.UC, LeafTop);
		const bool bSemi = O.Rise > O.W * 0.4f;
		const int32 NRay = bSemi ? 3 : 1;
		const float R = O.ArcRadius(), CV = O.ArcCenterV();
		const float Half = FMath::Asin(FMath::Min(O.W * 0.5f / R, 1.f));
		for (int32 I = 0; I < NRay; ++I)
		{
			const float A = ElemPi * 0.5f + Half - 2.f * Half * (float)(I + 1) / (float)(NRay + 1);
			const FV2 P(O.UC + (R - FW) * FMath::Cos(A), CV + (R - FW) * FMath::Sin(A));
			PlaneBar(M, F, C, P, BW, D, D + BD, 1.f);
		}
	}
	else
	{
		// фрамуга: два вертикальных горбылька
		for (int32 K = 1; K < 3; ++K)
		{
			const float U = IU0 + (IU1 - IU0) * (float)K / 3.f;
			PlaneBar(M, F, FV2(U, LeafTop), FV2(U, O.Spring() - FW), BW, D, D + BD, 1.f);
		}
	}
}

void Rustication(FMeshBuf& M, const FFacadeFrame& F, float U0, float U1, float V0, float V1, float Base, float Course, float Proj, float Chamfer, bool bCapL, bool bCapR, float UVS)
{
	if (U1 - U0 < 1e-3f || V1 - V0 < 1e-3f) return;
	TArray<FV2> Prof;
	RustProfile(Prof, V0, V1, Base, Course, Proj, Chamfer);
	if (Prof.Num() < 2) return;
	const FV2 Path[2] = { FV2(U0, 0.f), FV2(U1, 0.f) };
	FSweepOpts O;
	O.UVS = UVS; O.bCapStart = bCapL; O.bCapEnd = bCapR; O.Col = M.Color; O.bPlanarUV = true;
	// по камню: профиль касается стены на каждом шве, поэтому торец целиком — не простой многоугольник
	int32 From = 0;
	for (int32 I = 1; I < Prof.Num(); ++I)
	{
		if (Prof[I].X > 1e-6f && I < Prof.Num() - 1) continue;
		if (I - From >= 1) SweepProfile(M, FSweepBasis(F.O, F.R, F.N, ElemUp), Path, 2, Prof.GetData() + From, I - From + 1, O);
		From = I;
	}
}

void RusticatedWall(FMeshBuf& M, const FFacadeFrame& F, const FOpening* O, float WU0, float WU1, float WV0, float WV1, float Base, float Course, float UVS)
{
	const float Proj = 0.04f, Ch = 0.035f;
	if (!O)
	{
		Rustication(M, F, WU0, WU1, WV0, WV1, Base, Course, Proj, Ch, true, true, UVS);
		return;
	}
	Rustication(M, F, WU0, O->U0(), WV0, WV1, Base, Course, Proj, Ch, true, true, UVS);
	Rustication(M, F, O->U1(), WU1, WV0, WV1, Base, Course, Proj, Ch, true, true, UVS);
	Rustication(M, F, O->U0(), O->U1(), WV0, O->V0, Base, Course, Proj, Ch, false, false, UVS);
	if (!O->IsArched())
	{
		Rustication(M, F, O->U0(), O->U1(), O->Spring(), WV1, Base, Course, Proj, Ch, false, false, UVS);
		return;
	}
	// пазухи арки: полосы над отрезками дуги (низ — по верхней точке отрезка), под ними — гладкий треугольник
	TArray<FV2> Arc;
	O->ArcPoints(Arc);
	for (int32 I = 0; I + 1 < Arc.Num(); ++I)
	{
		const FV2 A = Arc[I], B = Arc[I + 1];
		const float VB = FMath::Max(A.Y, B.Y);
		Rustication(M, F, A.X, B.X, VB, WV1, Base, Course, Proj, Ch, true, true, UVS);
		const FV2 C = A.Y < B.Y ? FV2(A.X, VB) : FV2(B.X, VB);
		OrientedTri(M, F.P(A.X, A.Y, 0.f), F.P(B.X, B.Y, 0.f), F.P(C.X, C.Y, 0.f), FV2(A.X * UVS, A.Y * UVS), FV2(B.X * UVS, B.Y * UVS), FV2(C.X * UVS, C.Y * UVS), F.N);
	}
}

void Pilaster(FMeshBuf& M, const FFacadeFrame& F, float UC, float W, float Depth, float V0, float V1, FColor Col, float UVS)
{
	const float U0 = UC - W * 0.5f, U1 = UC + W * 0.5f;
	const FProfile PB = GetProfile(EProfile::PilasterBase), PC = GetProfile(EProfile::PilasterCapital);
	const FV2 Path[4] = { FV2(U0, 0.f), FV2(U0, Depth), FV2(U1, Depth), FV2(U1, 0.f) };
	const FSweepBasis Bs(F.O, F.R, F.N, ElemUp);
	FSweepOpts O;
	O.UVS = UVS; O.Col = Col; O.OffsetH = V0;
	SweepProfile(M, Bs, Path, 4, PB.Pts, PB.Num, O);
	const float CapH = 0.20f, BaseH = 0.16f;
	O.OffsetH = V1 - CapH;
	SweepProfile(M, Bs, Path, 4, PC.Pts, PC.Num, O);
	M.Color = Col;
	FacadeBox(M, F, U0, U1, V0 + BaseH, V1 - CapH, 0.f, Depth, UVS, BF_Front | BF_Left | BF_Right);
}

void Shutter(FMeshBuf& M, const FFacadeFrame& SF, float W, float H, float T, bool bDetail)
{
	const float S = 1.f / FMath::Max(H, 0.1f);   // одна ставня — один повтор текстуры жалюзи
	FacadeBox(M, SF, 0.f, W, 0.f, H, -T, T, S, BF_All);
	if (!bDetail) return;
	const float SW = 0.07f, RH = 0.09f, Rel = 0.012f;   // обвязка: бруски и выступ над полотном
	const int32 Fc = BF_Front | BF_Top | BF_Bottom | BF_Left | BF_Right;
	FacadeBox(M, SF, 0.f, SW, 0.f, H, T, T + Rel, S, Fc);
	FacadeBox(M, SF, W - SW, W, 0.f, H, T, T + Rel, S, Fc);
	FacadeBox(M, SF, SW, W - SW, 0.f, RH, T, T + Rel, S, Fc);
	FacadeBox(M, SF, SW, W - SW, H - RH, H, T, T + Rel, S, Fc);
	FacadeBox(M, SF, SW, W - SW, H * 0.5f - RH * 0.5f, H * 0.5f + RH * 0.5f, T, T + Rel, S, Fc);
}

void Dentils(FMeshBuf& M, const FFacadeFrame& F, float U0, float U1, float V0, float V1, float Proj, float W, float Step, float UVS)
{
	if (Step < W + 0.01f) return;
	const int32 N = FMath::FloorToInt((U1 - U0 - W) / Step) + 1;
	const float Start = U0 + (U1 - U0 - ((float)(N - 1) * Step + W)) * 0.5f;   // ровные поля по краям
	for (int32 I = 0; I < N; ++I)
	{
		const float U = Start + (float)I * Step;
		FacadeBox(M, F, U, U + W, V0, V1, 0.f, Proj, UVS, BF_Front | BF_Bottom | BF_Left | BF_Right);
	}
}

void Console(FMeshBuf& M, const FFacadeFrame& F, float UC, float W, float VTop, float Scale, FColor Col, float UVS)
{
	const FProfile Pr = GetProfile(EProfile::Console);
	const FV2 Path[2] = { FV2(UC - W * 0.5f, 0.f), FV2(UC + W * 0.5f, 0.f) };
	FSweepOpts O;
	O.ScaleO = O.ScaleH = Scale; O.OffsetH = VTop - 0.5f * Scale; O.UVS = UVS; O.bCapStart = O.bCapEnd = true; O.Col = Col;
	SweepProfile(M, FSweepBasis(F.O, F.R, F.N, ElemUp), Path, 2, Pr.Pts, Pr.Num, O);
}
}

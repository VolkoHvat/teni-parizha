// Тени Парижа — построитель квартала (порт lu_city.c + build_level из sandbox.c). Координаты Lutèce: метры, Y вверх.
// Отличия от оригинала (исправления): у верхних/нижних граней ниш, слуховых окон и коробок с зеркальной рамкой
// обход треугольников развёрнут так, чтобы лицевые стороны смотрели наружу (Cross(B − A, C − A) в правой тройке Lutèce;
// при выгрузке в Unreal порядок индексов не меняется — см. TPCityBuilder.h); добавлены потолок витрины, дверь из дерева,
// изнанка свесов двускатной крыши; предметы ставятся на фактическую высоту мостовой/тротуара.
#include "TPCityBuilder.h"

namespace TPCityImpl
{
	using TPCity::FV2;
	using TPCity::FV3;

	static constexpr float CityPi = 3.14159265358979f;
	static constexpr float CityTau = 6.28318530717959f;

	static inline float Dot(const FV3& A, const FV3& B) { return A.X * B.X + A.Y * B.Y + A.Z * B.Z; }
	static inline FV3 Cross(const FV3& A, const FV3& B) { return FV3(A.Y * B.Z - A.Z * B.Y, A.Z * B.X - A.X * B.Z, A.X * B.Y - A.Y * B.X); }
	static inline float Len(const FV3& A) { return FMath::Sqrt(Dot(A, A)); }
	static inline FV3 Norm(const FV3& A) { const float L = Len(A); return L > 1e-12f ? A * (1.f / L) : FV3(0.f, 0.f, 0.f); }
	static inline float Dist(const FV3& A, const FV3& B) { return Len(B - A); }
	static inline FV3 LerpV(const FV3& A, const FV3& B, float T) { return A + (B - A) * T; }
	static inline float LerpF(float A, float B, float T) { return A + (B - A) * T; }
	static inline FV3 VMin(const FV3& A, const FV3& B) { return FV3(FMath::Min(A.X, B.X), FMath::Min(A.Y, B.Y), FMath::Min(A.Z, B.Z)); }
	static inline FV3 VMax(const FV3& A, const FV3& B) { return FV3(FMath::Max(A.X, B.X), FMath::Max(A.Y, B.Y), FMath::Max(A.Z, B.Z)); }
	static inline float H01(uint32 H) { return (float)(H & 0xFFFFFFu) / 16777216.f; }
	static inline FColor C3(int32 R, int32 G, int32 B) { return FColor((uint8)R, (uint8)G, (uint8)B, 255); }
	static const FColor White(255, 255, 255, 255);

	/** поворот вектора вокруг единичной оси на угол (формула Родрига, как quat_axis в оригинале) */
	static FV3 Rotate(const FV3& K, float Angle, const FV3& V)
	{
		const float C = FMath::Cos(Angle), S = FMath::Sin(Angle);
		return V * C + Cross(K, V) * S + K * (Dot(K, V) * (1.f - C));
	}

	/** нормали по граням (с треугольника TriStart, только для вершин с VertStart) */
	static void ComputeNormals(TPCity::FMeshBuf& M, int32 VertStart, int32 TriStart)
	{
		for (int32 I = VertStart; I < M.Pos.Num(); ++I) M.Nrm[I] = FV3(0.f, 0.f, 0.f);
		for (int32 K = TriStart; K + 2 < M.Idx.Num(); K += 3)
		{
			const int32 I0 = M.Idx[K], I1 = M.Idx[K + 1], I2 = M.Idx[K + 2];
			if (I0 < VertStart || I1 < VertStart || I2 < VertStart) continue;
			const FV3 N = Cross(M.Pos[I1] - M.Pos[I0], M.Pos[I2] - M.Pos[I0]);
			M.Nrm[I0] = M.Nrm[I0] + N; M.Nrm[I1] = M.Nrm[I1] + N; M.Nrm[I2] = M.Nrm[I2] + N;
		}
		for (int32 I = VertStart; I < M.Pos.Num(); ++I)
		{
			const float L = Len(M.Nrm[I]);
			M.Nrm[I] = L > 1e-20f ? M.Nrm[I] * (1.f / L) : FV3(0.f, 1.f, 0.f);
		}
	}
}

namespace TPCity
{
// вспомогательные функции видны только внутри TPCity (без using на уровне файла — безопасно для unity-сборки)
using namespace TPCityImpl;

// ---------------------------------------------------------------- буфер и примитивы

int32 FMeshBuf::Vertex(const FV3& P, const FV3& N, const FV2& T)
{
	Pos.Add(P); Nrm.Add(N); UV.Add(T); Col.Add(Color);
	return Pos.Num() - 1;
}

void MbQuadBox(FMeshBuf& M, const FV3& A, const FV3& C, const FV3& D, const FV3& E, FV2 UV0, FV2 UV1)
{
	const FV3 N = Norm(Cross(C - A, E - A));
	const int32 I0 = M.Vertex(A, N, FV2(UV0.X, UV1.Y)), I1 = M.Vertex(C, N, FV2(UV1.X, UV1.Y));
	const int32 I2 = M.Vertex(D, N, FV2(UV1.X, UV0.Y)), I3 = M.Vertex(E, N, FV2(UV0.X, UV0.Y));
	M.Tri(I0, I1, I2); M.Tri(I0, I2, I3);
}

void MbBox(FMeshBuf& M, const FV3& Mn, const FV3& Mx, float S)
{
	FV3 P[8];
	for (int32 I = 0; I < 8; ++I) P[I] = FV3((I & 1) ? Mx.X : Mn.X, (I & 2) ? Mx.Y : Mn.Y, (I & 4) ? Mx.Z : Mn.Z);
	const float DX = (Mx.X - Mn.X) * S, DY = (Mx.Y - Mn.Y) * S, DZ = (Mx.Z - Mn.Z) * S;
	const float UX0 = Mn.X * S, UZ0 = Mn.Z * S, UY0 = Mn.Y * S;
	MbQuadBox(M, P[4], P[5], P[7], P[6], FV2(UX0, UY0), FV2(UX0 + DX, UY0 + DY));          // +Z
	MbQuadBox(M, P[1], P[0], P[2], P[3], FV2(-UX0 - DX, UY0), FV2(-UX0, UY0 + DY));        // −Z
	MbQuadBox(M, P[5], P[1], P[3], P[7], FV2(-UZ0 - DZ, UY0), FV2(-UZ0, UY0 + DY));        // +X
	MbQuadBox(M, P[0], P[4], P[6], P[2], FV2(UZ0, UY0), FV2(UZ0 + DZ, UY0 + DY));          // −X
	MbQuadBox(M, P[2], P[6], P[7], P[3], FV2(UX0, UZ0), FV2(UX0 + DX, UZ0 + DZ));          // +Y
	MbQuadBox(M, P[0], P[1], P[5], P[4], FV2(UX0, UZ0), FV2(UX0 + DX, UZ0 + DZ));          // −Y
}

void MbLathe(FMeshBuf& M, const FV3& Base, const FV2* Profile, int32 Count, int32 Seg, float UVPerMeter)
{
	const int32 Start = M.NumVerts(), TriStart = M.Idx.Num();
	float MaxR = 0.f;
	for (int32 I = 0; I < Count; ++I) MaxR = FMath::Max(MaxR, Profile[I].X);
	// по кругу укладываем целое число повторов текстуры, чтобы не было шва
	const float URep = UVPerMeter > 0.f ? FMath::Max(1.f, (float)FMath::RoundToInt(CityTau * MaxR * UVPerMeter)) : 1.f;
	const float VMul = UVPerMeter > 0.f ? UVPerMeter : 1.f;
	float VLen = 0.f;
	for (int32 I = 0; I < Count; ++I)
	{
		if (I)
		{
			const float DX = Profile[I].X - Profile[I - 1].X, DY = Profile[I].Y - Profile[I - 1].Y;
			VLen += FMath::Sqrt(DX * DX + DY * DY);
		}
		for (int32 S = 0; S <= Seg; ++S)
		{
			const float A = CityTau * (float)S / (float)Seg, SN = FMath::Sin(A), CS = FMath::Cos(A);
			M.Vertex(FV3(Base.X + Profile[I].X * SN, Base.Y + Profile[I].Y, Base.Z + Profile[I].X * CS), FV3(SN, 0.f, CS),
				FV2((float)S / (float)Seg * URep, VLen * VMul));
		}
	}
	for (int32 I = 0; I < Count - 1; ++I)
	{
		for (int32 S = 0; S < Seg; ++S)
		{
			const int32 A = Start + I * (Seg + 1) + S, C = A + Seg + 1;
			M.Tri(A, A + 1, C + 1); M.Tri(A, C + 1, C);
		}
	}
	ComputeNormals(M, Start, TriStart);
	// сварка нормалей на шве (первая и последняя вершина кольца совпадают)
	for (int32 I = 0; I < Count; ++I)
	{
		const int32 V0 = Start + I * (Seg + 1), V1 = V0 + Seg;
		const FV3 N = Norm(M.Nrm[V0] + M.Nrm[V1]);
		if (Len(N) > 0.5f) { M.Nrm[V0] = N; M.Nrm[V1] = N; }
	}
}

void MbCylinder(FMeshBuf& M, const FV3& Base, float R0, float R1, float H, int32 Seg, bool bCaps)
{
	const FV2 Pr[2] = { FV2(R0, 0.f), FV2(R1, H) };
	MbLathe(M, Base, Pr, 2, Seg);
	if (!bCaps) return;
	for (int32 Cap = 0; Cap < 2; ++Cap)
	{
		const float R = Cap ? R1 : R0, Y = Base.Y + (Cap ? H : 0.f);
		const FV3 N(0.f, Cap ? 1.f : -1.f, 0.f);
		const int32 Ctr = M.Vertex(FV3(Base.X, Y, Base.Z), N, FV2(0.5f, 0.5f)), First = M.NumVerts();
		for (int32 S = 0; S <= Seg; ++S)
		{
			const float A = CityTau * (float)S / (float)Seg;
			M.Vertex(FV3(Base.X + FMath::Sin(A) * R, Y, Base.Z + FMath::Cos(A) * R), N, FV2(0.5f + FMath::Sin(A) * 0.5f, 0.5f + FMath::Cos(A) * 0.5f));
		}
		for (int32 S = 0; S < Seg; ++S)
		{
			if (Cap) M.Tri(Ctr, First + S, First + S + 1);
			else M.Tri(Ctr, First + S + 1, First + S);
		}
	}
}

void MbSphere(FMeshBuf& M, const FV3& C, const FV3& R, int32 Seg, int32 Ring, float UVScale)
{
	const int32 Start = M.NumVerts();
	for (int32 J = 0; J <= Ring; ++J)
	{
		const float T = CityPi * (float)J / (float)Ring, ST = FMath::Sin(T), CT = FMath::Cos(T);
		for (int32 I = 0; I <= Seg; ++I)
		{
			const float A = CityTau * (float)I / (float)Seg;
			const FV3 N(ST * FMath::Sin(A), CT, ST * FMath::Cos(A));
			M.Vertex(FV3(C.X + N.X * R.X, C.Y + N.Y * R.Y, C.Z + N.Z * R.Z), Norm(FV3(N.X / R.X, N.Y / R.Y, N.Z / R.Z)),
				FV2((float)I / (float)Seg * UVScale, (float)J / (float)Ring * UVScale * 0.5f));
		}
	}
	for (int32 J = 0; J < Ring; ++J)
	{
		for (int32 I = 0; I < Seg; ++I)
		{
			const int32 A = Start + J * (Seg + 1) + I, D = A + Seg + 1;
			M.Tri(A, D, A + 1); M.Tri(A + 1, D, D + 1);
		}
	}
}

void MbTransform(FMeshBuf& M, int32 From, const FV3& Axis, float Angle, const FV3& Move)
{
	for (int32 I = From; I < M.Pos.Num(); ++I)
	{
		M.Pos[I] = Rotate(Axis, Angle, M.Pos[I]) + Move;
		M.Nrm[I] = Norm(Rotate(Axis, Angle, M.Nrm[I]));
	}
}

void MbTube(FMeshBuf& M, const FV3& A, const FV3& C, float R0, float R1, int32 Seg)
{
	const FV3 Ax = C - A;
	const float L = Len(Ax);
	if (L < 1e-6f) return;
	const FV3 Dir = Ax * (1.f / L), Up(0.f, 1.f, 0.f);
	FV3 K = Cross(Up, Dir);
	const float S = Len(K), D = Dot(Up, Dir);
	float Angle = FMath::Atan2(S, D);
	if (S < 1e-6f) { K = FV3(1.f, 0.f, 0.f); Angle = D > 0.f ? 0.f : CityPi; }
	else K = K * (1.f / S);
	const int32 Start = M.NumVerts();
	MbCylinder(M, FV3(0.f, 0.f, 0.f), R0, R1, L, Seg, false);
	MbTransform(M, Start, K, Angle, A);
}

// ---------------------------------------------------------------- построитель

void FBuilder::Init(float MinX, float MinZ, float MaxX, float MaxZ, float InChunk)
{
	Chunk = InChunk;
	Origin = FV3(MinX, 0.f, MinZ);
	CW = (int32)((MaxX - MinX) / Chunk) + 1;
	CH = (int32)((MaxZ - MinZ) / Chunk) + 1;
	Bufs.Reset(); Bufs.SetNum(CW * CH * CM_Num);
	Coll.Reset(); Coll.SetNum(CW * CH);
	SoftBufs.Reset(); SoftBufs.SetNum(CM_Num);
	SoftColl = FMeshBuf();
	LampPts.Reset(); HayPts.Reset(); WalkPts.Reset();
	Solids.Reset(); Flats.Reset();
	NumBuildings = 0;
	bSoft = false;
}

int32 FBuilder::ChunkIndex(const FV3& At) const
{
	const int32 X = FMath::Clamp((int32)((At.X - Origin.X) / Chunk), 0, CW - 1);
	const int32 Z = FMath::Clamp((int32)((At.Z - Origin.Z) / Chunk), 0, CH - 1);
	return Z * CW + X;
}

FMeshBuf& FBuilder::Mb(const FV3& At, int32 Mat)
{
	if (bSoft) return SoftBufs[Mat];
	return Bufs[ChunkIndex(At) * CM_Num + Mat];
}

FMeshBuf& FBuilder::CollBuf(const FV3& At)
{
	if (bSoft) return SoftColl;
	return Coll[ChunkIndex(At)];
}

void FBuilder::PhysBox(const FV3& Mn, const FV3& Mx, bool bGround)
{
	MbBox(CollBuf((Mn + Mx) * 0.5f), Mn, Mx, 1.f);
	if (!bGround) Solids.Add(FSolid{ Mn, Mx });
}

void FBuilder::PhysTri(const FV3& A, const FV3& B, const FV3& C)
{
	FMeshBuf& M = CollBuf(A);
	const FV3 N = Norm(Cross(B - A, C - A));
	const int32 I0 = M.Vertex(A, N, FV2()), I1 = M.Vertex(B, N, FV2()), I2 = M.Vertex(C, N, FV2());
	M.Tri(I0, I1, I2);
}

void FBuilder::PhysObb(const FV3& Center, float Yaw, const FV3& Half)
{
	const FV3 Up(0.f, 1.f, 0.f);
	FV3 P[8];
	FV3 Mn = Center, Mx = Center;
	for (int32 I = 0; I < 8; ++I)
	{
		const FV3 L((I & 1) ? Half.X : -Half.X, (I & 2) ? Half.Y : -Half.Y, (I & 4) ? Half.Z : -Half.Z);
		P[I] = Center + Rotate(Up, Yaw, L);
		Mn = VMin(Mn, P[I]); Mx = VMax(Mx, P[I]);
	}
	FMeshBuf& M = CollBuf(Center);
	const FV2 Z0(0.f, 0.f), Z1(1.f, 1.f);
	MbQuadBox(M, P[4], P[5], P[7], P[6], Z0, Z1);
	MbQuadBox(M, P[1], P[0], P[2], P[3], Z0, Z1);
	MbQuadBox(M, P[5], P[1], P[3], P[7], Z0, Z1);
	MbQuadBox(M, P[0], P[4], P[6], P[2], Z0, Z1);
	MbQuadBox(M, P[2], P[6], P[7], P[3], Z0, Z1);
	MbQuadBox(M, P[0], P[1], P[5], P[4], Z0, Z1);
	Solids.Add(FSolid{ Mn, Mx });
}

void FBuilder::Quad(FMeshBuf& M, const FV3& A, const FV3& B, const FV3& C, const FV3& D, FV2 UA, FV2 UB, FV2 UC, FV2 UD, FColor Col)
{
	const FV3 N = Norm(Cross(B - A, D - A));
	M.Color = Col;
	const int32 I0 = M.Vertex(A, N, UA), I1 = M.Vertex(B, N, UB), I2 = M.Vertex(C, N, UC), I3 = M.Vertex(D, N, UD);
	M.Tri(I0, I1, I2); M.Tri(I0, I2, I3);
}

void FBuilder::FrameRect(int32 Mat, const FFacadeFrame& F, float U0, float U1, float V0, float V1, float D, float UVS, FColor Col)
{
	if (U1 - U0 < 1e-3f || V1 - V0 < 1e-3f) return;
	const FV3 A = F.P(U0, V0, D), B = F.P(U1, V0, D), C = F.P(U1, V1, D), DD = F.P(U0, V1, D);
	Quad(Mb(A, Mat), A, B, C, DD, FV2(U0 * UVS, V0 * UVS), FV2(U1 * UVS, V0 * UVS), FV2(U1 * UVS, V1 * UVS), FV2(U0 * UVS, V1 * UVS), Col);
}

void FBuilder::FrameBox(int32 Mat, const FFacadeFrame& F, float U0, float U1, float V0, float V1, float D0, float D1, float UVS, FColor Col, bool bCollide, bool bBack)
{
	FV3 P[8];
	for (int32 I = 0; I < 8; ++I) P[I] = F.P((I & 1) ? U1 : U0, (I & 2) ? V1 : V0, (I & 4) ? D1 : D0);
	FMeshBuf& M = Mb(P[0], Mat);
	const float SU0 = U0 * UVS, SU1 = U1 * UVS, SV0 = V0 * UVS, SV1 = V1 * UVS, SD0 = D0 * UVS, SD1 = D1 * UVS;
	// рамка может быть «левой» (зеркальной) — тогда обход разворачиваем, чтобы грани смотрели наружу
	const bool bMirror = Dot(Cross(F.R, FV3(0.f, 1.f, 0.f)), F.N) < 0.f;
	auto AddQ = [this, &M, Col, bMirror](const FV3& A, const FV3& B, const FV3& C, const FV3& D, FV2 UA, FV2 UB, FV2 UC, FV2 UD)
	{
		if (bMirror) Quad(M, A, D, C, B, UA, UD, UC, UB, Col);
		else Quad(M, A, B, C, D, UA, UB, UC, UD, Col);
	};
	AddQ(P[4], P[5], P[7], P[6], FV2(SU0, SV0), FV2(SU1, SV0), FV2(SU1, SV1), FV2(SU0, SV1));   // перед (наружу)
	AddQ(P[2], P[6], P[7], P[3], FV2(SU0, SD0), FV2(SU0, SD1), FV2(SU1, SD1), FV2(SU1, SD0));   // верх
	AddQ(P[0], P[1], P[5], P[4], FV2(SU0, SD0), FV2(SU1, SD0), FV2(SU1, SD1), FV2(SU0, SD1));   // низ
	AddQ(P[0], P[4], P[6], P[2], FV2(SD0, SV0), FV2(SD1, SV0), FV2(SD1, SV1), FV2(SD0, SV1));   // левый торец
	AddQ(P[5], P[1], P[3], P[7], FV2(SD1, SV0), FV2(SD0, SV0), FV2(SD0, SV1), FV2(SD1, SV1));   // правый торец
	if (bBack) AddQ(P[1], P[0], P[2], P[3], FV2(SU1, SV0), FV2(SU0, SV0), FV2(SU0, SV1), FV2(SU1, SV1));   // задняя грань
	if (bCollide)
	{
		FV3 Mn = P[0], Mx = P[0];
		for (int32 I = 1; I < 8; ++I) { Mn = VMin(Mn, P[I]); Mx = VMax(Mx, P[I]); }
		PhysBox(Mn, Mx);
	}
}

void FBuilder::Soffit(int32 Mat, const FFacadeFrame& F, float U0, float U1, float V, float D0, float D1, float UVS, FColor Col, bool bFaceUp)
{
	if (U1 - U0 < 1e-3f || D1 - D0 < 1e-3f) return;
	const FV3 A = F.P(U0, V, D0), B = F.P(U1, V, D0), C = F.P(U1, V, D1), D = F.P(U0, V, D1);
	const FV2 UA(U0 * UVS, D0 * UVS), UB(U1 * UVS, D0 * UVS), UC(U1 * UVS, D1 * UVS), UD(U0 * UVS, D1 * UVS);
	// (A, B, C, D) при D1 > D0 смотрит вниз
	if (bFaceUp) Quad(Mb(A, Mat), A, D, C, B, UA, UD, UC, UB, Col);
	else Quad(Mb(A, Mat), A, B, C, D, UA, UB, UC, UD, Col);
}

void FBuilder::Box(int32 Mat, const FV3& Mn, const FV3& Mx, float UVS, FColor Col, bool bCollide)
{
	FMeshBuf& M = Mb((Mn + Mx) * 0.5f, Mat);
	M.Color = Col;
	MbBox(M, Mn, Mx, UVS);
	if (bCollide) PhysBox(Mn, Mx);
}

void FBuilder::Ground(int32 Mat, float X0, float Z0, float X1, float Z1, float Y, float UVS, bool bCollide)
{
	for (float Z = Z0; Z < Z1 - 1e-3f; Z += Chunk)
	{
		for (float X = X0; X < X1 - 1e-3f; X += Chunk)
		{
			const float XA = X, ZA = Z, XB = FMath::Min(X + Chunk, X1), ZB = FMath::Min(Z + Chunk, Z1);
			FMeshBuf& M = Mb(FV3((XA + XB) * 0.5f, Y, (ZA + ZB) * 0.5f), Mat);
			Quad(M, FV3(XA, Y, ZB), FV3(XB, Y, ZB), FV3(XB, Y, ZA), FV3(XA, Y, ZA),
				FV2(XA * UVS, ZB * UVS), FV2(XB * UVS, ZB * UVS), FV2(XB * UVS, ZA * UVS), FV2(XA * UVS, ZA * UVS), White);
			if (bCollide) PhysBox(FV3(XA, Y - 0.5f, ZA), FV3(XB, Y, ZB), true);
		}
	}
	if (bCollide && Y > 0.01f) Flats.Add(FFlat{ X0, Z0, X1, Z1, Y });
}

void FBuilder::RoofQuad(int32 Mat, const FV3& A, const FV3& B, const FV3& C, const FV3& D, float UVS)
{
	// UV: вдоль ската (по горизонтали A→B) и вверх по скату
	const float W = Dist(A, B), H = Dist(A, D);
	Quad(Mb(A, Mat), A, B, C, D, FV2(0.f, 0.f), FV2(W * UVS, 0.f), FV2(W * UVS, H * UVS), FV2(0.f, H * UVS), White);
	PhysTri(A, B, C); PhysTri(A, C, D);
}

void FBuilder::RoofTri(int32 Mat, const FV3& A, const FV3& B, const FV3& C, float UVS)
{
	FMeshBuf& M = Mb(A, Mat);
	const FV3 N = Norm(Cross(B - A, C - A));
	M.Color = White;
	const float W = Dist(A, B), H = Dist(LerpV(A, B, 0.5f), C);
	const int32 I0 = M.Vertex(A, N, FV2(0.f, 0.f)), I1 = M.Vertex(B, N, FV2(W * UVS, 0.f)), I2 = M.Vertex(C, N, FV2(W * 0.5f * UVS, H * UVS));
	M.Tri(I0, I1, I2);
	PhysTri(A, B, C);
}

// ---------------------------------------------------------------- окно в нише

void FBuilder::Window(const FBuilding& B, const FFacadeFrame& F, int32 WM, float UC, float V0, float W, float H, bool bDetail, bool bFrench, bool bDoor, FRng& Rng)
{
	const float U0 = UC - W * 0.5f, U1 = UC + W * 0.5f, V1 = V0 + H, Dep = 0.24f;
	const FColor Tint = B.Tint, Trim = B.TrimCol;
	// откосы ниши: левый смотрит вправо, правый — влево, верх — вниз, низ — вверх
	FrameRect(WM, FFacadeFrame(F.P(U0, 0.f, 0.f), -F.N, F.R), 0.f, Dep, V0, V1, 0.f, 0.5f, Tint);
	FrameRect(WM, FFacadeFrame(F.P(U1, 0.f, -Dep), F.N, -F.R), 0.f, Dep, V0, V1, 0.f, 0.5f, Tint);
	Soffit(WM, F, U0, U1, V1, -Dep, 0.f, 0.5f, Tint, false);
	Soffit(WM, F, U0, U1, V0, -Dep, 0.f, 0.5f, Tint, true);
	// стекло и переплёт (или дверь)
	const bool bLit = H01(Rng.U32()) < 0.35f;
	if (bDoor)
	{
		const FColor DoorCol = B.ShutterCol.A ? B.ShutterCol : C3(90, 60, 40);
		FrameRect(CM_Wood, F, U0, U1, V0, V1, -Dep, 0.6f, DoorCol);
		FrameBox(CM_Trim, F, UC - 0.03f, UC + 0.03f, V0, V1, -Dep, -Dep + 0.04f, 1.f, Trim, false);
		FrameBox(CM_Iron, F, UC + 0.12f, UC + 0.16f, V0 + 1.05f, V0 + 1.1f, -Dep, -Dep + 0.07f, 1.f, C3(30, 30, 34), false);   // ручка
	}
	else
	{
		FrameRect(CM_Glass, F, U0, U1, V0, V1, -Dep, 1.f, bLit ? C3(255, 200, 120) : C3(40, 46, 52));
	}
	if (!bDetail) return;
	const float FW = 0.05f;
	FrameBox(CM_Trim, F, U0, U1, V0, V0 + FW, -Dep, -Dep + 0.06f, 1.f, Trim, false);
	FrameBox(CM_Trim, F, U0, U1, V1 - FW, V1, -Dep, -Dep + 0.06f, 1.f, Trim, false);
	FrameBox(CM_Trim, F, U0, U0 + FW, V0, V1, -Dep, -Dep + 0.06f, 1.f, Trim, false);
	FrameBox(CM_Trim, F, U1 - FW, U1, V0, V1, -Dep, -Dep + 0.06f, 1.f, Trim, false);
	if (!bDoor)
	{
		FrameBox(CM_Trim, F, UC - FW * 0.5f, UC + FW * 0.5f, V0, V1, -Dep, -Dep + 0.05f, 1.f, Trim, false);
		const int32 Bars = bFrench ? 4 : 3;
		for (int32 K = 1; K < Bars; ++K)
		{
			const float VY = V0 + H * (float)K / (float)Bars;
			FrameBox(CM_Trim, F, U0, U1, VY - 0.02f, VY + 0.02f, -Dep, -Dep + 0.045f, 1.f, Trim, false);
		}
	}
	// подоконник (маленький выступ) и наличник сверху
	if (!bFrench && !bDoor) FrameBox(CM_Stone, F, U0 - 0.1f, U1 + 0.1f, V0 - 0.1f, V0, 0.f, 0.14f, 0.5f, Tint, false);
	FrameBox(CM_Stone, F, U0 - 0.12f, U1 + 0.12f, V1, V1 + 0.16f, 0.f, 0.07f, 0.5f, Tint, false);
	if (H01(B.Seed * 7u + 3u) > 0.5f) FrameBox(CM_Stone, F, UC - 0.14f, UC + 0.14f, V1 - 0.02f, V1 + 0.2f, 0.f, 0.1f, 0.5f, Tint, false);   // замковый камень
	// ставни, раскрытые к стене
	if (B.ShutterCol.A && !bDoor)
	{
		const float SW = W * 0.5f, Ang = 0.25f + 0.5f * H01(Rng.U32());
		for (int32 S = 0; S < 2; ++S)
		{
			const float Hinge = S ? U1 + 0.02f : U0 - 0.02f, Dir = S ? 1.f : -1.f;
			const FV3 HB = F.P(Hinge, V0, 0.02f);
			const FV3 Out = Norm(F.R * (Dir * FMath::Cos(Ang)) + F.N * FMath::Sin(Ang));
			FFacadeFrame SF(HB, Out, Norm(Cross(Out, FV3(0.f, 1.f, 0.f))));
			if (Dot(SF.N, F.N) < 0.f) SF.N = -SF.N;
			FrameBox(CM_Shutter, SF, 0.f, SW, 0.f, H, -0.02f, 0.02f, 1.f / H, B.ShutterCol, false, true);
		}
	}
}

// ---------------------------------------------------------------- фасад одной стороны

void FBuilder::Facade(const FBuilding& B, int32 Side, FRng& Rng)
{
	const float X0 = B.X0, Z0 = B.Z0, X1 = B.X1, Z1 = B.Z1, Y0 = B.BaseY;
	FFacadeFrame F;
	float FacadeLen = 0.f;
	switch (Side)
	{
	case 0: F = FFacadeFrame(FV3(X1, Y0, Z0), FV3(-1.f, 0.f, 0.f), FV3(0.f, 0.f, -1.f)); FacadeLen = X1 - X0; break;
	case 1: F = FFacadeFrame(FV3(X1, Y0, Z1), FV3(0.f, 0.f, -1.f), FV3(1.f, 0.f, 0.f)); FacadeLen = Z1 - Z0; break;
	case 2: F = FFacadeFrame(FV3(X0, Y0, Z1), FV3(1.f, 0.f, 0.f), FV3(0.f, 0.f, 1.f)); FacadeLen = X1 - X0; break;
	default: F = FFacadeFrame(FV3(X0, Y0, Z0), FV3(0.f, 0.f, 1.f), FV3(-1.f, 0.f, 0.f)); FacadeLen = Z1 - Z0; break;
	}
	const bool bStreet = ((B.StreetSides >> Side) & 1) != 0, bShop = ((B.Shops >> Side) & 1) != 0;
	const int32 WM = B.WallMat;
	const float UVS = WM == CM_Brick ? 0.4f : 0.45f;
	const float Bay = 2.7f;
	int32 NB = (int32)((FacadeLen - 0.8f) / Bay);
	if (NB < 1) NB = 1;
	const float Margin = (FacadeLen - (float)NB * Bay) * 0.5f;
	const float H = B.GroundH + (float)(B.Floors - 1) * B.FloorH;
	const FColor Tint = B.Tint;
	const FColor IronC = C3(30, 30, 34);
	for (int32 Fl = 0; Fl < B.Floors; ++Fl)
	{
		const float FV0 = Fl == 0 ? 0.f : B.GroundH + (float)(Fl - 1) * B.FloorH;
		const float FH = Fl == 0 ? B.GroundH : B.FloorH, FV1 = FV0 + FH;
		// цоколь первого этажа — камень
		const int32 FMat = (Fl == 0 && WM != CM_Stone) ? (int32)CM_Stone : WM;
		const float FUVS = FMat == CM_Stone ? 0.45f : UVS;
		const bool bFrench = Fl == B.BalconyFloor && bStreet;
		float Cur = 0.f;   // стена между проёмами
		for (int32 I = 0; I < NB; ++I)
		{
			const float UC = Margin + ((float)I + 0.5f) * Bay;
			float WW = 1.f, WH = 1.6f, WV0 = 1.1f;
			bool bDoor = false;
			if (Fl == 0)
			{
				if (bShop && bStreet) { WW = 2.1f; WH = FH - 0.9f; WV0 = 0.15f; }
				else if (bStreet && I == NB / 2) { WW = 1.3f; WH = 2.5f; WV0 = 0.f; bDoor = true; }
				else { WW = 1.0f; WH = 1.6f; WV0 = 1.1f; }
			}
			else if (!bStreet && H01(Rng.U32()) < 0.35f) { continue; }   // глухие участки на задворках
			else { WW = 1.08f; WH = bFrench ? FH - 0.55f : FH * 0.55f; WV0 = bFrench ? 0.05f : FH * 0.26f; }
			const float U0 = UC - WW * 0.5f, U1 = UC + WW * 0.5f;
			FrameRect(FMat, F, Cur, U0, FV0, FV1, 0.f, FUVS, Tint);                     // простенок слева
			FrameRect(FMat, F, U0, U1, FV0, FV0 + WV0, 0.f, FUVS, Tint);               // под окном
			FrameRect(FMat, F, U0, U1, FV0 + WV0 + WH, FV1, 0.f, FUVS, Tint);          // над окном
			Cur = U1;
			if (Fl == 0 && bShop && bStreet)
			{
				// лавка: тёмный проём с деревянной рамой, вывеска и навес
				const float D = 0.3f, Top = FV0 + WV0 + WH;
				const FColor FrameCol = B.ShutterCol.A ? B.ShutterCol : C3(90, 60, 40);
				FrameRect(CM_Glass, F, U0, U1, FV0 + WV0, Top, -D, 1.f, C3(60, 50, 40));
				FrameBox(CM_Wood, F, U0 - 0.08f, U0, FV0 + WV0, Top, -D, 0.04f, 0.5f, FrameCol, false);
				FrameBox(CM_Wood, F, U1, U1 + 0.08f, FV0 + WV0, Top, -D, 0.04f, 0.5f, FrameCol, false);
				Soffit(CM_Wood, F, U0, U1, Top, -D, 0.f, 0.5f, FrameCol, false);                                      // потолок витрины
				FrameBox(CM_Wood, F, U0 - 0.1f, U1 + 0.1f, Top, Top + 0.5f, 0.f, 0.08f, 0.5f, B.TrimCol, false);     // вывеска
				if (H01(Rng.U32()) < 0.7f)
				{
					// навес из ткани (полосатый), видно с обеих сторон
					const FColor AC = H01(Rng.U32()) < 0.5f ? C3(170, 40, 40) : C3(40, 80, 140);
					const FV3 A = F.P(U0 - 0.15f, Top + 0.45f, 0.02f), Bv = F.P(U1 + 0.15f, Top + 0.45f, 0.02f);
					const FV3 C = F.P(U1 + 0.15f, Top - 0.1f, 1.3f), Dv = F.P(U0 - 0.15f, Top - 0.1f, 1.3f);
					FMeshBuf& M = Mb(A, CM_Cloth);
					Quad(M, A, Bv, C, Dv, FV2(0.f, 0.f), FV2(WW * 2.f, 0.f), FV2(WW * 2.f, 1.f), FV2(0.f, 1.f), AC);
					Quad(M, Bv, A, Dv, C, FV2(0.f, 0.f), FV2(WW * 2.f, 0.f), FV2(WW * 2.f, 1.f), FV2(0.f, 1.f), AC);
				}
				// прилавок
				FrameBox(CM_Wood, F, U0, U1, FV0 + 0.15f, FV0 + 0.95f, -D, 0.05f, 0.5f, C3(110, 80, 55), false);
			}
			else
			{
				Window(B, F, FMat, UC, FV0 + WV0, WW, WH, bStreet || Fl > 0, bFrench, bDoor, Rng);
			}
		}
		FrameRect(FMat, F, Cur, FacadeLen, FV0, FV1, 0.f, FUVS, Tint);
		// межэтажный карниз (уступ для паркура)
		if (Fl < B.Floors - 1)
		{
			const float Big = Fl == 0 ? 0.3f : 0.2f, HH = Fl == 0 ? 0.3f : 0.2f;
			FrameBox(CM_Stone, F, -0.05f, FacadeLen + 0.05f, FV1 - HH * 0.5f, FV1 + HH * 0.5f, 0.f, Big, 0.5f, Tint, true);
		}
		// балкон
		if (bFrench && Fl > 0)
		{
			const float BU0 = Margin + Bay * 0.1f, BU1 = FacadeLen - Margin - Bay * 0.1f, BY = FV0;
			FrameBox(CM_Stone, F, BU0, BU1, BY - 0.18f, BY + 0.02f, 0.f, 0.85f, 0.5f, Tint, true);
			for (int32 K = 0; K < 3; ++K)   // консоли
			{
				const float CU = BU0 + (BU1 - BU0) * ((float)K + 0.5f) / 3.f;
				FrameBox(CM_Stone, F, CU - 0.12f, CU + 0.12f, BY - 0.5f, BY - 0.18f, 0.f, 0.75f, 0.5f, Tint, false);
			}
			// кованая решётка
			FrameBox(CM_Iron, F, BU0, BU1, BY + 0.95f, BY + 1.0f, 0.78f, 0.83f, 1.f, IronC, true, true);
			FrameBox(CM_Iron, F, BU0, BU1, BY + 0.1f, BY + 0.13f, 0.78f, 0.83f, 1.f, IronC, false, true);
			for (float U = BU0; U <= BU1; U += 0.13f) FrameBox(CM_Iron, F, U, U + 0.02f, BY, BY + 0.97f, 0.79f, 0.82f, 1.f, IronC, false, true);
			FrameBox(CM_Iron, F, BU0, BU0 + 0.03f, BY, BY + 1.f, 0.f, 0.83f, 1.f, IronC, false);
			FrameBox(CM_Iron, F, BU1 - 0.03f, BU1, BY, BY + 1.f, 0.f, 0.83f, 1.f, IronC, false);
		}
	}
	// руст по углам
	if (bStreet)
	{
		for (float V = 0.2f; V < H - 0.3f; V += 0.5f)
		{
			const bool bOdd = (((int32)(V / 0.5f)) & 1) != 0;
			const float WQ = bOdd ? 0.5f : 0.35f;
			FrameBox(CM_Stone, F, 0.f, WQ, V, V + 0.44f, 0.f, 0.04f, 0.5f, Tint, false);
			FrameBox(CM_Stone, F, FacadeLen - WQ, FacadeLen, V, V + 0.44f, 0.f, 0.04f, 0.5f, Tint, false);
		}
	}
	// венчающий карниз
	FrameBox(CM_Stone, F, -0.25f, FacadeLen + 0.25f, H - 0.25f, H, 0.f, 0.25f, 0.5f, Tint, true);
	FrameBox(CM_Stone, F, -0.4f, FacadeLen + 0.4f, H, H + 0.2f, 0.f, 0.45f, 0.5f, Tint, true);
	if (bStreet)   // сухарики
	{
		for (float U = 0.2f; U < FacadeLen - 0.1f; U += 0.35f) FrameBox(CM_Stone, F, U, U + 0.12f, H - 0.42f, H - 0.25f, 0.f, 0.16f, 0.5f, Tint, false);
	}
}

// ---------------------------------------------------------------- крыши

void FBuilder::Chimney(const FV3& Base, float H, FColor Tint)
{
	const FV3 Mn(Base.X - 0.45f, Base.Y, Base.Z - 0.3f), Mx(Base.X + 0.45f, Base.Y + H, Base.Z + 0.3f);
	Box(CM_Brick, Mn, Mx, 0.5f, Tint, true);
	Box(CM_Stone, FV3(Mn.X - 0.06f, Mx.Y, Mn.Z - 0.06f), FV3(Mx.X + 0.06f, Mx.Y + 0.12f, Mx.Z + 0.06f), 0.5f, Tint, true);
	for (int32 I = 0; I < 2; ++I)   // гончарные трубы
	{
		FMeshBuf& M = Mb(Base, CM_Tiles);
		M.Color = C3(170, 100, 70);
		MbCylinder(M, FV3(Base.X - 0.2f + (float)I * 0.4f, Mx.Y + 0.12f, Base.Z), 0.09f, 0.07f, 0.3f, 8, true);
	}
}

void FBuilder::RoofOf(const FBuilding& B, FRng& Rng)
{
	const float H = B.BaseY + B.GroundH + (float)(B.Floors - 1) * B.FloorH + 0.2f;
	const float X0 = B.X0 - 0.1f, Z0 = B.Z0 - 0.1f, X1 = B.X1 + 0.1f, Z1 = B.Z1 + 0.1f;
	if (B.Roof == Roof_Flat)
	{
		Ground(CM_Flag, X0, Z0, X1, Z1, H, 0.5f, false);
		PhysBox(FV3(X0, H - 0.5f, Z0), FV3(X1, H, Z1));
		const float T = 0.25f, PH = 0.7f;   // парапет
		Box(CM_Stone, FV3(X0, H, Z0), FV3(X1, H + PH, Z0 + T), 0.5f, B.Tint, true);
		Box(CM_Stone, FV3(X0, H, Z1 - T), FV3(X1, H + PH, Z1), 0.5f, B.Tint, true);
		Box(CM_Stone, FV3(X0, H, Z0 + T), FV3(X0 + T, H + PH, Z1 - T), 0.5f, B.Tint, true);
		Box(CM_Stone, FV3(X1 - T, H, Z0 + T), FV3(X1, H + PH, Z1 - T), 0.5f, B.Tint, true);
		Chimney(FV3(LerpF(X0, X1, 0.3f), H, LerpF(Z0, Z1, 0.6f)), 1.4f, B.Tint);
		return;
	}
	if (B.Roof == Roof_Mansard)
	{
		const float Ins = 0.85f, H1 = 2.5f;
		const float Ins2 = FMath::Min(FMath::Min(X1 - X0, Z1 - Z0) * 0.5f - Ins - 0.3f, 2.2f), H2 = Ins2 * 0.6f;
		const FV3 A0(X0, H, Z0), A1(X1, H, Z0), A2(X1, H, Z1), A3(X0, H, Z1);
		const FV3 B0(X0 + Ins, H + H1, Z0 + Ins), B1(X1 - Ins, H + H1, Z0 + Ins), B2(X1 - Ins, H + H1, Z1 - Ins), B3(X0 + Ins, H + H1, Z1 - Ins);
		// нижние крутые скаты
		RoofQuad(CM_Slate, A1, A0, B0, B1, 0.5f); RoofQuad(CM_Slate, A2, A1, B1, B2, 0.5f);
		RoofQuad(CM_Slate, A3, A2, B2, B3, 0.5f); RoofQuad(CM_Slate, A0, A3, B3, B0, 0.5f);
		// верхняя пологая часть: вальма с коньком вдоль длинной стороны
		const bool bAlongX = (X1 - X0) >= (Z1 - Z0);
		const float CXM = (X0 + X1) * 0.5f, CZM = (Z0 + Z1) * 0.5f;
		float Half = bAlongX ? (X1 - X0) * 0.5f - Ins - Ins2 : (Z1 - Z0) * 0.5f - Ins - Ins2;
		if (Half < 0.f) Half = 0.f;
		const FV3 R0 = bAlongX ? FV3(CXM - Half, H + H1 + H2, CZM) : FV3(CXM, H + H1 + H2, CZM - Half);
		const FV3 R1 = bAlongX ? FV3(CXM + Half, H + H1 + H2, CZM) : FV3(CXM, H + H1 + H2, CZM + Half);
		if (bAlongX)
		{
			RoofQuad(CM_Slate, B1, B0, R0, R1, 0.5f); RoofQuad(CM_Slate, B3, B2, R1, R0, 0.5f);
			RoofTri(CM_Slate, B2, B1, R1, 0.5f); RoofTri(CM_Slate, B0, B3, R0, 0.5f);
		}
		else
		{
			RoofQuad(CM_Slate, B2, B1, R0, R1, 0.5f); RoofTri(CM_Slate, B1, B0, R0, 0.5f);
			RoofQuad(CM_Slate, B0, B3, R1, R0, 0.5f); RoofTri(CM_Slate, B3, B2, R1, 0.5f);
		}
		// слуховые окна на нижнем скате со стороны улицы
		for (int32 Side = 0; Side < 4; ++Side)
		{
			if (!((B.StreetSides >> Side) & 1)) continue;
			const float SideLen = (Side & 1) ? Z1 - Z0 : X1 - X0;
			const int32 ND = (int32)(SideLen / 3.f);
			for (int32 I = 0; I < ND; ++I)
			{
				const float U = ((float)I + 0.5f) * SideLen / (float)ND;
				FV3 P, N;
				switch (Side)
				{
				case 0: P = FV3(X1 - U, H, Z0); N = FV3(0.f, 0.f, -1.f); break;
				case 1: P = FV3(X1, H, Z1 - U); N = FV3(1.f, 0.f, 0.f); break;
				case 2: P = FV3(X0 + U, H, Z1); N = FV3(0.f, 0.f, 1.f); break;
				default: P = FV3(X0, H, Z0 + U); N = FV3(-1.f, 0.f, 0.f); break;
				}
				const FV3 R = Cross(FV3(0.f, 1.f, 0.f), N);
				const FFacadeFrame DF(P + N * -0.25f, R, N);
				FrameBox(CM_Plaster, DF, -0.55f, 0.55f, 0.2f, 1.7f, -Ins - 0.2f, 0.f, 0.5f, B.Tint, true);
				FrameRect(CM_Glass, DF, -0.35f, 0.35f, 0.4f, 1.45f, 0.01f, 1.f, H01(Rng.U32()) < 0.3f ? C3(255, 200, 120) : C3(40, 46, 52));
				FrameBox(CM_Trim, DF, -0.4f, 0.4f, 1.45f, 1.52f, -0.02f, 0.04f, 1.f, B.TrimCol, false);
				// двускатная крышка (обход развёрнут относительно оригинала — скаты смотрят наружу)
				const FV3 T0 = DF.P(-0.65f, 1.7f, 0.1f), T1 = DF.P(0.65f, 1.7f, 0.1f), T2 = DF.P(0.f, 2.15f, 0.1f);
				const FV3 K0 = DF.P(-0.65f, 1.7f, -Ins - 0.3f), K1 = DF.P(0.65f, 1.7f, -Ins - 0.3f), K2 = DF.P(0.f, 2.15f, -Ins - 0.3f);
				RoofQuad(CM_Slate, T0, T2, K2, K0, 0.5f);
				RoofQuad(CM_Slate, K1, K2, T2, T1, 0.5f);
				// изнанка крышки (свес виден снизу)
				Quad(Mb(T0, CM_Slate), T0, K0, K2, T2, FV2(0.f, 0.f), FV2(0.6f, 0.f), FV2(0.6f, 0.4f), FV2(0.f, 0.4f), C3(120, 120, 125));
				Quad(Mb(T1, CM_Slate), K1, T1, T2, K2, FV2(0.f, 0.f), FV2(0.6f, 0.f), FV2(0.6f, 0.4f), FV2(0.f, 0.4f), C3(120, 120, 125));
				FMeshBuf& M = Mb(T0, CM_Plaster);
				M.Color = B.Tint;
				const int32 Q0 = M.Vertex(T0, N, FV2(0.f, 0.f)), Q1 = M.Vertex(T1, N, FV2(0.6f, 0.f)), Q2 = M.Vertex(T2, N, FV2(0.3f, 0.25f));
				M.Tri(Q0, Q1, Q2);
			}
		}
		const int32 NCh = 1 + (int32)(H01(Rng.U32()) * 3.f);
		for (int32 I = 0; I < NCh; ++I)
		{
			const float U = H01(Rng.U32());
			FV3 P = bAlongX ? FV3(LerpF(R0.X, R1.X, U), 0.f, CZM + ((I & 1) ? 0.6f : -0.6f)) : FV3(CXM + ((I & 1) ? 0.6f : -0.6f), 0.f, LerpF(R0.Z, R1.Z, U));
			P.Y = H + H1 + H2 * 0.5f;
			Chimney(P, 1.6f, C3(150, 90, 70));
		}
		return;
	}
	// двускатная черепичная
	const bool bAlongX = (X1 - X0) >= (Z1 - Z0);
	const float Rise = (bAlongX ? (Z1 - Z0) : (X1 - X0)) * 0.5f * 0.75f;
	const float CXM = (X0 + X1) * 0.5f, CZM = (Z0 + Z1) * 0.5f, OV = 0.35f;
	const FV3 E0(X0 - OV, H - 0.25f, Z0 - OV), E1(X1 + OV, H - 0.25f, Z0 - OV), E2(X1 + OV, H - 0.25f, Z1 + OV), E3(X0 - OV, H - 0.25f, Z1 + OV);
	const FColor Under = C3(150, 120, 95);
	// изнанка свеса (видна снизу с улицы)
	auto Underside = [this, Under](const FV3& A, const FV3& Bq, const FV3& C, const FV3& D)
	{
		const float W = Dist(A, Bq), Hh = Dist(A, D);
		Quad(Mb(A, CM_Wood), A, D, C, Bq, FV2(0.f, 0.f), FV2(0.f, Hh * 0.5f), FV2(W * 0.5f, Hh * 0.5f), FV2(W * 0.5f, 0.f), Under);
	};
	FMeshBuf& GM = Mb(FV3(CXM, H, CZM), B.WallMat);
	if (bAlongX)
	{
		const FV3 R0(X0 - OV, H + Rise, CZM), R1(X1 + OV, H + Rise, CZM);
		RoofQuad(CM_Tiles, E1, E0, R0, R1, 0.6f); RoofQuad(CM_Tiles, E3, E2, R1, R0, 0.6f);
		Underside(E1, E0, R0, R1); Underside(E3, E2, R1, R0);
		GM.Color = B.Tint;
		for (int32 S = 0; S < 2; ++S)   // фронтоны
		{
			const float X = S ? X1 : X0;
			const FV3 N(S ? 1.f : -1.f, 0.f, 0.f);
			const FV3 G0(X, H, Z0), G1(X, H, Z1), G2(X, H + Rise, CZM);
			const int32 Q0 = GM.Vertex(G0, N, FV2(Z0 * 0.45f, H * 0.45f)), Q1 = GM.Vertex(G1, N, FV2(Z1 * 0.45f, H * 0.45f));
			const int32 Q2 = GM.Vertex(G2, N, FV2(CZM * 0.45f, (H + Rise) * 0.45f));
			if (S) { GM.Tri(Q0, Q2, Q1); PhysTri(G0, G2, G1); }
			else { GM.Tri(Q0, Q1, Q2); PhysTri(G0, G1, G2); }
		}
	}
	else
	{
		const FV3 R0(CXM, H + Rise, Z0 - OV), R1(CXM, H + Rise, Z1 + OV);
		RoofQuad(CM_Tiles, E0, E3, R1, R0, 0.6f); RoofQuad(CM_Tiles, E2, E1, R0, R1, 0.6f);
		Underside(E0, E3, R1, R0); Underside(E2, E1, R0, R1);
		GM.Color = B.Tint;
		for (int32 S = 0; S < 2; ++S)
		{
			const float Z = S ? Z1 : Z0;
			const FV3 N(0.f, 0.f, S ? 1.f : -1.f);
			const FV3 G0(X0, H, Z), G1(X1, H, Z), G2(CXM, H + Rise, Z);
			const int32 Q0 = GM.Vertex(G0, N, FV2(X0 * 0.45f, H * 0.45f)), Q1 = GM.Vertex(G1, N, FV2(X1 * 0.45f, H * 0.45f));
			const int32 Q2 = GM.Vertex(G2, N, FV2(CXM * 0.45f, (H + Rise) * 0.45f));
			if (S) { GM.Tri(Q0, Q1, Q2); PhysTri(G0, G1, G2); }
			else { GM.Tri(Q0, Q2, Q1); PhysTri(G0, G2, G1); }
		}
	}
	Chimney(FV3(LerpF(X0, X1, 0.7f), H + Rise * 0.4f, LerpF(Z0, Z1, 0.4f)), 1.8f, C3(150, 90, 70));
}

void FBuilder::Building(const FBuilding& B)
{
	FRng Rng((uint64)(B.Seed * 977u + 13u));
	const float H = B.GroundH + (float)(B.Floors - 1) * B.FloorH;
	// объём для столкновений: лазать можно по всей стене
	PhysBox(FV3(B.X0, B.BaseY, B.Z0), FV3(B.X1, B.BaseY + H + 0.2f, B.Z1));
	for (int32 S = 0; S < 4; ++S) Facade(B, S, Rng);
	RoofOf(B, Rng);
	++NumBuildings;
}

// ---------------------------------------------------------------- уличные предметы

void FBuilder::Lamp(const FV3& P)
{
	FMeshBuf& M = Mb(P, CM_Iron);
	M.Color = C3(28, 28, 32);
	MbCylinder(M, P, 0.09f, 0.06f, 0.5f, 10, true);
	MbCylinder(M, FV3(P.X, P.Y + 0.5f, P.Z), 0.05f, 0.04f, 3.2f, 8, false);
	MbCylinder(M, FV3(P.X, P.Y + 3.7f, P.Z), 0.2f, 0.12f, 0.12f, 8, true);
	MbCylinder(M, FV3(P.X, P.Y + 4.25f, P.Z), 0.02f, 0.22f, 0.25f, 8, true);
	FMeshBuf& G = Mb(P, CM_Lamp);
	G.Color = White;
	MbCylinder(G, FV3(P.X, P.Y + 3.82f, P.Z), 0.16f, 0.2f, 0.43f, 8, true);
	PhysBox(FV3(P.X - 0.08f, P.Y, P.Z - 0.08f), FV3(P.X + 0.08f, P.Y + 4.3f, P.Z + 0.08f));
	LampPts.Add(FV3(P.X, P.Y + 4.f, P.Z));
}

void FBuilder::Tree(const FV3& P, float H, uint32 Seed)
{
	FRng R((uint64)(Seed * 31u + 5u));
	FMeshBuf& M = Mb(P, CM_Bark);
	M.Color = C3(110, 90, 75);
	MbCylinder(M, P, 0.22f, 0.13f, H * 0.55f, 10, false);
	for (int32 I = 0; I < 3; ++I)
	{
		const float A = (float)I * 2.1f + R.F();
		const FV3 B0(P.X, P.Y + H * (0.35f + 0.1f * (float)I), P.Z);
		MbTube(M, B0, FV3(B0.X + FMath::Sin(A) * 1.2f, B0.Y + 1.4f, B0.Z + FMath::Cos(A) * 1.2f), 0.09f, 0.04f, 6);
	}
	FMeshBuf& LV = Mb(P, CM_Leaves);
	for (int32 I = 0; I < 9; ++I)
	{
		const float A = R.F() * CityTau;
		const float RR = R.Range(0.2f, 1.6f);
		const float Y = P.Y + H * R.Range(0.55f, 0.95f);
		const float S = R.Range(1.0f, 1.7f);
		const int32 Start = LV.NumVerts();
		const int32 CR = 70 + (int32)(R.F() * 40.f);
		const int32 CG = 105 + (int32)(R.F() * 40.f);
		LV.Color = C3(CR, CG, 45);
		MbSphere(LV, FV3(P.X + FMath::Sin(A) * RR, Y, P.Z + FMath::Cos(A) * RR), FV3(S, S * 0.8f, S), 9, 6, 3.f);
		for (int32 K = Start; K < LV.NumVerts(); ++K) LV.Col[K].A = 0xB0;   // альфа вершины — гибкость для ветра
	}
	PhysBox(FV3(P.X - 0.2f, P.Y, P.Z - 0.2f), FV3(P.X + 0.2f, P.Y + H * 0.55f, P.Z + 0.2f));
}

void FBuilder::HayCart(const FV3& P, float Yaw)
{
	bSoft = true;   // вся телега — мягкая (компонент с тегом TPSoft)
	const FV3 Up(0.f, 1.f, 0.f);
	FMeshBuf& M = Mb(P, CM_Wood);
	const int32 S0 = M.NumVerts();
	M.Color = C3(130, 95, 65);
	MbBox(M, FV3(-1.1f, 0.55f, -0.75f), FV3(1.1f, 0.7f, 0.75f), 0.5f);
	MbBox(M, FV3(-1.1f, 0.7f, -0.8f), FV3(1.1f, 1.0f, -0.72f), 0.5f);
	MbBox(M, FV3(-1.1f, 0.7f, 0.72f), FV3(1.1f, 1.0f, 0.8f), 0.5f);
	for (int32 I = 0; I < 2; ++I)   // колёса
	{
		const int32 W0 = M.NumVerts();
		MbCylinder(M, FV3(0.f, 0.f, 0.f), 0.5f, 0.5f, 0.1f, 16, true);
		MbTransform(M, W0, FV3(1.f, 0.f, 0.f), CityPi * 0.5f, FV3(0.f, 0.5f, I ? 0.85f : -0.95f));
	}
	MbTube(M, FV3(1.1f, 0.65f, -0.4f), FV3(2.6f, 0.45f, -0.3f), 0.05f, 0.05f, 6);   // оглобли
	MbTube(M, FV3(1.1f, 0.65f, 0.4f), FV3(2.6f, 0.45f, 0.3f), 0.05f, 0.05f, 6);
	MbTransform(M, S0, Up, Yaw, P);
	FMeshBuf& HY = Mb(P, CM_Dirt);
	const int32 S1 = HY.NumVerts();
	HY.Color = C3(220, 185, 95);
	MbSphere(HY, FV3(0.f, 0.95f, 0.f), FV3(1.2f, 0.55f, 0.8f), 14, 8, 2.f);
	MbTransform(HY, S1, Up, Yaw, P);
	PhysObb(P + Rotate(Up, Yaw, FV3(0.f, 0.975f, 0.f)), Yaw, FV3(1.1f, 0.475f, 0.8f));
	HayPts.Add(FV3(P.X, P.Y + 1.45f, P.Z));
	bSoft = false;
}

void FBuilder::Stall(const FV3& P, float Yaw, FColor Awning)
{
	const FV3 Up(0.f, 1.f, 0.f);
	FMeshBuf& M = Mb(P, CM_Wood);
	const int32 S0 = M.NumVerts();
	M.Color = C3(140, 105, 70);
	MbBox(M, FV3(-1.2f, 0.f, -0.6f), FV3(1.2f, 0.9f, 0.6f), 0.5f);
	for (int32 I = 0; I < 4; ++I)
	{
		MbBox(M, FV3((I & 1) ? 1.1f : -1.2f, 0.f, (I & 2) ? 0.5f : -0.7f), FV3((I & 1) ? 1.2f : -1.1f, 2.3f, (I & 2) ? 0.6f : -0.6f), 0.5f);
	}
	MbTransform(M, S0, Up, Yaw, P);
	FMeshBuf& CL = Mb(P, CM_Cloth);
	FV3 Q[4] = { FV3(-1.35f, 2.35f, -0.85f), FV3(1.35f, 2.35f, -0.85f), FV3(1.35f, 2.1f, 0.85f), FV3(-1.35f, 2.1f, 0.85f) };
	for (int32 K = 0; K < 4; ++K) Q[K] = Rotate(Up, Yaw, Q[K]) + P;
	Quad(CL, Q[0], Q[1], Q[2], Q[3], FV2(0.f, 0.f), FV2(3.f, 0.f), FV2(3.f, 1.f), FV2(0.f, 1.f), Awning);
	Quad(CL, Q[1], Q[0], Q[3], Q[2], FV2(0.f, 0.f), FV2(3.f, 0.f), FV2(3.f, 1.f), FV2(0.f, 1.f), Awning);
	FMeshBuf& FR = Mb(P, CM_Trim);
	const int32 S2 = FR.NumVerts();
	FRng R((uint64)(uint32)(P.X * 13.f + P.Z * 7.f));
	for (int32 I = 0; I < 18; ++I)   // фрукты и овощи на прилавке
	{
		if (R.F() < 0.5f) FR.Color = C3(200, 60, 40);
		else FR.Color = R.F() < 0.5f ? C3(230, 180, 50) : C3(90, 150, 60);
		const float FX = R.Range(-1.f, 1.f);
		const float FZ = R.Range(-0.45f, 0.45f);
		MbSphere(FR, FV3(FX, 0.98f, FZ), FV3(0.08f, 0.08f, 0.08f), 6, 4);
	}
	MbTransform(FR, S2, Up, Yaw, P);
	PhysObb(FV3(P.X, P.Y + 0.45f, P.Z), Yaw, FV3(1.2f, 0.45f, 0.6f));
}

void FBuilder::Fountain(const FV3& P, float R)
{
	FMeshBuf& M = Mb(P, CM_Stone);
	M.Color = C3(225, 215, 195);
	const FV2 Rim[6] = { FV2(R, 0.f), FV2(R + 0.1f, 0.1f), FV2(R + 0.1f, 0.55f), FV2(R - 0.05f, 0.65f), FV2(R - 0.25f, 0.65f), FV2(R - 0.3f, 0.2f) };
	MbLathe(M, P, Rim, 6, 40, 0.45f);
	const FV2 Column[7] = { FV2(0.6f, 0.2f), FV2(0.45f, 0.5f), FV2(0.3f, 1.4f), FV2(0.9f, 1.6f), FV2(0.95f, 1.75f), FV2(0.25f, 1.9f), FV2(0.15f, 2.8f) };
	MbLathe(M, P, Column, 7, 24, 0.45f);
	FMeshBuf& W = Mb(P, CM_Water);
	W.Color = White;
	const FV3 Up(0.f, 1.f, 0.f);
	const int32 Base = W.Vertex(FV3(P.X, P.Y + 0.5f, P.Z), Up, FV2(0.f, 0.f));
	for (int32 I = 0; I <= 32; ++I)
	{
		const float A = CityTau * (float)I / 32.f;
		W.Vertex(FV3(P.X + FMath::Sin(A) * (R - 0.25f), P.Y + 0.5f, P.Z + FMath::Cos(A) * (R - 0.25f)), Up, FV2(FMath::Sin(A), FMath::Cos(A)));
	}
	for (int32 I = 0; I < 32; ++I) W.Tri(Base, Base + 1 + I, Base + 2 + I);
	// столкновения: кольцо из коробок и столб
	for (int32 I = 0; I < 12; ++I)
	{
		const float A = CityTau * (float)I / 12.f;
		PhysObb(FV3(P.X + FMath::Sin(A) * R, P.Y + 0.32f, P.Z + FMath::Cos(A) * R), A, FV3(R * 0.3f, 0.33f, 0.2f));
	}
	PhysBox(FV3(P.X - 0.5f, P.Y, P.Z - 0.5f), FV3(P.X + 0.5f, P.Y + 2.8f, P.Z + 0.5f));
}

void FBuilder::Crates(const FV3& P, int32 Count, uint32 Seed)
{
	FRng R((uint64)Seed);
	for (int32 I = 0; I < Count; ++I)
	{
		const float S = R.Range(0.7f, 1.f);
		const float QX = P.X + R.Range(-0.4f, 0.4f) + (float)(I % 2) * 1.0f;
		const float QZ = P.Z + R.Range(-0.3f, 0.3f);
		FV3 Q(QX, P.Y + (float)(I / 2) * 0.95f, QZ);
		if (I >= 2) { Q.X = P.X + 0.5f; Q.Y = P.Y + (float)(I - 1) * 0.95f; }   // ступенькой вверх — удобно забираться
		Box(CM_Wood, FV3(Q.X - S * 0.5f, Q.Y, Q.Z - S * 0.5f), FV3(Q.X + S * 0.5f, Q.Y + S * 0.95f, Q.Z + S * 0.5f), 1.f / S, C3(170, 130, 90), true);
	}
}

// ---------------------------------------------------------------- проверки для точек

float FBuilder::GroundHeight(float X, float Z) const
{
	float Y = 0.f;
	for (const FFlat& F : Flats)
	{
		if (X >= F.X0 && X <= F.X1 && Z >= F.Z0 && Z <= F.Z1) Y = FMath::Max(Y, F.Y);
	}
	return Y;
}

bool FBuilder::CapsuleBlocked(const FV3& P, float R) const
{
	const float Y0 = P.Y + 0.4f, Y1 = P.Y + 1.6f;
	for (const FSolid& S : Solids)
	{
		const float DX = FMath::Max(FMath::Max(S.Mn.X - P.X, 0.f), P.X - S.Mx.X);
		const float DZ = FMath::Max(FMath::Max(S.Mn.Z - P.Z, 0.f), P.Z - S.Mx.Z);
		const float DY = FMath::Max(FMath::Max(S.Mn.Y - Y1, 0.f), Y0 - S.Mx.Y);
		if (DX * DX + DZ * DZ + DY * DY < R * R) return true;
	}
	return false;
}

int32 FBuilder::CountTriangles() const
{
	int32 N = 0;
	for (const FMeshBuf& M : Bufs) N += M.Idx.Num() / 3;
	for (const FMeshBuf& M : SoftBufs) N += M.Idx.Num() / 3;
	return N;
}

// ---------------------------------------------------------------- квартал

void FBuilder::BuildLevel(int32 Seed)
{
	Init(-20.f, -20.f, 160.f, 140.f, 24.f);
	// земля: булыжник улиц и плиты тротуаров/площади
	Ground(CM_Cobble, -20.f, -20.f, 160.f, 140.f, 0.f, 1.f, true);
	// при Seed = 1789 раскладка совпадает с песочницей Lutèce
	FRng R(20240789ull + (uint64)((int64)Seed - 1789) * 7919ull);
	const FColor Tints[6] = { C3(255, 250, 240), C3(250, 238, 215), C3(238, 228, 210), C3(255, 242, 225), C3(232, 225, 214), C3(245, 232, 205) };
	const FColor Shut[5] = { C3(60, 80, 70), C3(90, 50, 40), C3(70, 90, 110), C3(200, 195, 180), C3(50, 60, 80) };
	// кварталы 3×2, между ними улицы 8 м; в каждом квартале переулок 3.2 м
	const float BXS[3][2] = { { 0.f, 30.f }, { 38.f, 68.f }, { 104.f, 134.f } };
	const float BZS[2][2] = { { 0.f, 26.f }, { 82.f, 108.f } };
	for (int32 BI = 0; BI < 3; ++BI)
	{
		for (int32 BJ = 0; BJ < 2; ++BJ)
		{
			const float QX0 = BXS[BI][0], QX1 = BXS[BI][1], QZ0 = BZS[BJ][0], QZ1 = BZS[BJ][1];
			Ground(CM_Flag, QX0 - 2.f, QZ0 - 2.f, QX1 + 2.f, QZ1 + 2.f, 0.12f, 1.f, true);
			const float XM = (QX0 + QX1) * 0.5f, Alley = 3.2f, ZM = (QZ0 + QZ1) * 0.5f;
			const float Lots[4][4] = {
				{ QX0, QZ0, XM - Alley * 0.5f, ZM }, { XM + Alley * 0.5f, QZ0, QX1, ZM },
				{ QX0, ZM, XM - Alley * 0.5f, QZ1 }, { XM + Alley * 0.5f, ZM, QX1, QZ1 } };
			for (int32 L = 0; L < 4; ++L)
			{
				// делим участок на 1–2 дома разной высоты
				const float LX0 = Lots[L][0], LZ0 = Lots[L][1], LX1 = Lots[L][2], LZ1 = Lots[L][3];
				const bool bSplit = R.F() < 0.6f;
				const int32 NB = bSplit ? 2 : 1;
				for (int32 S = 0; S < NB; ++S)
				{
					FBuilding B;
					float SX0 = LX0, SX1 = LX1;
					if (bSplit) { const float Mid = (LX0 + LX1) * 0.5f; if (S == 0) SX1 = Mid; else SX0 = Mid; }
					B.X0 = SX0; B.Z0 = LZ0; B.X1 = SX1; B.Z1 = LZ1; B.BaseY = 0.12f;
					B.Floors = 3 + (int32)(R.F() * 3.f); B.FloorH = 3.1f; B.GroundH = 3.6f;
					const float WMR = R.F();
					B.WallMat = WMR < 0.55f ? CM_Stone : (WMR < 0.9f ? CM_Plaster : CM_Brick);
					B.Tint = Tints[R.U32() % 6];
					if (R.F() < 0.75f) B.ShutterCol = Shut[R.U32() % 5];
					else B.ShutterCol = FColor(0, 0, 0, 0);
					B.TrimCol = C3(235, 230, 220);
					const float RF = R.F();
					B.Roof = RF < 0.6f ? Roof_Mansard : (RF < 0.85f ? Roof_Gable : Roof_Flat);
					// стороны на улицу: внешние стороны квартала и переулок
					int32 Sides = 0;
					if (FMath::Abs(B.Z0 - QZ0) < 0.1f) Sides |= 1;
					if (FMath::Abs(B.Z1 - QZ1) < 0.1f) Sides |= 4;
					if (FMath::Abs(B.X1 - QX1) < 0.1f || FMath::Abs(B.X1 - (XM - Alley * 0.5f)) < 0.1f) Sides |= 2;
					if (FMath::Abs(B.X0 - QX0) < 0.1f || FMath::Abs(B.X0 - (XM + Alley * 0.5f)) < 0.1f) Sides |= 8;
					B.StreetSides = Sides;
					B.Shops = Sides & (R.F() < 0.6f ? 5 : 0);
					B.BalconyFloor = R.F() < 0.6f ? 1 : 0;
					B.Seed = R.U32();
					Building(B);
				}
			}
		}
	}
	// площадь (между кварталами по центру)
	const float PX0 = 30.f, PX1 = 104.f, PZ0 = 34.f, PZ1 = 74.f;
	Ground(CM_Flag, PX0, PZ0, PX1, PZ1, 0.06f, 0.7f, true);
	auto At = [this](float X, float Z) { return FV3(X, GroundHeight(X, Z), Z); };
	Fountain(At(67.f, 54.f), 3.4f);
	for (int32 I = 0; I < 6; ++I) Stall(At(44.f + (float)I * 4.2f, 40.f), 0.f, (I & 1) ? C3(170, 40, 40) : C3(230, 220, 200));
	for (int32 I = 0; I < 6; ++I) Stall(At(70.f + (float)I * 4.2f, 68.f), CityPi, (I & 1) ? C3(40, 80, 140) : C3(200, 170, 60));
	for (int32 I = 0; I < 8; ++I)
	{
		Tree(At(34.f + (float)I * 9.f, 47.f), 7.5f, (uint32)I);
		Tree(At(34.f + (float)I * 9.f, 61.f), 7.f, (uint32)I + 50u);
	}
	HayCart(At(20.f, 31.f), 0.3f); HayCart(At(88.f, 76.5f), 1.4f); HayCart(At(52.f, 79.f), 0.f);
	Crates(At(58.f, 36.f), 3, 5u); Crates(At(96.f, 60.f), 4, 9u); Crates(At(14.f, 30.f), 2, 3u);
	for (float X = -10.f; X < 150.f; X += 16.f)
	{
		Lamp(At(X, -3.f)); Lamp(At(X, 29.f)); Lamp(At(X, 79.f)); Lamp(At(X, 111.f));
	}
	// точки для прогулок горожан: тротуары и площадь
	for (int32 I = 0; I < 200; ++I)
	{
		FV3 Pt;
		const float U = R.F();
		if (U < 0.4f) { Pt.X = R.Range(PX0 + 2.f, PX1 - 2.f); Pt.Z = R.Range(PZ0 + 2.f, PZ1 - 2.f); }
		else if (U < 0.7f) { Pt.X = R.Range(-8.f, 140.f); Pt.Z = R.F() < 0.5f ? -1.5f : 28.f; }
		else { Pt.X = R.Range(-8.f, 140.f); Pt.Z = R.F() < 0.5f ? 80.5f : 110.f; }
		Pt.Y = GroundHeight(Pt.X, Pt.Z);
		if (CapsuleBlocked(Pt, 0.35f)) continue;
		WalkPts.Add(Pt);
	}
	// старт героя: на площади к западу от фонтана, лицом к нему
	PlayerStart = At(60.f, 54.f);
}

} // namespace TPCity

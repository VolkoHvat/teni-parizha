// Тени Парижа — процедурные PBR-текстуры (порт lu_texgen.c): бесшовный шум, ячейки Вороного, кладка, черепица, доски,
// карты нормалей и затенения из высот. Строки изображения считаются параллельно (ParallelFor).
#include "TPTexGen.h"
#include "Async/ParallelFor.h"

namespace TPTexGenImpl
{
	static constexpr float TexPi = 3.14159265358979f;
	static constexpr float TexTau = 6.28318530717959f;

	static inline uint32 Hash(uint32 X)
	{
		X ^= X >> 16; X *= 0x7feb352dU; X ^= X >> 15; X *= 0x846ca68bU; X ^= X >> 16;
		return X;
	}
	static inline float H01(uint32 H) { return (float)(H & 0xFFFFFFu) / 16777216.f; }
	static inline uint32 H3(int32 X, int32 Y, uint32 S)
	{
		return Hash((uint32)X * 73856093u ^ (uint32)Y * 19349663u ^ S * 83492791u);
	}
	static inline float Clamp01(float T) { return T < 0.f ? 0.f : (T > 1.f ? 1.f : T); }
	static inline float ClampF(float T, float A, float B) { return T < A ? A : (T > B ? B : T); }
	static inline float Smooth(float T) { T = Clamp01(T); return T * T * (3.f - 2.f * T); }
	static inline float LerpF(float A, float B, float T) { return A + (B - A) * T; }
	static inline int32 Wrap(int32 V, int32 P) { return ((V % P) + P) % P; }

	static inline TPTexGen::FRGB Mul(const TPTexGen::FRGB& C, float S) { return TPTexGen::FRGB(C.R * S, C.G * S, C.B * S); }
	static inline TPTexGen::FRGB Mix(const TPTexGen::FRGB& A, const TPTexGen::FRGB& B, float T)
	{
		return TPTexGen::FRGB(LerpF(A.R, B.R, T), LerpF(A.G, B.G, T), LerpF(A.B, B.B, T));
	}

	/** таблица градиентов шума (вместо cos/sin в каждой точке) */
	struct FGradTable
	{
		float GX[1024];
		float GY[1024];
		FGradTable()
		{
			for (int32 I = 0; I < 1024; ++I)
			{
				const float A = (float)I / 1024.f * TexTau;
				GX[I] = FMath::Cos(A);
				GY[I] = FMath::Sin(A);
			}
		}
	};
	static const FGradTable& Grad()
	{
		static const FGradTable Table;   // потокобезопасная инициализация (C++11)
		return Table;
	}

	/** таблицы sRGB <-> линейный для мип-уровней альбедо */
	struct FSrgbTable
	{
		float ToLin[256];
		FSrgbTable()
		{
			for (int32 I = 0; I < 256; ++I) ToLin[I] = FMath::Pow((float)I / 255.f, 2.2f);
		}
	};
	static const FSrgbTable& Srgb()
	{
		static const FSrgbTable Table;
		return Table;
	}

	static inline uint8 ToByte(float V) { return (uint8)FMath::Clamp((int32)(V * 255.f + 0.5f), 0, 255); }

	static FColor PackSrgb(const TPTexGen::FRGB& C, float A)
	{
		const float R = FMath::Pow(Clamp01(C.R), 1.f / 2.2f);
		const float G = FMath::Pow(Clamp01(C.G), 1.f / 2.2f);
		const float B = FMath::Pow(Clamp01(C.B), 1.f / 2.2f);
		return FColor(ToByte(R), ToByte(G), ToByte(B), ToByte(Clamp01(A)));
	}

	static TPTexGen::FRGB VCol(const TPTexGen::FPalette& P, float T, float Jitter)
	{
		const TPTexGen::FRGB C = Mix(P.C0, P.C1, Clamp01(T));
		return Mul(C, 1.f + Jitter * P.Var);
	}

	/** уменьшить изображение вдвое (бесшовно — размер степень двойки) */
	static void Downsample(const TArray<FColor>& Src, int32 SN, TArray<FColor>& Dst, int32 Kind /*0 sRGB, 1 normal, 2 linear*/)
	{
		const int32 DN = FMath::Max(1, SN / 2);
		Dst.SetNumUninitialized(DN * DN);
		const FSrgbTable& T = Srgb();
		for (int32 Y = 0; Y < DN; ++Y)
		{
			for (int32 X = 0; X < DN; ++X)
			{
				const int32 X0 = FMath::Min(X * 2, SN - 1), X1 = FMath::Min(X * 2 + 1, SN - 1);
				const int32 Y0 = FMath::Min(Y * 2, SN - 1), Y1 = FMath::Min(Y * 2 + 1, SN - 1);
				const FColor& A = Src[Y0 * SN + X0];
				const FColor& B = Src[Y0 * SN + X1];
				const FColor& C = Src[Y1 * SN + X0];
				const FColor& D = Src[Y1 * SN + X1];
				FColor Out;
				if (Kind == 0)
				{
					const float R = (T.ToLin[A.R] + T.ToLin[B.R] + T.ToLin[C.R] + T.ToLin[D.R]) * 0.25f;
					const float G = (T.ToLin[A.G] + T.ToLin[B.G] + T.ToLin[C.G] + T.ToLin[D.G]) * 0.25f;
					const float Bl = (T.ToLin[A.B] + T.ToLin[B.B] + T.ToLin[C.B] + T.ToLin[D.B]) * 0.25f;
					Out = PackSrgb(TPTexGen::FRGB(R, G, Bl), ((float)A.A + B.A + C.A + D.A) / (4.f * 255.f));
				}
				else if (Kind == 1)
				{
					float NX = ((float)A.R + B.R + C.R + D.R) / (4.f * 127.5f) - 1.f;
					float NY = ((float)A.G + B.G + C.G + D.G) / (4.f * 127.5f) - 1.f;
					float NZ = ((float)A.B + B.B + C.B + D.B) / (4.f * 127.5f) - 1.f;
					const float L = FMath::Sqrt(NX * NX + NY * NY + NZ * NZ);
					if (L > 1e-5f) { NX /= L; NY /= L; NZ /= L; } else { NX = 0.f; NY = 0.f; NZ = 1.f; }
					Out = FColor(ToByte(NX * 0.5f + 0.5f), ToByte(NY * 0.5f + 0.5f), ToByte(NZ * 0.5f + 0.5f), 255);
				}
				else
				{
					Out = FColor((uint8)(((int32)A.R + B.R + C.R + D.R + 2) / 4), (uint8)(((int32)A.G + B.G + C.G + D.G + 2) / 4),
						(uint8)(((int32)A.B + B.B + C.B + D.B + 2) / 4), (uint8)(((int32)A.A + B.A + C.A + D.A + 2) / 4));
				}
				Dst[Y * DN + X] = Out;
			}
		}
	}

	static void BuildMips(TArray<TArray<FColor>>& Mips, int32 N, int32 Kind)
	{
		int32 S = N;
		while (S > 1)
		{
			TArray<FColor> Next;
			Downsample(Mips.Last(), S, Next, Kind);
			Mips.Add(MoveTemp(Next));
			S /= 2;
		}
	}
}

float TPTexGen::Noise(float X, float Y, int32 P, uint32 Seed)
{
	using namespace TPTexGenImpl;
	const FGradTable& G = Grad();
	const int32 XI = FMath::FloorToInt(X), YI = FMath::FloorToInt(Y);
	const float FX = X - (float)XI, FY = Y - (float)YI;
	const float UX = FX * FX * FX * (FX * (FX * 6.f - 15.f) + 10.f), UY = FY * FY * FY * (FY * (FY * 6.f - 15.f) + 10.f);
	float R = 0.f;
	for (int32 J = 0; J < 2; ++J)
	{
		for (int32 I = 0; I < 2; ++I)
		{
			const int32 CX = Wrap(XI + I, P), CY = Wrap(YI + J, P);
			const uint32 H = H3(CX, CY, Seed);
			const int32 GI = (int32)((H & 0xFFFFu) >> 6);
			const float D = G.GX[GI] * (FX - (float)I) + G.GY[GI] * (FY - (float)J);
			const float W = (I ? UX : 1.f - UX) * (J ? UY : 1.f - UY);
			R += D * W;
		}
	}
	return R * 1.41f;
}

float TPTexGen::Fbm(float X, float Y, int32 P, int32 Octaves, uint32 Seed)
{
	float S = 0.f, A = 0.5f, NSum = 0.f;
	for (int32 I = 0; I < Octaves; ++I)
	{
		S += Noise(X, Y, P, Seed + (uint32)I * 101u) * A;
		NSum += A; X *= 2.f; Y *= 2.f; P *= 2; A *= 0.5f;
	}
	return S / NSum;
}

void TPTexGen::Voronoi(float X, float Y, int32 P, uint32 Seed, float& F1, float& F2, uint32& Id)
{
	using namespace TPTexGenImpl;
	const int32 XI = FMath::FloorToInt(X), YI = FMath::FloorToInt(Y);
	float B1 = 9.f, B2 = 9.f; uint32 BId = 0;
	for (int32 J = -1; J <= 1; ++J)
	{
		for (int32 I = -1; I <= 1; ++I)
		{
			const int32 CX = XI + I, CY = YI + J;
			const uint32 H = H3(Wrap(CX, P), Wrap(CY, P), Seed);
			const float PX = (float)CX + 0.15f + 0.7f * H01(H);
			const float PY = (float)CY + 0.15f + 0.7f * H01((H >> 7) ^ (H * 31u));
			const float D = FMath::Sqrt((PX - X) * (PX - X) + (PY - Y) * (PY - Y));
			if (D < B1) { B2 = B1; B1 = D; BId = H; }
			else if (D < B2) { B2 = D; }
		}
	}
	F1 = B1; F2 = B2; Id = BId;
}

void TPTexGen::Generate(FSet& Out, EType Type, int32 N, const FPalette& Pal, uint32 Seed, float FlatRough, float FlatMetal, bool bMips)
{
	using namespace TPTexGenImpl;
	N = FMath::Max(N, 1);
	Out.N = N;
	Out.Albedo.Reset(); Out.Normal.Reset(); Out.ORM.Reset();
	Out.Albedo.AddDefaulted(1); Out.Normal.AddDefaulted(1); Out.ORM.AddDefaulted(1);
	TArray<FColor>& Alb = Out.Albedo[0];
	TArray<FColor>& Nrm = Out.Normal[0];
	TArray<FColor>& Orm = Out.ORM[0];
	Alb.SetNumUninitialized(N * N); Nrm.SetNumUninitialized(N * N); Orm.SetNumUninitialized(N * N);

	if (Type == EType::Flat)
	{
		const FColor A = PackSrgb(Pal.C0, 1.f);
		const FColor NFlat(128, 128, 255, 255);
		const FColor O(255, ToByte(Clamp01(FlatRough)), ToByte(Clamp01(FlatMetal)), 255);
		for (int32 I = 0; I < N * N; ++I) { Alb[I] = A; Nrm[I] = NFlat; Orm[I] = O; }
		Out.Average = Pal.C0;
		if (bMips) { BuildMips(Out.Albedo, N, 0); BuildMips(Out.Normal, N, 1); BuildMips(Out.ORM, N, 2); }
		return;
	}

	TArray<float> Hgt, Rough;
	Hgt.SetNumUninitialized(N * N); Rough.SetNumUninitialized(N * N);
	TArray<FRGB> RowSum;
	RowSum.SetNum(N);
	const float Metal = Type == EType::Metal ? 1.f : 0.f;
	float Strength = 3.f;
	const float AO = 3.f;
	if (Type == EType::Cloth) Strength = 1.f;
	if (Type == EType::Cobble || Type == EType::Slate || Type == EType::RoofTiles) Strength = 4.f;
	const FPalette P = Pal;
	const int32 Size = N;

	// --- 1. высоты, альбедо, шероховатость ---
	ParallelFor(Size, [&Hgt, &Rough, &Alb, &RowSum, Type, Size, P, Seed](int32 PY)
	{
		FRGB Sum(0.f, 0.f, 0.f);
		for (int32 PX = 0; PX < Size; ++PX)
		{
			const float U = ((float)PX + 0.5f) / (float)Size, V = ((float)PY + 0.5f) / (float)Size;
			float H = 0.5f, A = 1.f, R = 0.85f;
			FRGB C = P.C0;
			const float Grain = Fbm(U * 16.f, V * 16.f, 16, 4, Seed);
			switch (Type)
			{
			case EType::StoneBlocks: case EType::Brick: case EType::Flagstone:
			{
				const bool bBrick = Type == EType::Brick, bFlag = Type == EType::Flagstone;
				const int32 Rows = bBrick ? 8 : (bFlag ? 3 : 4), Cols = bBrick ? 4 : (bFlag ? 3 : 2);
				const float Mort = bBrick ? 0.1f : (bFlag ? 0.025f : 0.035f);
				const float FY = V * (float)Rows; const int32 Row = (int32)FY;
				float Ofs = (Row & 1) ? 0.5f : 0.f;
				if (bFlag) Ofs = H01(H3(Row, 0, Seed)) * 0.6f;
				const float FX = U * (float)Cols + Ofs; const int32 Col = FMath::FloorToInt(FX);
				const float LX = FX - (float)Col, LY = FY - (float)Row;
				const uint32 BId = H3(Wrap(Col, Cols), Row, Seed);
				const float Edge = FMath::Min(FMath::Min(LX, 1.f - LX) * (float)Rows / (float)Cols * (bBrick ? 2.f : 1.f), FMath::Min(LY, 1.f - LY));
				const float Wob = Fbm(U * 32.f, V * 32.f, 32, 3, Seed + 7) * 0.012f;
				float M = Smooth((Edge + Wob - Mort * 0.5f) / (Mort * 0.8f));
				const float Chips = Fbm(U * 48.f, V * 48.f, 48, 3, Seed + 3);
				M *= 1.f - Smooth((Chips - 0.35f) * 6.f) * Smooth((0.12f - Edge) * 20.f) * 0.7f;
				H = M * (0.85f + 0.15f * Smooth(Edge * 6.f)) + Grain * 0.05f;
				C = VCol(P, H01(BId), H01(BId >> 9) * 2.f - 1.f);
				C = Mul(C, 0.92f + 0.16f * Grain);
				const FRGB Mortar = Mul(Mix(P.C0, FRGB(0.5f, 0.5f, 0.48f), 0.5f), 0.7f);
				C = Mix(Mortar, C, M);
				// копоть (в оригинале период по вертикали ломал бесшовность — здесь период общий)
				const float Grime = Smooth(Fbm(U * 4.f, V * 4.f, 4, 3, Seed + 11) * 1.5f + 0.2f) * 0.35f;
				C = Mul(C, 1.f - Grime * 0.75f);
				R = LerpF(0.95f, 0.75f + 0.1f * Grain, M);
				break;
			}
			case EType::Plaster:
			{
				const float N1 = Fbm(U * 8.f, V * 8.f, 8, 5, Seed);
				float F1 = 0.f, F2 = 0.f; uint32 CellId = 0;
				Voronoi(U * 6.f, V * 6.f, 6, Seed + 5, F1, F2, CellId);
				const float Crack = Smooth(1.f - (F2 - F1) * 25.f) * Smooth(Fbm(U * 4.f, V * 4.f, 4, 2, Seed + 9) * 3.f);
				H = 0.5f + N1 * 0.25f - Crack * 0.3f;
				C = VCol(P, 0.5f + N1, N1);
				const float Stain = Smooth(Fbm(U * 3.f, V * 3.f, 3, 4, Seed + 2) * 2.f) * 0.3f;
				C = Mul(C, (1.f - Stain) * (1.f - Crack * 0.5f));
				R = 0.9f;
				break;
			}
			case EType::Cobble:
			{
				float F1 = 0.f, F2 = 0.f; uint32 CellId = 0;
				const float W = Fbm(U * 10.f, V * 10.f, 10, 2, Seed + 3) * 0.15f;
				Voronoi(U * 10.f + W, V * 10.f + W, 10, Seed, F1, F2, CellId);
				const float M = Smooth((F2 - F1) * 4.5f);
				H = M * (0.75f + 0.25f * H01(CellId)) + Grain * 0.04f;
				C = Mix(FRGB(0.12f, 0.11f, 0.1f), VCol(P, H01(CellId), H01(CellId >> 8) * 2.f - 1.f), M);
				C = Mul(C, 0.9f + 0.2f * Grain);
				R = LerpF(0.95f, 0.55f + 0.2f * H01(CellId >> 4), M);
				break;
			}
			case EType::Slate: case EType::RoofTiles:
			{
				const bool bSlate = Type == EType::Slate;
				const int32 Rows = bSlate ? 10 : 8, Cols = 6;
				const float FY = V * (float)Rows; const int32 Row = (int32)FY; const float LY = FY - (float)Row;
				const float FX = U * (float)Cols + ((Row & 1) ? 0.5f : 0.f); const int32 Col = FMath::FloorToInt(FX); const float LX = FX - (float)Col;
				const uint32 TileId = H3(Wrap(Col, Cols), Row, Seed);
				if (bSlate)
				{
					H = LY * 0.9f + 0.1f;
					const float Side = FMath::Min(LX, 1.f - LX);
					H *= Smooth(Side * 30.f);
					C = VCol(P, H01(TileId), H01(TileId >> 8) * 2.f - 1.f);
					C = Mul(C, 0.7f + 0.4f * LY);
					R = 0.55f + 0.2f * Grain;
				}
				else
				{
					const float Arch = FMath::Sin(LX * TexPi);
					H = Arch * 0.7f + LY * 0.3f;
					C = VCol(P, H01(TileId), H01(TileId >> 8) * 2.f - 1.f);
					C = Mul(C, 0.6f + 0.4f * Arch * (0.6f + 0.4f * LY));
					R = 0.7f;
				}
				C = Mul(C, 0.92f + 0.16f * Grain);
				const float Moss = Smooth(Fbm(U * 5.f, V * 5.f, 5, 3, Seed + 4) * 2.5f - 0.4f) * 0.4f;
				C = Mix(C, FRGB(0.16f, 0.19f, 0.1f), Moss * (1.f - LY));
				break;
			}
			case EType::Wood: case EType::Shutter:
			{
				const bool bShutter = Type == EType::Shutter;
				const int32 Planks = bShutter ? 8 : 5;
				const float FX = U * (float)Planks; const int32 Pk = (int32)FX; const float LX = FX - (float)Pk;
				const float G = Fbm(U * 4.f + H01(H3(Pk, 0, Seed)) * 7.f, V * 40.f, 4, 4, Seed + (uint32)Pk);
				if (bShutter)
				{
					const float LY = FMath::Fmod(V * 16.f, 1.f);
					H = Smooth(LY * 4.f) * Smooth((1.f - LY) * 1.5f) * 0.8f;
					C = VCol(P, 0.5f + G, G);
					C = Mul(C, 0.75f + 0.35f * H);
					R = 0.7f;
				}
				else
				{
					H = 0.6f + G * 0.2f;
					H *= Smooth(FMath::Min(LX, 1.f - LX) * 40.f);
					C = VCol(P, H01(H3(Pk, 1, Seed)), G * 2.f);
					C = Mul(C, 0.85f + 0.3f * G);
					R = 0.75f;
				}
				break;
			}
			case EType::Dirt:
			{
				const float N1 = Fbm(U * 8.f, V * 8.f, 8, 6, Seed);
				H = 0.5f + N1 * 0.4f;
				C = VCol(P, 0.5f + N1 * 1.5f, N1);
				R = 0.95f;
				break;
			}
			case EType::Cloth:
			{
				// саржевое плетение (частота подобрана под бесшовность) и широкие полосы, как у навесов
				const float WX = FMath::Sin(U * TexTau * 40.f), WY = FMath::Sin(V * TexTau * 40.f);
				H = 0.5f + 0.25f * (WX * (WY > 0.f ? 1.f : 0.f) + WY * (WX <= 0.f ? 1.f : 0.f));
				C = VCol(P, 0.5f + Grain, Grain * 0.5f);
				const float Stripe = FMath::Fmod(U * 4.f, 1.f) < 0.5f ? 1.f : 0.68f;
				C = Mul(C, (0.9f + 0.1f * H) * Stripe);
				R = 0.95f;
				break;
			}
			case EType::Metal:
			{
				const float B = Fbm(U * 2.f, V * 64.f, 2, 4, Seed);
				H = 0.5f + B * 0.1f;
				C = VCol(P, 0.5f + B, 0.f);
				R = 0.35f + 0.15f * B;
				break;
			}
			case EType::Leaves:
			{
				float F1 = 0.f, F2 = 0.f; uint32 CellId = 0;
				const float W = Fbm(U * 6.f, V * 6.f, 6, 2, Seed + 3) * 0.25f;
				Voronoi(U * 12.f + W, V * 12.f - W, 12, Seed, F1, F2, CellId);
				const float M = Smooth((F2 - F1) * 3.f - 0.08f);
				const bool bHole = H01(CellId >> 3) < 0.16f;
				A = bHole ? 0.f : (M > 0.12f ? 1.f : 0.f);
				H = M * 0.8f + Grain * 0.1f;
				C = VCol(P, H01(CellId), H01(CellId >> 8) * 2.f - 1.f);
				C = Mul(C, 0.65f + 0.35f * M);
				R = 0.7f;
				break;
			}
			default: break;
			}
			const int32 Idx = PY * Size + PX;
			Hgt[Idx] = H;
			Alb[Idx] = PackSrgb(C, A);
			Rough[Idx] = R;
			Sum.R += Clamp01(C.R); Sum.G += Clamp01(C.G); Sum.B += Clamp01(C.B);
		}
		RowSum[PY] = Sum;
	});

	// --- 2. нормали (как в UE/DirectX: зелёный — в сторону роста V, т. е. вниз по изображению) и ORM ---
	ParallelFor(Size, [&Hgt, &Rough, &Nrm, &Orm, Size, Strength, AO, Metal](int32 PY)
	{
		const int32 NN = Size;
		for (int32 PX = 0; PX < NN; ++PX)
		{
			const float HL = Hgt[PY * NN + (PX + NN - 1) % NN], HR = Hgt[PY * NN + (PX + 1) % NN];
			const float HD = Hgt[((PY + NN - 1) % NN) * NN + PX], HU = Hgt[((PY + 1) % NN) * NN + PX];
			const float K = Strength * (float)NN / 256.f;
			float NX = -(HR - HL) * K, NY = -(HU - HD) * K, NZ = 1.f;
			const float L = FMath::Sqrt(NX * NX + NY * NY + NZ * NZ);
			NX /= L; NY /= L; NZ /= L;
			Nrm[PY * NN + PX] = FColor(ToByte(NX * 0.5f + 0.5f), ToByte(NY * 0.5f + 0.5f), ToByte(NZ * 0.5f + 0.5f), 255);
			// затенение: насколько точка ниже соседей
			const float H0 = Hgt[PY * NN + PX];
			float Avg = 0.f; int32 Cnt = 0;
			for (int32 Kk = 1; Kk <= 4; ++Kk)
			{
				const int32 D = (Kk * NN / 128 + 1) % NN;
				Avg += Hgt[PY * NN + (PX + D) % NN] + Hgt[PY * NN + (PX + NN - D) % NN]
					+ Hgt[((PY + D) % NN) * NN + PX] + Hgt[((PY + NN - D) % NN) * NN + PX];
				Cnt += 4;
			}
			Avg /= (float)Cnt;
			const float O = ClampF(1.f - FMath::Max(0.f, Avg - H0) * AO, 0.2f, 1.f);
			Orm[PY * NN + PX] = FColor(ToByte(O), ToByte(Clamp01(Rough[PY * NN + PX])), ToByte(Metal), 255);
		}
	});

	FRGB Total(0.f, 0.f, 0.f);
	for (const FRGB& S : RowSum) { Total.R += S.R; Total.G += S.G; Total.B += S.B; }
	const float Inv = 1.f / (float)(N * N);
	Out.Average = FRGB(Total.R * Inv, Total.G * Inv, Total.B * Inv);

	if (bMips)
	{
		BuildMips(Out.Albedo, N, 0);
		BuildMips(Out.Normal, N, 1);
		BuildMips(Out.ORM, N, 2);
	}
}

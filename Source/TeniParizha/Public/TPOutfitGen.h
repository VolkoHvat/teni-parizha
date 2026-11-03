// Тени Парижа — построитель одежды 1789 года на скелете манекена (только заголовок, без UObject).
//
// Одежда собирается из жёстких частей, по одной на кость (таз, поясница, грудь, шея, голова, плечо, предплечье,
// кисть, бедро, голень, стопа). Каждая часть — меш в пространстве персонажа в позе покоя скелета (A-поза манекена):
// в игре её цепляют к кости с относительным преобразованием «обратное покою кости» — в покое она стоит на месте,
// а в движении поворачивается вместе с костью. Скелетный меш манекена при этом скрыт и служит только каркасом.
// Стыки суставов закрыты шарами в центре сустава (шар не меняется при повороте вокруг своего центра).
//
// Пространство персонажа: сантиметры, X — вперёд, Y — вправо, Z — вверх, пол — Z = 0.
// Высоты таблиц заданы для эталонного манекена и переносятся на реальный скелет по опорным суставам (ZR).
// Лицевая сторона треугольника — по часовой стрелке при взгляде снаружи (как в UE).
#pragma once

#include "CoreMinimal.h"
#include "TPAnimPoseMath.h"

namespace TPOutfit
{
	// ---------------- описание наряда ----------------
	enum ECoat : uint8 { Coat_None, Coat_Habit, Coat_Short, Coat_Uniform, Coat_Jacket };   // кафтан до колен, короткий (герой), мундир, куртка
	enum ELegs : uint8 { Legs_Culottes, Legs_Trousers };                                     // кюлоты до колен, длинные штаны
	enum EFeet : uint8 { Feet_Shoes, Feet_Boots, Feet_Gaiters };                             // туфли с пряжкой, сапоги, гетры
	enum EHat : uint8 { Hat_None, Hat_Tricorne, Hat_Cap, Hat_Bonnet };                       // треуголка, чепец, колпак
	enum EHair : uint8 { Hair_Queue, Hair_Bun };                                             // косица с бантом, узел

	struct FDesc
	{
		bool bFemale = false;
		ECoat Coat = Coat_Habit;
		bool bWaistcoat = true;       // камзол (иначе видна рубаха)
		bool bDress = false;          // платье: лиф + юбка до пола (ноги не строятся)
		bool bApron = false;          // фартук
		bool bFichu = false;          // косынка на плечах
		bool bStripes = false;        // полоски на штанах / юбке
		ELegs Legs = Legs_Culottes;
		EFeet Feet = Feet_Shoes;
		EHat Hat = Hat_Tricorne;
		EHair Hair = Hair_Queue;
		bool bCrossBelts = false;     // белые перевязи стражи
		bool bSash = false;           // кушак
		bool bSword = false;          // шпага у левого бедра
		float Bulk = 1.f;             // полнота 0.9..1.2

		FColor Skin = FColor(224, 178, 146);
		FColor HairCol = FColor(70, 48, 32);
		FColor CoatCol = FColor(92, 62, 40);
		FColor Facing = FColor(92, 62, 40);     // отвороты, обшлага, воротник
		FColor Lining = FColor(150, 130, 100);  // подкладка полы
		FColor WaistcoatCol = FColor(210, 196, 160);
		FColor Shirt = FColor(236, 232, 220);
		FColor LegsCol = FColor(40, 36, 34);
		FColor Stripe = FColor(236, 232, 220);
		FColor Stockings = FColor(226, 222, 212);
		FColor ShoesCol = FColor(28, 24, 22);
		FColor HatCol = FColor(30, 28, 28);
		FColor HatTrim = FColor(30, 28, 28);
		FColor DressCol = FColor(120, 60, 54);
		FColor ApronCol = FColor(232, 226, 210);
		FColor SashCol = FColor(150, 30, 34);
		FColor Metal = FColor(190, 170, 110);
	};

	inline uint32 HashBytes(uint32 H, const void* Data, int32 N)
	{
		const uint8* P = static_cast<const uint8*>(Data);
		for (int32 I = 0; I < N; ++I) { H = (H ^ P[I]) * 16777619u; }
		return H;
	}

	// Ключ кэша мешей: одинаковое описание — одинаковые меши.
	inline uint32 HashDesc(const FDesc& D)
	{
		uint32 H = 2166136261u;
		const uint8 Flags[] = { (uint8)D.bFemale, (uint8)D.Coat, (uint8)D.bWaistcoat, (uint8)D.bDress, (uint8)D.bApron, (uint8)D.bFichu,
			(uint8)D.bStripes, (uint8)D.Legs, (uint8)D.Feet, (uint8)D.Hat, (uint8)D.Hair, (uint8)D.bCrossBelts, (uint8)D.bSash, (uint8)D.bSword };
		H = HashBytes(H, Flags, (int32)sizeof(Flags));
		const int32 BulkQ = (int32)(D.Bulk * 1000.f + 0.5f);
		H = HashBytes(H, &BulkQ, (int32)sizeof(BulkQ));
		const FColor Cols[] = { D.Skin, D.HairCol, D.CoatCol, D.Facing, D.Lining, D.WaistcoatCol, D.Shirt, D.LegsCol, D.Stripe,
			D.Stockings, D.ShoesCol, D.HatCol, D.HatTrim, D.DressCol, D.ApronCol, D.SashCol, D.Metal };
		for (const FColor& C : Cols) { const uint8 B[3] = { C.R, C.G, C.B }; H = HashBytes(H, B, 3); }
		return H;
	}

	// ---------------- готовые наряды ----------------
	// Герой: короткий тёмный кафтан (до середины бедра — не мешает бегу), бордовый камзол, сапоги, кушак, шпага, без шляпы.
	inline FDesc Hero()
	{
		FDesc D;
		D.Coat = Coat_Short; D.Feet = Feet_Boots; D.Hat = Hat_None; D.bSash = true; D.bSword = true;
		D.CoatCol = FColor(46, 52, 62); D.Facing = FColor(36, 40, 48); D.Lining = FColor(96, 30, 34);
		D.WaistcoatCol = FColor(110, 34, 38); D.LegsCol = FColor(70, 60, 50); D.ShoesCol = FColor(34, 26, 22);
		D.SashCol = FColor(40, 34, 30); D.HairCol = FColor(46, 34, 26); D.Metal = FColor(170, 170, 176);
		return D;
	}

	// Стража (французская гвардия): синий мундир с красными отворотами, белые камзол, кюлоты, гетры и перевязи, треуголка с галуном.
	inline FDesc Guard()
	{
		FDesc D;
		D.Coat = Coat_Uniform; D.Feet = Feet_Gaiters; D.Hat = Hat_Tricorne; D.bCrossBelts = true; D.bSword = true;
		D.CoatCol = FColor(34, 48, 96); D.Facing = FColor(170, 34, 36); D.Lining = FColor(170, 34, 36);
		D.WaistcoatCol = FColor(226, 222, 210); D.LegsCol = FColor(220, 216, 204); D.Stockings = FColor(232, 230, 222);
		D.HatCol = FColor(26, 24, 24); D.HatTrim = FColor(232, 228, 216); D.HairCol = FColor(214, 210, 200);   // пудреные волосы
		D.Metal = FColor(200, 180, 110);
		return D;
	}

	// Горожане. Variant: 0 — буржуа в кафтане и треуголке, 1 — мастеровой в куртке, полосатых штанах и красном колпаке,
	// 2 — лавочник в камзоле поверх рубахи.
	inline FDesc CitizenMale(int32 Variant)
	{
		FDesc D;
		switch (((Variant % 3) + 3) % 3)
		{
		case 0:
			D.Coat = Coat_Habit; D.Hat = Hat_Tricorne;
			D.CoatCol = FColor(104, 70, 44); D.Facing = FColor(96, 64, 40); D.Lining = FColor(170, 150, 112);
			D.WaistcoatCol = FColor(214, 198, 150); D.LegsCol = FColor(46, 40, 36);
			break;
		case 1:
			D.Coat = Coat_Jacket; D.bWaistcoat = false; D.Legs = Legs_Trousers; D.bStripes = true; D.Hat = Hat_Bonnet;
			D.CoatCol = FColor(58, 66, 92); D.Facing = FColor(58, 66, 92); D.Lining = FColor(58, 66, 92);
			D.LegsCol = FColor(214, 206, 186); D.Stripe = FColor(60, 74, 120); D.HatCol = FColor(168, 36, 34);
			D.Shirt = FColor(222, 214, 196); D.HairCol = FColor(40, 30, 24); D.ShoesCol = FColor(70, 52, 36);
			break;
		default:
			D.Coat = Coat_None; D.Hat = Hat_None; D.bApron = false;
			D.WaistcoatCol = FColor(120, 84, 52); D.LegsCol = FColor(86, 70, 54); D.Stockings = FColor(150, 146, 138);
			D.HairCol = FColor(110, 80, 50);
			break;
		}
		return D;
	}

	// Горожанки. Variant: 0 — работница (полосатая юбка, фартук, косынка, чепец), 1 — горожанка в платье с косынкой.
	inline FDesc CitizenFemale(int32 Variant)
	{
		FDesc D;
		D.bFemale = true; D.bDress = true; D.Coat = Coat_None; D.bWaistcoat = false; D.Hat = Hat_Cap; D.Hair = Hair_Bun; D.bFichu = true;
		D.Shirt = FColor(240, 236, 226); D.HatCol = FColor(242, 240, 232); D.ShoesCol = FColor(60, 44, 34);
		if (((Variant % 2) + 2) % 2 == 0)
		{
			D.bApron = true; D.bStripes = true;
			D.DressCol = FColor(176, 96, 70); D.Stripe = FColor(226, 212, 186); D.WaistcoatCol = FColor(70, 58, 52);
			D.ApronCol = FColor(214, 206, 186); D.HairCol = FColor(92, 60, 36);
		}
		else
		{
			D.DressCol = FColor(70, 96, 120); D.WaistcoatCol = FColor(70, 96, 120); D.HairCol = FColor(150, 110, 70);
		}
		return D;
	}

	// ---------------- результат ----------------
	enum ESection : int32 { S_Cloth, S_Smooth, S_Count };   // ткань (текстура переплетения) и гладкое (кожа, металл, кожа обуви)

	struct FPiece
	{
		TArray<FVector> Pos;          // пространство персонажа, поза покоя
		TArray<FVector> Nrm;
		TArray<FVector2D> UV;
		TArray<FColor> Col;           // sRGB
		TArray<int32> Idx[S_Count];   // треугольники по секциям
		int32 NumTris() const { return (Idx[S_Cloth].Num() + Idx[S_Smooth].Num()) / 3; }
		bool IsEmpty() const { return NumTris() == 0; }
	};

	struct FMesh
	{
		FPiece Pieces[TPAnim::B_Count];   // индекс — кость (TPAnim::EBone)
		int32 NumTris() const { int32 N = 0; for (int32 B = 0; B < TPAnim::B_Count; ++B) { N += Pieces[B].NumTris(); } return N; }
		int32 NumPieces() const { int32 N = 0; for (int32 B = 0; B < TPAnim::B_Count; ++B) { N += Pieces[B].IsEmpty() ? 0 : 1; } return N; }
	};

	// ---------------- построитель ----------------
	namespace Detail
	{
		using namespace TPAnim;

		inline FColor Mul(const FColor& C, float K)
		{
			auto Ch = [K](uint8 V) { const float X = V * K; return (uint8)(X < 0.f ? 0.f : X > 255.f ? 255.f : X + 0.5f); };
			return FColor(Ch(C.R), Ch(C.G), Ch(C.B), 255);
		}

		inline float Deg(float A) { return A * (PI / 180.f); }

		struct FBuilder
		{
			const FTPRig& R;
			const FDesc& D;
			FMesh& M;
			float K;   // масштаб скелета (1 — эталонный манекен)

			FBuilder(const FTPRig& InR, const FDesc& InD, FMesh& InM) : R(InR), D(InD), M(InM), K(InR.Scale) {}

			// высота эталонного манекена → высота реального скелета (кусочно-линейно по опорным суставам)
			float ZR(float Z) const
			{
				const float Std[6] = { 0.f, 92.f, 97.f, 137.f, 153.f, 162.f };
				const float Real[6] = { 0.f, R.HipH, R.RestPos[B_Pelvis].Z, R.RestPos[B_Spine3].Z, R.RestPos[B_Neck].Z, R.RestPos[B_Head].Z };
				if (Z >= Std[5]) { return Real[5] + (Z - Std[5]) * K; }
				for (int32 I = 1; I < 6; ++I)
				{
					if (Z <= Std[I])
					{
						const float T = (Z - Std[I - 1]) / (Std[I] - Std[I - 1]);
						return Real[I - 1] + T * (Real[I] - Real[I - 1]);
					}
				}
				return Z;
			}

			// ось позвоночника: X на высоте Z (между суставами таза, позвонков и шеи)
			float SpineX(float ZReal) const
			{
				const int32 Ch[5] = { B_Pelvis, B_Spine1, B_Spine2, B_Spine3, B_Neck };
				if (ZReal <= R.RestPos[Ch[0]].Z) { return R.RestPos[Ch[0]].X; }
				for (int32 I = 1; I < 5; ++I)
				{
					const FVector& A = R.RestPos[Ch[I - 1]];
					const FVector& B = R.RestPos[Ch[I]];
					if (ZReal <= B.Z) { const float T = (ZReal - A.Z) / FMath::Max(B.Z - A.Z, 0.01f); return A.X + T * (B.X - A.X); }
				}
				return R.RestPos[Ch[4]].X;
			}

			// ---- треугольник с лицевой стороной наружу (по часовой стрелке снаружи, как в UE) ----
			static void Tri(FPiece& P, int32 Sec, int32 A, int32 B, int32 C)
			{
				const FVector Fn = FVector::CrossProduct(P.Pos[B] - P.Pos[A], P.Pos[C] - P.Pos[A]);
				const FVector Nn = P.Nrm[A] + P.Nrm[B] + P.Nrm[C];
				if (Fn.SizeSquared() < 1e-10f) { return; }   // вырожденный (полюс) — пропускаем
				TArray<int32>& I = P.Idx[Sec];
				if (FVector::DotProduct(Fn, Nn) > 0.f) { I.Add(A); I.Add(C); I.Add(B); }
				else { I.Add(A); I.Add(B); I.Add(C); }
			}

			static int32 Vert(FPiece& P, const FVector& Pos, const FVector& N, const FVector2D& UV, const FColor& C)
			{
				P.Pos.Add(Pos); P.Nrm.Add(N); P.UV.Add(UV); P.Col.Add(C);
				return P.Pos.Num() - 1;
			}

			/**
			 * Сетка (NR+1)×(NS+1) точек: Pts[r*(NS+1)+s]. Нормали — по соседям, наружу от Ref[r] (центр кольца) или от Ref[0]
			 * при bOneRef. bWrap — столбец NS совпадает со столбцом 0 (замкнутое кольцо, шов для UV).
			 * LiningCol — если задан (A = 255), строится изнанка (двусторонняя ткань) этим цветом.
			 */
			void Grid(int32 Bone, int32 Sec, const TArray<FVector>& Pts, const TArray<FColor>& Cols, int32 NR, int32 NS,
				const TArray<FVector>& Ref, bool bOneRef, bool bWrap, const FColor& LiningCol, float UVScale = 1.f)
			{
				FPiece& P = M.Pieces[Bone];
				const int32 W = NS + 1;
				TArray<FVector> N;
				N.SetNumUninitialized((NR + 1) * W);
				for (int32 Rr = 0; Rr <= NR; ++Rr)
				{
					for (int32 S = 0; S <= NS; ++S)
					{
						int32 S0 = S - 1, S1 = S + 1;
						if (bWrap) { if (S0 < 0) { S0 = NS - 1; } if (S1 > NS) { S1 = 1; } }
						else { S0 = FMath::Max(S0, 0); S1 = FMath::Min(S1, NS); }
						const int32 R0 = FMath::Max(Rr - 1, 0), R1 = FMath::Min(Rr + 1, NR);
						const FVector DS = Pts[Rr * W + S1] - Pts[Rr * W + S0];
						const FVector DR = Pts[R1 * W + S] - Pts[R0 * W + S];
						FVector Nv = FVector::CrossProduct(DS, DR);
						const FVector& Pv = Pts[Rr * W + S];
						const FVector Out = Pv - Ref[bOneRef ? 0 : Rr];
						if (Nv.SizeSquared() < 1e-8f) { Nv = Out; }
						Nv = Nv.GetSafeNormal();
						if (FVector::DotProduct(Nv, Out) < 0.f) { Nv = -Nv; }
						if (Nv.SizeSquared() < 0.5f) { Nv = FVector(0.f, 0.f, 1.f); }
						N[Rr * W + S] = Nv;
					}
				}
				// UV: u — по окружности (длина дуги), v — вдоль (длина), в метрах × UVScale
				TArray<float> Us, Vs;
				Us.SetNumUninitialized((NR + 1) * W);
				Vs.SetNumUninitialized((NR + 1) * W);
				for (int32 Rr = 0; Rr <= NR; ++Rr)
				{
					float Acc = 0.f;
					for (int32 S = 0; S <= NS; ++S)
					{
						if (S > 0) { Acc += (Pts[Rr * W + S] - Pts[Rr * W + S - 1]).Size(); }
						Us[Rr * W + S] = Acc * 0.01f * UVScale;
					}
				}
				for (int32 S = 0; S <= NS; ++S)
				{
					float Acc = 0.f;
					for (int32 Rr = 0; Rr <= NR; ++Rr)
					{
						if (Rr > 0) { Acc += (Pts[Rr * W + S] - Pts[(Rr - 1) * W + S]).Size(); }
						Vs[Rr * W + S] = Acc * 0.01f * UVScale;
					}
				}
				const int32 Layers = LiningCol.A == 255 ? 2 : 1;
				for (int32 L = 0; L < Layers; ++L)
				{
					const int32 Base = P.Pos.Num();
					for (int32 I = 0; I < (NR + 1) * W; ++I)
					{
						if (L == 0) { Vert(P, Pts[I], N[I], FVector2D(Us[I], Vs[I]), Cols[I]); }
						else { Vert(P, Pts[I] - N[I] * 0.25f, -N[I], FVector2D(Us[I], Vs[I]), LiningCol); }
					}
					for (int32 Rr = 0; Rr < NR; ++Rr)
					{
						for (int32 S = 0; S < NS; ++S)
						{
							const int32 A = Base + Rr * W + S, B = A + 1, C = A + W, E = C + 1;
							Tri(P, Sec, A, B, E);
							Tri(P, Sec, A, E, C);
						}
					}
				}
			}

			// кольцо: центр, оси (U — вбок, V — вперёд), полуоси вбок / вперёд / назад, угловой диапазон (0 — вперёд, + к U)
			struct FRing
			{
				FVector C, U, V;
				float RU, RF, RB;
				float Th0, Th1;
				FColor Col;
			};

			static FVector RingPoint(const FRing& Q, float Th)
			{
				const float Cs = FMath::Cos(Th), Sn = FMath::Sin(Th);
				return Q.C + Q.U * (Sn * Q.RU) + Q.V * (Cs * (Cs >= 0.f ? Q.RF : Q.RB));
			}

			// Лофт по кольцам; CapA/CapB — купол на первом/последнем кольце (высота купола в долях радиуса)
			template<class FColFn>
			void Loft(int32 Bone, int32 Sec, const TArray<FRing>& Rings, int32 NS, const FColFn& ColFn, const FColor& LiningCol,
				float CapA = -1.f, float CapB = -1.f)
			{
				const int32 NR = Rings.Num() - 1;
				if (NR < 1) { return; }
				const bool bWrap = FMath::Abs(Rings[0].Th1 - Rings[0].Th0 - 2.f * PI) < 1e-3f;
				TArray<FVector> Pts, Ref;
				TArray<FColor> Cols;
				for (int32 Rr = 0; Rr <= NR; ++Rr)
				{
					const FRing& Q = Rings[Rr];
					Ref.Add(Q.C);
					for (int32 S = 0; S <= NS; ++S)
					{
						const float Th = Q.Th0 + (Q.Th1 - Q.Th0) * (float)S / (float)NS;
						Pts.Add(RingPoint(Q, Th));
						Cols.Add(ColFn(Rr, Th, Q.Col));
					}
				}
				Grid(Bone, Sec, Pts, Cols, NR, NS, Ref, false, bWrap, LiningCol);
				if (CapA >= 0.f) { Cap(Bone, Sec, Rings[0], Rings[1].C, CapA, NS, ColFn(0, 0.f, Rings[0].Col)); }
				if (CapB >= 0.f) { Cap(Bone, Sec, Rings[NR], Rings[NR - 1].C, CapB, NS, ColFn(NR, 0.f, Rings[NR].Col)); }
			}

			// купол на кольце, обращённый от точки Inner
			void Cap(int32 Bone, int32 Sec, const FRing& Q, const FVector& Inner, float H, int32 NS, const FColor& C)
			{
				const FVector Ax = (Q.C - Inner).GetSafeNormal();
				const float Rm = (Q.RU + 0.5f * (Q.RF + Q.RB)) * 0.5f;
				const int32 NR = 3;
				TArray<FVector> Pts, Ref;
				TArray<FColor> Cols;
				for (int32 Rr = 0; Rr <= NR; ++Rr)
				{
					const float A = 0.5f * PI * (float)Rr / (float)NR;   // 0 — край, π/2 — вершина
					FRing Sub = Q;
					Sub.RU = Q.RU * FMath::Cos(A); Sub.RF = Q.RF * FMath::Cos(A); Sub.RB = Q.RB * FMath::Cos(A);
					Sub.C = Q.C + Ax * (H * Rm * FMath::Sin(A));
					for (int32 S = 0; S <= NS; ++S)
					{
						const float Th = Q.Th0 + (Q.Th1 - Q.Th0) * (float)S / (float)NS;
						Pts.Add(RingPoint(Sub, Th));
						Cols.Add(C);
					}
				}
				Ref.Add(Q.C - Ax * Rm);
				Grid(Bone, Sec, Pts, Cols, NR, NS, Ref, true, true, FColor(0, 0, 0, 0));
			}

			// эллипсоид с осями AX (вперёд), AY (вбок), AZ (полярная)
			void Ellipsoid(int32 Bone, int32 Sec, const FVector& C, const FVector& AX, const FVector& AY, const FVector& AZ,
				float RX, float RY, float RZ, const FColor& Col, int32 NS = 12, int32 NR = 8)
			{
				TArray<FVector> Pts, Ref;
				TArray<FColor> Cols;
				for (int32 Rr = 0; Rr <= NR; ++Rr)
				{
					const float Ph = PI * (float)Rr / (float)NR;
					const float Sp = FMath::Sin(Ph), Cp = FMath::Cos(Ph);
					for (int32 S = 0; S <= NS; ++S)
					{
						const float Th = 2.f * PI * (float)S / (float)NS;
						Pts.Add(C + AZ * (-Cp * RZ) + AX * (FMath::Cos(Th) * Sp * RX) + AY * (FMath::Sin(Th) * Sp * RY));
						Cols.Add(Col);
					}
				}
				Ref.Add(C);
				Grid(Bone, Sec, Pts, Cols, NR, NS, Ref, true, true, FColor(0, 0, 0, 0));
			}

			void Sphere(int32 Bone, int32 Sec, const FVector& C, float Rad, const FColor& Col, int32 NS = 12, int32 NR = 8)
			{
				Ellipsoid(Bone, Sec, C, FVector(1.f, 0.f, 0.f), FVector(0.f, 1.f, 0.f), FVector(0.f, 0.f, 1.f), Rad, Rad, Rad, Col, NS, NR);
			}

			// базис трубки вдоль Dir: V — проекция Front, U = V × Dir (вбок)
			static void Frame(const FVector& Dir, const FVector& Front, FVector& U, FVector& V)
			{
				V = RejectFrom(Front, Dir).GetSafeNormal();
				if (V.SizeSquared() < 0.5f) { V = AnyPerpendicular(Dir); }
				U = FVector::CrossProduct(V, Dir).GetSafeNormal();
			}

			// Трубка от A к B: профиль радиусов Rad[i] на долях T[i] (0..1 вдоль, допускаются <0 и >1 — выход за суставы)
			void Tube(int32 Bone, int32 Sec, const FVector& A, const FVector& B, const FVector& Front, const float* T, const float* Rad,
				const FColor* Col, int32 N, int32 NS, float CapA = -1.f, float CapB = -1.f, float Flat = 1.f)
			{
				const FVector Dir = (B - A).GetSafeNormal();
				FVector U, V;
				Frame(Dir, Front, U, V);
				TArray<FRing> Rings;
				for (int32 I = 0; I < N; ++I)
				{
					FRing Q;
					Q.C = A + (B - A) * T[I]; Q.U = U; Q.V = V;
					Q.RU = Rad[I]; Q.RF = Rad[I] * Flat; Q.RB = Rad[I] * Flat;
					Q.Th0 = 0.f; Q.Th1 = 2.f * PI; Q.Col = Col[I];
					Rings.Add(Q);
				}
				Loft(Bone, Sec, Rings, NS, [](int32, float, const FColor& C) { return C; }, FColor(0, 0, 0, 0), CapA, CapB);
			}

			// ---------------- туловище ----------------
			// Сечение тела на эталонной высоте Z: полуширина, глубина вперёд и назад (слой камзола / лифа).
			void TorsoSection(float Z, float& W, float& F, float& B) const
			{
				static const float Tab[][4] = {
					{ 82.f, 13.f, 8.5f, 10.f }, { 88.f, 16.f, 9.5f, 11.5f }, { 94.f, 17.f, 10.f, 12.f }, { 100.f, 16.5f, 10.f, 11.5f },
					{ 106.f, 15.5f, 10.f, 10.5f }, { 114.f, 15.5f, 10.5f, 10.f }, { 122.f, 16.5f, 11.f, 10.5f }, { 130.f, 17.5f, 11.5f, 11.f },
					{ 138.f, 18.5f, 11.5f, 11.f }, { 144.f, 19.f, 10.5f, 10.f }, { 148.f, 17.f, 8.5f, 8.5f }, { 151.f, 12.f, 7.f, 7.f },
					{ 153.f, 7.f, 6.f, 6.f } };
				const int32 N = (int32)(sizeof(Tab) / sizeof(Tab[0]));
				int32 I = 1;
				while (I < N - 1 && Z > Tab[I][0]) { ++I; }
				const float T = FMath::Clamp((Z - Tab[I - 1][0]) / (Tab[I][0] - Tab[I - 1][0]), 0.f, 1.f);
				W = Tab[I - 1][1] + T * (Tab[I][1] - Tab[I - 1][1]);
				F = Tab[I - 1][2] + T * (Tab[I][2] - Tab[I - 1][2]);
				B = Tab[I - 1][3] + T * (Tab[I][3] - Tab[I - 1][3]);
				// полнота — живот и бока; у женщин уже талия, грудь вперёд
				const float Belly = FMath::Clamp(1.f - FMath::Abs(Z - 112.f) / 18.f, 0.f, 1.f);
				W *= 1.f + (D.Bulk - 1.f) * Belly; F *= 1.f + (D.Bulk - 1.f) * 1.6f * Belly;
				if (D.bFemale)
				{
					const float Waist = FMath::Clamp(1.f - FMath::Abs(Z - 108.f) / 10.f, 0.f, 1.f);
					const float Bust = FMath::Clamp(1.f - FMath::Abs(Z - 131.f) / 7.f, 0.f, 1.f);
					W *= 1.f - 0.12f * Waist; F *= 1.f - 0.08f * Waist; F += 2.5f * Bust;
					const float Sh = FMath::Clamp((Z - 136.f) / 10.f, 0.f, 1.f);
					W *= 1.f - 0.08f * Sh;
				}
				W *= K; F *= K; B *= K;
			}

			// точка на поверхности туловища (эталонная высота Z, угол Th от «вперёд» к «вправо», отступ Off наружу)
			FVector TorsoPoint(float Z, float Th, float Off) const
			{
				float W, F, B;
				TorsoSection(Z, W, F, B);
				const float Zr = ZR(Z);
				FRing Q;
				Q.C = FVector(SpineX(Zr), 0.f, Zr); Q.U = FVector(0.f, 1.f, 0.f); Q.V = FVector(1.f, 0.f, 0.f);
				Q.RU = W + Off * K; Q.RF = F + Off * K; Q.RB = B + Off * K;
				return RingPoint(Q, Th);
			}

			FVector TorsoCenter(float Z) const { const float Zr = ZR(Z); return FVector(SpineX(Zr), 0.f, Zr); }

			// кость туловища для полосы высот [Z0, Z1] эталона
			struct FBand { int32 Bone; float Z0, Z1; };
			static int32 TorsoBands(FBand* Out)
			{
				Out[0] = { B_Pelvis, -1000.f, 108.f };
				Out[1] = { B_Spine2, 103.f, 131.f };
				Out[2] = { B_Spine3, 126.f, 1000.f };
				return 3;
			}

			/**
			 * Полоса туловища от Z0 до Z1 (эталон), углы Range(Z, Th0, Th1) по высоте, отступ Off; режется на части по костям с нахлёстом —
			 * верхняя часть в нахлёсте на 0.35 см шире нижней (черепица: при наклоне не видно щели).
			 * ColFn(Z, Th, Base) — цвет вершины. Шаг колец ~3 см.
			 */
			template<class FRangeFn, class FColFn>
			void TorsoBand(float Z0, float Z1, const FRangeFn& Range, float Off, int32 NS, const FColor& Base, const FColFn& ColFn,
				const FColor& LiningCol, bool bCapBottom = false, int32 Sec = S_Cloth)
			{
				FBand Bands[3];
				const int32 NB = TorsoBands(Bands);
				for (int32 Bi = 0; Bi < NB; ++Bi)
				{
					const float A = FMath::Max(Z0, Bands[Bi].Z0), B = FMath::Min(Z1, Bands[Bi].Z1);
					if (B - A < 0.5f) { continue; }
					const int32 NR = FMath::Max(1, (int32)((B - A) / 3.f + 0.5f));
					TArray<FRing> Rings;
					for (int32 Rr = 0; Rr <= NR; ++Rr)
					{
						const float Z = A + (B - A) * (float)Rr / (float)NR;
						// нахлёст: эта часть заходит в нижнюю полосу соседа (Z < начала следующей части снизу)
						const bool bOverLower = Bi > 0 && Z < Bands[Bi - 1].Z1;
						float W, F, Bk;
						TorsoSection(Z, W, F, Bk);
						const float O = (Off + (bOverLower ? 0.35f : 0.f)) * K;
						FRing Q;
						Q.C = TorsoCenter(Z); Q.U = FVector(0.f, 1.f, 0.f); Q.V = FVector(1.f, 0.f, 0.f);
						Q.RU = W + O; Q.RF = F + O; Q.RB = Bk + O;
						Range(Z, Q.Th0, Q.Th1); Q.Col = Base;
						Rings.Add(Q);
					}
					const float Za = A, Zb = B;
					const int32 NRr = NR;
					Loft(Bands[Bi].Bone, Sec, Rings, NS,
						[&ColFn, Za, Zb, NRr](int32 Rr, float Th, const FColor& C) { return ColFn(Za + (Zb - Za) * (float)Rr / (float)NRr, Th, C); },
						LiningCol, (bCapBottom && Bi == 0 && A <= Z0 + 0.01f) ? 0.4f : -1.f, -1.f);
				}
			}

			// полоска по поверхности туловища (перевязь, кушак): точки (Z, Th) эталона, ширина Wd, отступ Off
			void Strap(const float* Zs, const float* Ths, int32 N, float Wd, float Off, const FColor& Col)
			{
				// каждая точка — в свою кость по высоте; соседние отрезки разных костей дублируются на стыке
				FBand Bands[3];
				const int32 NB = TorsoBands(Bands);
				for (int32 Bi = 0; Bi < NB; ++Bi)
				{
					TArray<FVector> Pts, Ref;
					TArray<FColor> Cols;
					int32 Rows = 0;
					for (int32 I = 0; I < N; ++I)
					{
						const float Mid = Bi == 0 ? -1000.f : 0.5f * (Bands[Bi - 1].Z1 + Bands[Bi].Z0);
						const float Top = Bi == NB - 1 ? 1000.f : 0.5f * (Bands[Bi].Z1 + Bands[Bi + 1].Z0);
						// отрезок I-1..I принадлежит части, если середина отрезка в её полосе
						auto InBand = [Mid, Top](float Z) { return Z >= Mid && Z < Top; };
						const bool bUse = (I > 0 && InBand(0.5f * (Zs[I - 1] + Zs[I]))) || (I + 1 < N && InBand(0.5f * (Zs[I] + Zs[I + 1])));
						if (!bUse) { continue; }
						const FVector C = TorsoPoint(Zs[I], Ths[I], Off + (Bi > 0 ? 0.35f : 0.f));
						const FVector In = TorsoCenter(Zs[I]);
						const FVector Along = (TorsoPoint(Zs[FMath::Min(I + 1, N - 1)], Ths[FMath::Min(I + 1, N - 1)], Off)
							- TorsoPoint(Zs[FMath::Max(I - 1, 0)], Ths[FMath::Max(I - 1, 0)], Off)).GetSafeNormal();
						const FVector Out = RejectFrom(C - In, Along).GetSafeNormal();
						const FVector Side = FVector::CrossProduct(Along, Out).GetSafeNormal();
						Pts.Add(C - Side * (0.5f * Wd * K)); Pts.Add(C + Side * (0.5f * Wd * K));
						Cols.Add(Col); Cols.Add(Col);
						Ref.Add(In);
						++Rows;
					}
					if (Rows >= 2) { Grid(Bands[Bi].Bone, S_Cloth, Pts, Cols, Rows - 1, 1, Ref, false, false, FColor(0, 0, 0, 0)); }
				}
			}

			// маленькая пуговица / заклёпка на туловище
			void TorsoButton(float Z, float Th, float Off, float Rad, const FColor& Col)
			{
				FBand Bands[3];
				TorsoBands(Bands);
				const int32 Bone = Z < 105.5f ? B_Pelvis : Z < 128.5f ? B_Spine2 : B_Spine3;
				Sphere(Bone, S_Smooth, TorsoPoint(Z, Th, Off), Rad * K, Col, 6, 4);
			}

			// ---------------- части наряда ----------------
			void BuildTorso()
			{
				const FDesc& d = D;
				const FColor Under = d.bDress ? d.DressCol : d.bWaistcoat ? d.WaistcoatCol : d.Shirt;
				const FColor LegC = d.LegsCol, StrC = d.Stripe;
				const bool bDress = d.bDress;
				const bool bStr = d.Legs == Legs_Trousers && d.bStripes;
				const float Waist = d.Legs == Legs_Trousers ? 106.f : 100.f;   // длинные штаны — с высокой талией
				// нижний слой: камзол / рубаха / лиф; ниже талии — штаны (у платья — юбка закрывает)
				TorsoBand(82.f, 153.f, [](float, float& A, float& B) { A = 0.f; B = 2.f * PI; }, 0.f, 24, Under,
					[Under, LegC, StrC, bDress, bStr, Waist](float Z, float Th, const FColor&) {
						if (bDress || Z >= Waist) { return Under; }
						const int32 Col = (int32)(Th / (2.f * PI) * 24.f + 0.5f);
						return (bStr && (Col % 2) == 0) ? StrC : LegC;
					},
					FColor(0, 0, 0, 0), true);
				if (!bDress && d.bWaistcoat && d.Coat != Coat_Short)
				{
					// полы камзола спереди до бёдер (у короткого кафтана героя камзол кончается на талии)
					TArray<FRing> Rings;
					for (int32 I = 0; I <= 3; ++I)
					{
						const float Z = 101.f - 2.4f * I;
						float W, F, B;
						TorsoSection(Z, W, F, B);
						FRing Q; Q.C = TorsoCenter(Z); Q.U = FVector(0.f, 1.f, 0.f); Q.V = FVector(1.f, 0.f, 0.f);
						Q.RU = W + 0.7f * K; Q.RF = F + 0.7f * K; Q.RB = B + 0.7f * K;
						const float Half = Deg(80.f - 9.f * I);
						Q.Th0 = -Half; Q.Th1 = Half; Q.Col = d.WaistcoatCol;
						Rings.Add(Q);
					}
					Loft(B_Pelvis, S_Cloth, Rings, 12, [](int32, float, const FColor& C) { return C; }, Mul(d.WaistcoatCol, 0.7f));
					for (int32 I = 0; I < 7; ++I) { TorsoButton(103.f + 5.5f * I, 0.f, 0.45f, 0.65f, d.Metal); }
				}
				if (!bDress)
				{
					// жабо и шейный платок
					Ellipsoid(B_Spine3, S_Cloth, TorsoPoint(146.5f, 0.f, 0.2f), FVector(1.f, 0.f, 0.f), FVector(0.f, 1.f, 0.f),
						FVector(0.f, 0.f, 1.f), 0.9f * K, 2.f * K, 3.6f * K, d.Shirt, 8, 6);
				}
			}

			// кафтан / мундир / куртка: верх на туловище + полы на тазе
			void BuildCoat()
			{
				const FDesc& d = D;
				if (d.Coat == Coat_None) { return; }
				const bool bJacket = d.Coat == Coat_Jacket;
				const float Off = 1.1f;
				// разрез спереди: до груди почти сомкнут, ниже расходится
				auto Open = [bJacket](float Z) {
					if (bJacket) { return Deg(Z > 132.f ? 14.f : 14.f + (132.f - Z) * 0.9f); }
					return Deg(Z > 128.f ? 5.f : 5.f + (128.f - Z) * 1.6f);
				};
				const FColor Coat = d.CoatCol, Face = d.Facing;
				const bool bLapels = d.Coat == Coat_Uniform;
				const float ZTop = 151.f, ZWaist = bJacket ? 94.f : 104.f;
				// верх: разрез плавно расходится книзу (угловой диапазон — свой у каждого кольца)
				TorsoBand(ZWaist, ZTop, [&Open](float Z, float& A, float& B) { A = Open(Z); B = 2.f * PI - A; }, Off, 22, Coat,
					[Coat, Face, bLapels, &Open](float Zz, float Th, const FColor&) {
						// отвороты мундира: красная полоса у края разреза на груди
						const float Half = Open(Zz);
						const float Edge = FMath::Min(Th - Half, 2.f * PI - Half - Th);
						return (bLapels && Zz > 112.f && Edge < Deg(26.f)) ? Face : Coat;
					}, FColor(0, 0, 0, 0));
				// пуговицы по краю разреза
				for (int32 I = 0; I < 7; ++I)
				{
					const float Z = 116.f + 5.f * I;
					if (Z > 148.f) { break; }
					const float Th = Open(Z) + Deg(bLapels ? 20.f : 6.f);
					TorsoButton(Z, Th, Off + 0.3f, 0.75f, d.Metal);
					TorsoButton(Z, -Th, Off + 0.3f, 0.75f, d.Metal);
				}
				// стоячий воротник
				{
					const FVector N0 = R.RestPos[B_Neck];
					const float T[3] = { 0.f, 0.5f, 1.f };
					const float Rad[3] = { 7.6f * K, 7.4f * K, 7.2f * K };
					const FColor Cc[3] = { Face, Face, Face };
					Tube(B_Spine3, S_Cloth, FVector(N0.X - 1.f * K, 0.f, ZR(148.f)), FVector(N0.X - 1.5f * K, 0.f, ZR(155.f)), FVector(1.f, 0.f, 0.f),
						T, Rad, Cc, 3, 16, -1.f, -1.f, 0.9f);
				}
				// полы: от талии до подола, открыты спереди, двусторонние (изнанка — подкладка)
				const float Hem = d.Coat == Coat_Habit ? 55.f : d.Coat == Coat_Uniform ? 60.f : d.Coat == Coat_Short ? 72.f : 0.f;
				if (Hem > 0.f)
				{
					const int32 NR = 7;
					TArray<FRing> Rings;
					for (int32 Rr = 0; Rr <= NR; ++Rr)
					{
						const float T = (float)Rr / (float)NR;
						const float Z = ZWaist + 1.f + (Hem - ZWaist - 1.f) * T;
						float W, F, B;
						TorsoSection(FMath::Max(Z, 92.f), W, F, B);
						const float Flare = K * (Z < 92.f ? (92.f - Z) * 0.16f : 0.f) + K * 1.6f * T;
						FRing Q;
						Q.C = TorsoCenter(FMath::Max(Z, 82.f)); Q.C.Z = ZR(Z);
						Q.U = FVector(0.f, 1.f, 0.f); Q.V = FVector(1.f, 0.f, 0.f);
						Q.RU = W + (Off + 0.4f) * K + Flare; Q.RF = F + (Off + 0.4f) * K + Flare * 0.8f; Q.RB = B + (Off + 0.6f) * K + Flare * 1.25f;
						// разрез продолжает разрез верха и расходится к подолу
						const float H0 = Open(ZWaist + 1.f);
						const float Half = H0 + (Deg(82.f) - H0) * FMath::Pow(T, 0.7f);
						Q.Th0 = Half; Q.Th1 = 2.f * PI - Half; Q.Col = Coat;
						Rings.Add(Q);
					}
					Loft(B_Pelvis, S_Cloth, Rings, 22, [](int32, float, const FColor& C) { return C; }, d.Lining);
					// клапаны карманов
					for (int32 S = 0; S < 2; ++S)
					{
						const float Th = S == 0 ? Deg(-62.f) : Deg(62.f);
						const float Z = 97.f;
						float W, F, B;
						TorsoSection(Z, W, F, B);
						const FVector P = TorsoPoint(Z, Th, Off + 1.6f);
						const FVector Out = (P - TorsoCenter(Z)).GetSafeNormal();
						const FVector Up(0.f, 0.f, 1.f);
						const FVector Side = FVector::CrossProduct(Up, Out).GetSafeNormal();
						Ellipsoid(B_Pelvis, S_Cloth, P, Out, Side, Up, 0.7f * K, 7.f * K, 2.4f * K, Mul(Coat, 0.85f), 8, 4);
					}
				}
				if (bJacket)
				{
					// низ куртки — короткий край до бёдер (закрытая полоса сзади)
					TorsoBand(90.f, ZWaist + 0.5f, [](float, float& A, float& B) { A = Deg(30.f); B = 2.f * PI - Deg(30.f); }, Off, 20, Coat,
						[](float, float, const FColor& C) { return C; }, Mul(Coat, 0.7f));
				}
			}

			void BuildDress()
			{
				const FDesc& d = D;
				if (!d.bDress) { return; }
				// юбка: от талии до пола, на кринолине-подушках шире в бёдрах
				const float Tz[] = { 106.f, 100.f, 92.f, 80.f, 60.f, 40.f, 20.f, 4.f };
				const float Ws[] = { 0.f, 3.5f, 7.f, 10.f, 13.f, 16.f, 19.f, 21.f };
				const int32 N = (int32)(sizeof(Tz) / sizeof(Tz[0]));
				TArray<FRing> Rings;
				for (int32 I = 0; I < N; ++I)
				{
					float W, F, B;
					TorsoSection(FMath::Max(Tz[I], 92.f), W, F, B);
					FRing Q;
					Q.C = TorsoCenter(FMath::Max(Tz[I], 82.f)); Q.C.Z = ZR(Tz[I]);
					Q.C.X -= 2.f * K * (1.f - Tz[I] / 106.f);   // сзади юбка пышнее
					Q.U = FVector(0.f, 1.f, 0.f); Q.V = FVector(1.f, 0.f, 0.f);
					Q.RU = W + 0.6f * K + Ws[I] * K; Q.RF = F + 0.6f * K + Ws[I] * K * 1.05f; Q.RB = B + 0.6f * K + Ws[I] * K * 1.25f;
					Q.Th0 = 0.f; Q.Th1 = 2.f * PI; Q.Col = d.DressCol;
					Rings.Add(Q);
				}
				const bool bStr = d.bStripes;
				const FColor Dc = d.DressCol, Sc = d.Stripe;
				Loft(B_Pelvis, S_Cloth, Rings, 36,
					[bStr, Dc, Sc](int32, float Th, const FColor&) {
						const int32 K2 = (int32)(Th / (2.f * PI) * 36.f + 0.5f);   // вертикальные полосы по столбцам сетки
						return (bStr && (K2 % 3) == 0) ? Sc : Dc;
					}, Mul(Dc, 0.6f));
				// кайма у подола
				{
					TArray<FRing> Hem;
					for (int32 I = 0; I < 2; ++I)
					{
						FRing Q = Rings[N - 1];
						Q.C.Z = ZR(4.f + 5.f * I);
						const float Kq = 1.f - 0.03f * I;
						Q.RU = Rings[N - 1].RU * Kq + 0.3f * K; Q.RF = Rings[N - 1].RF * Kq + 0.3f * K; Q.RB = Rings[N - 1].RB * Kq + 0.3f * K;
						Q.Col = Mul(Dc, 0.7f);
						Hem.Add(Q);
					}
					Loft(B_Pelvis, S_Cloth, Hem, 36, [](int32, float, const FColor& C) { return C; }, FColor(0, 0, 0, 0));
				}
				if (d.bApron)
				{
					TArray<FRing> Ap;
					for (int32 I = 0; I < N - 1; ++I)
					{
						FRing Q = Rings[I];
						Q.RU += 0.7f * K; Q.RF += 0.7f * K; Q.RB += 0.7f * K;
						Q.Th0 = -Deg(62.f); Q.Th1 = Deg(62.f); Q.Col = d.ApronCol;
						if (Tz[I] < 25.f) { break; }
						Ap.Add(Q);
					}
					Loft(B_Pelvis, S_Cloth, Ap, 14, [](int32, float, const FColor& C) { return C; }, Mul(d.ApronCol, 0.75f));
					// завязки фартука — полоса по талии
					const float Zs[] = { 104.5f, 104.5f, 104.5f, 104.5f, 104.5f, 104.5f, 104.5f, 104.5f, 104.5f };
					float Ths[9];
					for (int32 I = 0; I < 9; ++I) { Ths[I] = Deg(-180.f + 45.f * I); }
					Strap(Zs, Ths, 9, 2.6f, 0.9f, d.ApronCol);
				}
				// лиф: шнуровка спереди тёмной полосой
				{
					const float Zs[] = { 104.f, 109.f, 114.f, 119.f, 124.f };
					const float Ths[] = { 0.f, 0.f, 0.f, 0.f, 0.f };
					Strap(Zs, Ths, 5, 2.f, 0.3f, d.WaistcoatCol);
				}
			}

			void BuildFichu()
			{
				const FDesc& d = D;
				if (!d.bFichu) { return; }
				// косынка: накинута на плечи, спереди концы сходятся мысом
				const int32 NS = 28, NR = 4;
				TArray<FVector> Pts, Ref;
				TArray<FColor> Cols;
				for (int32 Rr = 0; Rr <= NR; ++Rr)
				{
					const float T = (float)Rr / (float)NR;
					for (int32 S = 0; S <= NS; ++S)
					{
						const float Th = -PI + 2.f * PI * (float)S / (float)NS;
						const float Fr = FMath::Pow(FMath::Max(FMath::Cos(Th), 0.f), 3.f);
						const float Bk = FMath::Pow(FMath::Max(-FMath::Cos(Th), 0.f), 3.f);
						const float ZLo = 137.f - 9.f * Fr - 6.f * Bk;
						const float Z = ZLo + (151.5f - ZLo) * T;
						Pts.Add(TorsoPoint(Z, Th, 0.7f + 0.3f * (1.f - T)));
						Cols.Add(d.Shirt);
					}
					Ref.Add(TorsoCenter(137.f + 14.f * T));
				}
				Grid(B_Spine3, S_Cloth, Pts, Cols, NR, NS, Ref, false, true, Mul(d.Shirt, 0.8f));
			}

			void BuildBelts()
			{
				const FDesc& d = D;
				if (d.bCrossBelts)
				{
					// перевязи: с левого плеча к правому бедру и с правого к левому, спереди и сзади
					const int32 N = 9;
					for (int32 S = 0; S < 2; ++S)
					{
						const float Sg = S == 0 ? 1.f : -1.f;
						float Zs[N], ThF[N], ThB[N];
						for (int32 I = 0; I < N; ++I)
						{
							const float T = (float)I / (float)(N - 1);
							Zs[I] = 151.f - T * 55.f;
							ThF[I] = Sg * Deg(-72.f + T * 165.f);
							ThB[I] = Sg * Deg(-108.f - T * 165.f);
						}
						Strap(Zs, ThF, N, 4.2f, 1.6f + 0.25f * S, d.WaistcoatCol);
						Strap(Zs, ThB, N, 4.2f, 1.6f + 0.25f * S, d.WaistcoatCol);
					}
					// патронная сумка сзади на правом бедре
					const FVector P = TorsoPoint(97.f, Deg(150.f), 3.4f);
					Ellipsoid(B_Pelvis, S_Smooth, P, FVector(1.f, 0.f, 0.f), FVector(0.f, 1.f, 0.f), FVector(0.f, 0.f, 1.f), 3.f * K, 9.f * K, 6.f * K, FColor(24, 20, 18), 10, 6);
				}
				if (d.bSash)
				{
					const int32 N = 13;
					float Zs[N], Ths[N];
					for (int32 I = 0; I < N; ++I) { Zs[I] = 105.f; Ths[I] = Deg(-180.f + 30.f * I); }
					Strap(Zs, Ths, N, 7.f, 1.25f, d.SashCol);
				}
			}

			void BuildSword()
			{
				const FDesc& d = D;
				if (!d.bSword) { return; }
				// ножны у левого бедра, рукоять впереди
				const FVector Top = TorsoPoint(99.f, Deg(-100.f), 3.4f);
				const FVector Dir = FVector(-0.78f, -0.12f, -0.62f).GetSafeNormal();
				const FVector End = Top + Dir * (70.f * K);
				const float T[3] = { 0.f, 0.9f, 1.f };
				const float Rd[3] = { 1.3f * K, 1.1f * K, 0.9f * K };
				const FColor Cs[3] = { FColor(30, 24, 20), FColor(30, 24, 20), d.Metal };
				Tube(B_Pelvis, S_Smooth, Top, End, FVector(0.f, -1.f, 0.f), T, Rd, Cs, 3, 6, 0.5f, 0.5f, 0.6f);
				const FVector Grip0 = Top - Dir * (1.f * K);
				const FVector Grip1 = Top - Dir * (14.f * K);
				const float T2[2] = { 0.f, 1.f };
				const float R2[2] = { 0.9f * K, 0.8f * K };
				const FColor C2[2] = { FColor(40, 30, 24), FColor(40, 30, 24) };
				Tube(B_Pelvis, S_Smooth, Grip0, Grip1, FVector(0.f, -1.f, 0.f), T2, R2, C2, 2, 6, -1.f, 0.6f);
				Sphere(B_Pelvis, S_Smooth, Grip1 - Dir * (0.8f * K), 1.4f * K, d.Metal, 8, 5);   // навершие
				Ellipsoid(B_Pelvis, S_Smooth, Top - Dir * (0.5f * K), Dir, FVector(0.f, 1.f, 0.f), FVector::CrossProduct(Dir, FVector(0.f, 1.f, 0.f)).GetSafeNormal(),
					0.6f * K, 3.2f * K, 2.4f * K, d.Metal, 10, 5);   // гарда-чашка
			}

			// руки: шар плеча, рукав, локоть, предплечье, обшлаг, манжета, кисть
			void BuildArms()
			{
				const FDesc& d = D;
				const bool bCoat = d.Coat != Coat_None;
				const bool bShortSleeve = d.bDress;   // у платья рукав до локтя с оборкой
				const FColor Sleeve = d.bDress ? d.DressCol : bCoat ? d.CoatCol : d.Shirt;
				const float Big = bCoat ? 1.f : 0.92f;
				for (int32 S = 0; S < 2; ++S)
				{
					const int32 BU = UpperArmBone(S), BL = LowerArmBone(S), BH = HandBone(S);
					const FVector Sh = R.RestPos[BU], El = R.RestPos[BL], Wr = R.RestPos[BH];
					const FVector Fwd(1.f, 0.f, 0.f);
					// плечо
					Sphere(BU, S_Cloth, Sh, 7.f * K * Big, Sleeve, 12, 8);
					{
						const float T[4] = { 0.f, 0.4f, 0.85f, 1.02f };
						const float Rd[4] = { 7.f * K * Big, 6.4f * K * Big, 5.8f * K * Big, 5.7f * K * Big };
						const FColor C[4] = { Sleeve, Sleeve, Sleeve, Sleeve };
						Tube(BU, S_Cloth, Sh, El, Fwd, T, Rd, C, 4, 12);
					}
					// локоть и предплечье
					if (bShortSleeve)
					{
						// рукав до локтя с оборкой, дальше рука
						Sphere(BL, S_Cloth, El, 5.6f * K, Sleeve, 12, 8);
						const float T[3] = { 0.f, 0.12f, 0.2f };
						const float Rd[3] = { 5.6f * K, 6.6f * K, 7.4f * K };
						const FColor C[3] = { d.Shirt, d.Shirt, d.Shirt };
						Tube(BL, S_Cloth, El, Wr, Fwd, T, Rd, C, 3, 12);
						const float T2[4] = { 0.1f, 0.5f, 0.92f, 1.02f };
						const float R2[4] = { 4.4f * K, 4.f * K, 3.3f * K, 3.2f * K };
						const FColor C2[4] = { d.Skin, d.Skin, d.Skin, d.Skin };
						Tube(BL, S_Smooth, El, Wr, Fwd, T2, R2, C2, 4, 10);
					}
					else
					{
						Sphere(BL, S_Cloth, El, 5.7f * K * Big, Sleeve, 12, 8);
						const float T[3] = { 0.f, 0.5f, 0.98f };
						const float Rd[3] = { 5.5f * K * Big, 5.f * K * Big, 4.5f * K * Big };
						const FColor C[3] = { Sleeve, Sleeve, Sleeve };
						Tube(BL, S_Cloth, El, Wr, Fwd, T, Rd, C, 3, 12);
						if (bCoat)
						{
							// широкий обшлаг
							const float Tc[4] = { 0.56f, 0.6f, 0.93f, 0.95f };
							const float Rc[4] = { 5.4f * K, 6.3f * K, 6.6f * K, 5.4f * K };
							const FColor Cc[4] = { d.Facing, d.Facing, d.Facing, Mul(d.Facing, 0.6f) };
							Tube(BL, S_Cloth, El, Wr, Fwd, Tc, Rc, Cc, 4, 12);
							for (int32 I = 0; I < 2; ++I)
							{
								const FVector P = El + (Wr - El) * (0.68f + 0.12f * I);
								FVector U, V;
								Frame((Wr - El).GetSafeNormal(), Fwd, U, V);
								Sphere(BL, S_Smooth, P - V * (6.6f * K), 0.6f * K, d.Metal, 6, 4);
							}
						}
						// манжета рубахи с оборкой
						const float Tm[3] = { 0.92f, 1.0f, 1.06f };
						const float Rm[3] = { 4.4f * K, 5.4f * K, 5.9f * K };
						const FColor Cm[3] = { d.Shirt, d.Shirt, Mul(d.Shirt, 0.92f) };
						Tube(BL, S_Cloth, El, Wr, Fwd, Tm, Rm, Cm, 3, 12);
					}
					BuildHand(S);
				}
			}

			void BuildHand(int32 S)
			{
				const FDesc& d = D;
				const int32 BL = LowerArmBone(S), BH = HandBone(S);
				const FVector Wr = R.RestPos[BH];
				const FVector Dir = (Wr - R.RestPos[BL]).GetSafeNormal();
				FVector Side, Thick;
				Frame(Dir, FVector(1.f, 0.f, 0.f), Thick, Side);   // Side — к «вперёд» (большой палец), Thick — толщина ладони
				Sphere(BH, S_Smooth, Wr, 3.4f * K, d.Skin, 10, 6);   // запястье
				Ellipsoid(BH, S_Smooth, Wr + Dir * (5.f * K), Side, Thick, Dir, 4.3f * K, 2.2f * K, 5.4f * K, d.Skin, 12, 7);   // ладонь
				Ellipsoid(BH, S_Smooth, Wr + Dir * (11.f * K) - Thick * (0.6f * K), Side, Thick, Dir, 3.9f * K, 1.8f * K, 4.4f * K, Mul(d.Skin, 0.97f), 12, 7);   // пальцы
				const FVector TDir = (Dir + Side * 0.9f).GetSafeNormal();
				Ellipsoid(BH, S_Smooth, Wr + Dir * (4.f * K) + Side * (3.6f * K) + TDir * (2.f * K), Side, Thick, TDir, 1.4f * K, 1.4f * K, 3.2f * K, d.Skin, 8, 5);   // большой палец
			}

			// ноги: бедро, колено, голень, стопа
			void BuildLegs()
			{
				const FDesc& d = D;
				for (int32 S = 0; S < 2; ++S)
				{
					const int32 BT = ThighBone(S), BC = CalfBone(S), BF = FootBone(S);
					const FVector Hp = R.RestPos[BT], Kn = R.RestPos[BC], An = R.RestPos[BF];
					const FVector Fwd(1.f, 0.f, 0.f);
					const bool bTrousers = d.Legs == Legs_Trousers;
					const FColor Lc = d.LegsCol;
					if (!d.bDress)
					{
						Sphere(BT, S_Cloth, Hp, 8.6f * K, Lc, 12, 8);
						const float T[5] = { 0.f, 0.3f, 0.75f, 0.94f, 1.06f };
						const float Rd[5] = { 8.8f * K, 8.3f * K, 7.f * K, bTrousers ? 6.8f * K : 6.4f * K, bTrousers ? 6.8f * K : 6.4f * K };
						FColor C[5] = { Lc, Lc, Lc, Lc, Lc };
						if (!bTrousers) { C[3] = Mul(Lc, 0.75f); C[4] = Mul(Lc, 0.75f); }   // подвязка под коленом
						if (bTrousers && d.bStripes) { Striped(BT, Hp, Kn, T, Rd, 5, Lc, d.Stripe); }
						else { Tube(BT, S_Cloth, Hp, Kn, Fwd, T, Rd, C, 5, 14); }
						if (!bTrousers) { Sphere(BT, S_Smooth, Kn + (Kn - Hp).GetSafeNormal() * (-2.5f * K) + FVector(0.f, SideSign(S) * 6.2f * K, 0.f), 0.7f * K, d.Metal, 6, 4); }
					}
					// голень
					const FColor Calf = bTrousers ? Lc : d.Stockings;
					Sphere(BC, S_Cloth, Kn, (bTrousers ? 6.7f : 5.8f) * K, Calf, 12, 8);
					if (bTrousers && d.bStripes)
					{
						const float T[4] = { 0.f, 0.4f, 0.9f, 1.f };
						const float Rd[4] = { 6.7f * K, 6.4f * K, 5.8f * K, 5.7f * K };
						Striped(BC, Kn, An, T, Rd, 4, Lc, d.Stripe);
					}
					else if (d.Feet == Feet_Boots)
					{
						const float T[5] = { -0.12f, -0.04f, 0.3f, 0.85f, 1.05f };
						const float Rd[5] = { 7.6f * K, 7.3f * K, 6.6f * K, 5.4f * K, 5.4f * K };
						const FColor C[5] = { Mul(d.ShoesCol, 1.3f), Mul(d.ShoesCol, 1.3f), d.ShoesCol, d.ShoesCol, d.ShoesCol };
						Tube(BC, S_Smooth, Kn, An, Fwd, T, Rd, C, 5, 14, 0.15f);
					}
					else
					{
						const float T[4] = { 0.f, 0.32f, 0.85f, 1.02f };
						const float Rd[4] = { 5.5f * K, 6.1f * K, 4.4f * K, 4.2f * K };
						const FColor C[4] = { Calf, Calf, Calf, Calf };
						Tube(BC, S_Cloth, Kn, An, Fwd, T, Rd, C, 4, 12);
						if (d.Feet == Feet_Gaiters)
						{
							// гетры до колена на пуговицах
							const float Tg[4] = { 0.06f, 0.1f, 0.85f, 1.04f };
							const float Rg[4] = { 6.f * K, 6.5f * K, 5.f * K, 4.9f * K };
							const FColor Cg[4] = { Mul(d.Stockings, 0.85f), d.Stockings, d.Stockings, d.Stockings };
							Tube(BC, S_Cloth, Kn, An, Fwd, Tg, Rg, Cg, 4, 12);
							for (int32 I = 0; I < 4; ++I)
							{
								const FVector P = Kn + (An - Kn) * (0.25f + 0.17f * I) + FVector(0.f, SideSign(S) * 1.f, 0.f) * (5.6f * K - 0.9f * I * K);
								Sphere(BC, S_Smooth, P, 0.55f * K, FColor(30, 28, 26), 6, 4);
							}
						}
					}
					BuildFoot(S);
				}
			}

			// полосатая трубка: вертикальные полосы по столбцам сетки
			void Striped(int32 Bone, const FVector& A, const FVector& B, const float* T, const float* Rd, int32 N, const FColor& C0, const FColor& C1)
			{
				const FVector Dir = (B - A).GetSafeNormal();
				FVector U, V;
				Frame(Dir, FVector(1.f, 0.f, 0.f), U, V);
				TArray<FRing> Rings;
				for (int32 I = 0; I < N; ++I)
				{
					FRing Q; Q.C = A + (B - A) * T[I]; Q.U = U; Q.V = V; Q.RU = Rd[I]; Q.RF = Rd[I]; Q.RB = Rd[I];
					Q.Th0 = 0.f; Q.Th1 = 2.f * PI; Q.Col = C0;
					Rings.Add(Q);
				}
				Loft(Bone, S_Cloth, Rings, 18, [C0, C1](int32, float Th, const FColor&) {
					const int32 Col = (int32)(Th / (2.f * PI) * 18.f + 0.5f);
					return (Col % 2) == 0 ? C1 : C0;
				}, FColor(0, 0, 0, 0));
			}

			void BuildFoot(int32 S)
			{
				const FDesc& d = D;
				const int32 BF = FootBone(S), BB = BallBone(S);
				const FVector An = R.RestPos[BF], Ba = R.RestPos[BB];
				FVector Dir = Ba - An; Dir.Z = 0.f; Dir = Dir.GetSafeNormal();
				const FVector Side = FVector::CrossProduct(FVector(0.f, 0.f, 1.f), Dir).GetSafeNormal();
				const FVector Heel = FVector(An.X, An.Y, 0.f) - Dir * (7.f * K);
				const float Len = FVector::DotProduct(FVector(Ba.X, Ba.Y, 0.f) - Heel, Dir) + 8.f * K;
				// сечения: доля длины, полуширина, высота верха
				const float Tt[] = { 0.f, 0.06f, 0.2f, 0.38f, 0.6f, 0.82f, 0.95f, 1.f };
				const float Wt[] = { 2.6f, 3.6f, 4.2f, 4.4f, 4.6f, 4.3f, 3.3f, 1.6f };
				const float Ht[] = { 5.f, 7.f, 9.5f, 9.f, 5.6f, 3.8f, 2.8f, 1.8f };
				const int32 N = (int32)(sizeof(Tt) / sizeof(Tt[0]));
				const bool bBoot = d.Feet == Feet_Boots;
				TArray<FRing> Rings;
				for (int32 I = 0; I < N; ++I)
				{
					FRing Q;
					const float H = Ht[I] * K * (bBoot && Tt[I] < 0.45f ? 1.25f : 1.f);
					Q.C = Heel + Dir * (Len * Tt[I]) + FVector(0.f, 0.f, 0.5f * H);
					Q.U = Side; Q.V = FVector(0.f, 0.f, 1.f);
					Q.RU = Wt[I] * K; Q.RF = 0.5f * H; Q.RB = 0.5f * H;
					Q.Th0 = 0.f; Q.Th1 = 2.f * PI; Q.Col = d.ShoesCol;
					Rings.Add(Q);
				}
				Loft(BF, S_Smooth, Rings, 12, [](int32, float, const FColor& C) { return C; }, FColor(0, 0, 0, 0), 0.6f, 0.5f);
				// лодыжка — шар в суставе (закрывает стык с голенью)
				Sphere(BF, S_Cloth, An, 4.3f * K, bBoot ? d.ShoesCol : (d.Legs == Legs_Trousers ? d.LegsCol : d.Stockings), 10, 6);
				// пряжка
				if (!bBoot)
				{
					const FVector P = Heel + Dir * (Len * 0.62f) + FVector(0.f, 0.f, 5.3f * K);
					Ellipsoid(BF, S_Smooth, P, Dir, Side, FVector(0.f, 0.f, 1.f), 1.4f * K, 2.2f * K, 0.6f * K, d.Metal, 8, 4);
				}
			}

			// шея и голова (лицо — простые формы; подробные лица — этап 12)
			void BuildHead()
			{
				const FDesc& d = D;
				const FVector Nk = R.RestPos[B_Neck], Hd = R.RestPos[B_Head];
				{
					const float T[3] = { -0.25f, 0.5f, 1.3f };
					const float Rd[3] = { 5.6f * K, 5.2f * K, 5.f * K };
					const FColor C[3] = { d.Skin, d.Skin, d.Skin };
					Tube(B_Neck, S_Smooth, Nk, Hd, FVector(1.f, 0.f, 0.f), T, Rd, C, 3, 12);
				}
				if (!d.bDress)
				{
					// шейный платок
					const float T[3] = { -0.1f, 0.3f, 0.62f };
					const float Rd[3] = { 6.3f * K, 6.1f * K, 6.f * K };
					const FColor C[3] = { d.Shirt, d.Shirt, d.Shirt };
					Tube(B_Neck, S_Cloth, Nk, Hd, FVector(1.f, 0.f, 0.f), T, Rd, C, 3, 12);
				}
				const FVector X(1.f, 0.f, 0.f), Y(0.f, 1.f, 0.f), Z(0.f, 0.f, 1.f);
				auto At = [&Hd, this](float Dx, float Dy, float Dz) { return Hd + FVector(Dx, Dy, Dz) * K; };
				Ellipsoid(B_Head, S_Smooth, At(1.5f, 0.f, 9.f), X, Y, Z, 10.f * K, 7.6f * K, 11.3f * K, d.Skin, 16, 10);       // череп
				const float Jw = d.bFemale ? 0.86f : 1.f;
				Ellipsoid(B_Head, S_Smooth, At(5.f, 0.f, d.bFemale ? 4.2f : 3.5f), X, Y, Z, 5.6f * K * Jw, 5.6f * K * Jw, 5.f * K * Jw, d.Skin, 12, 7);   // челюсть
				Ellipsoid(B_Head, S_Smooth, At(11.2f, 0.f, 8.6f), X, Y, Z, 1.6f * K, 1.25f * K, 2.4f * K, Mul(d.Skin, 0.96f), 8, 5);   // нос
				for (int32 S = 0; S < 2; ++S)
				{
					const float Sg = SideSign(S);
					Ellipsoid(B_Head, S_Smooth, At(0.5f, Sg * 7.7f, 8.f), X, Y, Z, 2.f * K, 1.f * K, 3.f * K, Mul(d.Skin, 0.93f), 8, 5);      // ухо
					Ellipsoid(B_Head, S_Smooth, At(9.4f, Sg * 3.2f, 10.6f), X, Y, Z, 0.7f * K, 1.1f * K, 0.75f * K, FColor(40, 34, 30), 8, 5);   // глаз
					Ellipsoid(B_Head, S_Smooth, At(9.7f, Sg * 3.3f, 12.6f), X, Y, Z, 0.6f * K, 1.6f * K, 0.45f * K, Mul(d.HairCol, 0.8f), 8, 4); // бровь
				}
				Ellipsoid(B_Head, S_Smooth, At(10.f, 0.f, 4.6f), X, Y, Z, 0.6f * K, 2.f * K, 0.5f * K, Mul(d.Skin, 0.7f), 8, 4);   // рот
				// волосы: шапка сдвинута назад — лицо открыто
				if (d.Hat != Hat_Cap)
				{
					Ellipsoid(B_Head, S_Smooth, At(-0.6f, 0.f, 10.8f), X, Y, Z, 10.2f * K, 8.1f * K, 11.4f * K, d.HairCol, 16, 10);
				}
				if (d.Hair == Hair_Queue && !d.bFemale)
				{
					const float T[3] = { 0.f, 0.5f, 1.f };
					const float Rd[3] = { 1.6f * K, 1.4f * K, 1.1f * K };
					const FColor C[3] = { d.HairCol, d.HairCol, d.HairCol };
					Tube(B_Head, S_Smooth, At(-9.5f, 0.f, 6.f), At(-11.5f, 0.f, -12.f), X, T, Rd, C, 3, 8, 0.5f, 0.6f);
					Ellipsoid(B_Head, S_Cloth, At(-10.4f, 0.f, 2.f), X, Y, Z, 1.f * K, 3.4f * K, 1.6f * K, FColor(26, 24, 26), 8, 4);   // бант
				}
				else if (d.Hair == Hair_Bun && d.Hat != Hat_Cap)
				{
					Ellipsoid(B_Head, S_Smooth, At(-8.f, 0.f, 9.f), X, Y, Z, 3.f * K, 4.f * K, 3.6f * K, d.HairCol, 10, 6);
				}
				// головной убор
				if (d.Hat == Hat_Tricorne) { BuildTricorne(); }
				else if (d.Hat == Hat_Cap)
				{
					// чепец: пышная шапочка и оборка вокруг лица
					Ellipsoid(B_Head, S_Cloth, At(-1.f, 0.f, 12.f), X, Y, Z, 10.8f * K, 9.f * K, 10.f * K, d.HatCol, 16, 9);
					// лента по околышу (цвет платья)
					TArray<FRing> Rings;
					for (int32 I = 0; I < 2; ++I)
					{
						const float Dz = -3.f + 2.2f * I;
						const float Sc = FMath::Sqrt(FMath::Max(1.f - (Dz / 10.f) * (Dz / 10.f), 0.f));
						FRing Q;
						Q.C = At(-1.f, 0.f, 12.f + Dz); Q.U = Y; Q.V = X;
						Q.RU = 9.f * Sc * K + 0.35f * K; Q.RF = 10.8f * Sc * K + 0.35f * K; Q.RB = Q.RF;
						Q.Th0 = 0.f; Q.Th1 = 2.f * PI; Q.Col = d.DressCol;
						Rings.Add(Q);
					}
					Loft(B_Head, S_Cloth, Rings, 24, [](int32, float, const FColor& C) { return C; }, FColor(0, 0, 0, 0));
				}
				else if (d.Hat == Hat_Bonnet)
				{
					// колпак: мягкий, верх свешивается назад
					const float T[4] = { 0.f, 0.35f, 0.75f, 1.f };
					const float Rd[4] = { 9.4f * K, 8.6f * K, 5.6f * K, 3.f * K };
					const FColor C[4] = { Mul(d.HatCol, 0.85f), d.HatCol, d.HatCol, d.HatCol };
					Tube(B_Head, S_Cloth, At(-0.5f, 0.f, 14.5f), At(-8.f, 0.f, 26.f), X, T, Rd, C, 4, 14, -1.f, 0.8f, 1.05f);
				}
			}

			void BuildTricorne()
			{
				const FDesc& d = D;
				const FVector Hd = R.RestPos[B_Head];
				const FVector Base = Hd + FVector(-0.5f, 0.f, 17.5f) * K;
				// тулья
				{
					TArray<FRing> Rings;
					for (int32 I = 0; I < 3; ++I)
					{
						FRing Q;
						Q.C = Base + FVector(0.f, 0.f, (float)I * 3.5f * K); Q.U = FVector(0.f, 1.f, 0.f); Q.V = FVector(1.f, 0.f, 0.f);
						Q.RU = (9.4f - 0.4f * I) * K; Q.RF = (10.8f - 0.5f * I) * K; Q.RB = (10.8f - 0.5f * I) * K;
						Q.Th0 = 0.f; Q.Th1 = 2.f * PI; Q.Col = d.HatCol;
						Rings.Add(Q);
					}
					Loft(B_Head, S_Cloth, Rings, 20, [](int32, float, const FColor& C) { return C; }, FColor(0, 0, 0, 0), -1.f, 0.35f);
				}
				// поля: загнуты вверх с трёх сторон, углы — спереди и сзади по бокам
				const int32 NS = 36, NR = 3;
				TArray<FVector> Pts, Ref;
				TArray<FColor> Cols;
				for (int32 Rr = 0; Rr <= NR; ++Rr)
				{
					for (int32 S = 0; S <= NS; ++S)
					{
						const float Th = 2.f * PI * (float)S / (float)NS;
						// «скруглённый треугольник»: расстояние до края больше у углов (0°, ±120°)
						const float Corner = FMath::Pow(FMath::Max(FMath::Cos(1.5f * Th), 0.f), 6.f);
						const float REdge = (14.5f + 4.5f * Corner) * K;
						const float HEdge = (7.5f - 3.5f * Corner) * K;
						const float RIn = (FMath::Abs(FMath::Cos(Th)) * 10.8f + FMath::Abs(FMath::Sin(Th)) * 9.4f) * K;
						float Rad, H;
						if (Rr == 0) { Rad = RIn; H = 0.f; }
						else if (Rr == 1) { Rad = RIn + 0.6f * (REdge - RIn); H = 0.4f * K; }
						else if (Rr == 2) { Rad = REdge; H = 0.35f * HEdge; }
						else { Rad = REdge - 1.2f * K; H = HEdge; }
						Pts.Add(Base + FVector(FMath::Cos(Th) * Rad, FMath::Sin(Th) * Rad, H));
						Cols.Add(Rr == NR ? d.HatTrim : d.HatCol);
					}
					Ref.Add(Base + FVector(0.f, 0.f, -40.f * K));   // снизу: «наружу» — вверх и в стороны
				}
				Grid(B_Head, S_Cloth, Pts, Cols, NR, NS, Ref, false, true, Mul(d.HatCol, 0.8f));
				// кокарда слева
				Ellipsoid(B_Head, S_Cloth, Base + FVector(5.f, -10.5f, 5.f) * K, FVector(0.4f, -0.9f, 0.f).GetSafeNormal(), FVector(0.9f, 0.4f, 0.f).GetSafeNormal(),
					FVector(0.f, 0.f, 1.f), 0.5f * K, 2.2f * K, 2.2f * K, d.bCrossBelts ? FColor(236, 236, 236) : Mul(d.HatCol, 1.6f), 10, 4);
			}
		};
	}

	// Построить одежду на скелете Rig (поза покоя). Части — по костям; пустые части не строятся.
	// bMergeSections = false — не сливать мелкие детали в основную секцию (для тестов).
	inline void Build(const TPAnim::FTPRig& Rig, const FDesc& D, FMesh& Out, bool bMergeSections = true)
	{
		for (int32 B = 0; B < TPAnim::B_Count; ++B) { Out.Pieces[B] = FPiece(); }
		Detail::FBuilder Bd(Rig, D, Out);
		Bd.BuildTorso();
		Bd.BuildCoat();
		Bd.BuildDress();
		Bd.BuildFichu();
		Bd.BuildBelts();
		Bd.BuildSword();
		Bd.BuildArms();
		Bd.BuildLegs();
		Bd.BuildHead();
		// мелкие детали другой секции (пуговицы, пряжки, платок на шее) переносим в основную: меньше вызовов отрисовки
		for (int32 B = 0; B < TPAnim::B_Count; ++B)
		{
			FPiece& P = Out.Pieces[B];
			const int32 NC = P.Idx[S_Cloth].Num(), NSm = P.Idx[S_Smooth].Num();
			if (!bMergeSections || NC == 0 || NSm == 0) { continue; }
			const int32 Minor = NC < NSm ? (int32)S_Cloth : (int32)S_Smooth;
			const int32 Major = 1 - Minor;
			if (P.Idx[Minor].Num() * 100 > P.Idx[Major].Num() * 30) { continue; }   // заметная доля — оставить отдельной секцией
			for (int32 I = 0; I < P.Idx[Minor].Num(); ++I) { P.Idx[Major].Add(P.Idx[Minor][I]); }
			P.Idx[Minor].Reset();
		}
	}
}

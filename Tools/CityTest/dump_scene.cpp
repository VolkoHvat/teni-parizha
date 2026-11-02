// Тени Парижа — сцена для превью фасадов: ряд домов разного стиля вдоль улицы (и угловой дом), выгрузка геометрии
// построителя квартала в двоичный файл для рендера Lutèce (render_city.c). Координаты Lutèce: метры, Y вверх.
// Улица — со стороны −Z (фасады домов — сторона 0), угловой дом смотрит ещё и на +X.
// Запуск: dump_scene out.bin [row|level]
#include "TPCityElements.h"
#include <cstdio>
#include <cstring>

using namespace TPCity;

static float H01(uint32 H) { return (float)(H & 0xFFFFFFu) / 16777216.f; }
static uint32 HMix(uint32 H) { H ^= H >> 16; H *= 0x85EBCA6Bu; H ^= H >> 13; H *= 0xC2B2AE35u; H ^= H >> 16; return H; }

/** подобрать Seed с нужным стилем (те же хеши, что в FBuilder::Facade) */
static uint32 FindSeed(int Rust, int Arch, int Seg, int Pil, uint32 Start)
{
	for (uint32 S = Start; S < Start + 200000u; ++S)
	{
		const bool bRust = H01(HMix(S + 1u)) < 0.6f, bArch = bRust && H01(HMix(S + 2u)) < 0.6f;
		const bool bSeg = H01(HMix(S + 3u)) < 0.45f, bPil = H01(HMix(S + 4u)) < 0.3f;
		if ((Rust < 0 || bRust == (Rust != 0)) && (Arch < 0 || bArch == (Arch != 0)) && (Seg < 0 || bSeg == (Seg != 0)) && (Pil < 0 || bPil == (Pil != 0))) return S;
	}
	return Start;
}

static void Write(FILE* F, const TArray<FMeshBuf>& Bufs, int32 NumMat)
{
	for (int32 I = 0; I < Bufs.Num(); ++I)
	{
		const FMeshBuf& M = Bufs[I];
		if (M.IsEmpty()) continue;
		const int32 Hdr[3] = { I % NumMat, M.Pos.Num(), M.Idx.Num() };
		fwrite(Hdr, sizeof(int32), 3, F);
		for (int32 V = 0; V < M.Pos.Num(); ++V)
		{
			const float D[8] = { M.Pos[V].X, M.Pos[V].Y, M.Pos[V].Z, M.Nrm[V].X, M.Nrm[V].Y, M.Nrm[V].Z, M.UV[V].X, M.UV[V].Y };
			fwrite(D, sizeof(float), 8, F);
			const uint8 C[4] = { M.Col[V].R, M.Col[V].G, M.Col[V].B, M.Col[V].A };
			fwrite(C, 1, 4, F);
		}
		fwrite(M.Idx.GetData(), sizeof(int32), M.Idx.Num(), F);
	}
}

int main(int argc, char** argv)
{
	if (argc < 2) { printf("dump_scene out.bin [row|level]\n"); return 1; }
	const bool bLevel = argc > 2 && !strcmp(argv[2], "level");
	FBuilder B;
	if (bLevel)
	{
		B.BuildLevel(1789);
	}
	else
	{
		B.Init(-20.f, -30.f, 90.f, 30.f, 24.f);
		B.Ground(CM_Cobble, -20.f, -30.f, 90.f, 30.f, 0.f, 1.f, true);
		B.Ground(CM_Flag, -2.f, -2.f, 63.f, 12.f, 0.12f, 1.f, true);
		const FColor Tints[5] = { FColor(250, 238, 215), FColor(255, 250, 240), FColor(238, 228, 210), FColor(232, 225, 214), FColor(245, 232, 205) };
		const FColor Shut[5] = { FColor(0, 0, 0, 0), FColor(60, 80, 70), FColor(0, 0, 0, 0), FColor(90, 50, 40), FColor(70, 90, 110) };
		const float XS[6] = { 0.f, 12.f, 25.f, 36.5f, 49.f, 61.f };
		const int32 Wall[5] = { CM_Stone, CM_Plaster, CM_Stone, CM_Brick, CM_Plaster };
		const int32 Floors[5] = { 5, 4, 5, 4, 5 };
		const int32 Roofs[5] = { Roof_Mansard, Roof_Gable, Roof_Mansard, Roof_Flat, Roof_Mansard };
		const int32 Shops[5] = { 0, 1, 0, 1, 1 };
		const int32 Balc[5] = { 1, 0, 1, 0, 1 };
		// стили: руст+арки и лучковые окна; гладкий с плоскими; пилястры; угловой руст и лучковые; руст+арки (лавки в арках)
		const uint32 Seeds[5] = { FindSeed(1, 1, 1, 0, 100u), FindSeed(0, 0, 0, 0, 300u), FindSeed(1, 0, 0, 1, 500u), FindSeed(0, 0, 1, 0, 700u), FindSeed(1, 1, 0, 0, 900u) };
		for (int32 I = 0; I < 5; ++I)
		{
			FBuilding H;
			H.X0 = XS[I]; H.X1 = XS[I + 1]; H.Z0 = 0.f; H.Z1 = 10.f; H.BaseY = 0.12f;
			H.Floors = Floors[I]; H.FloorH = 3.1f; H.GroundH = 3.6f;
			H.WallMat = Wall[I]; H.Tint = Tints[I]; H.ShutterCol = Shut[I]; H.TrimCol = FColor(235, 230, 220);
			H.Roof = Roofs[I];
			H.StreetSides = I == 4 ? 3 : 1;
			H.Shops = Shops[I];
			H.BalconyFloor = Balc[I];
			H.Seed = Seeds[I];
			B.Building(H);
			printf("дом %d: seed %u\n", I, Seeds[I]);
		}
		B.Lamp(FV3(6.f, 0.12f, -1.2f)); B.Lamp(FV3(30.f, 0.12f, -1.2f)); B.Lamp(FV3(54.f, 0.12f, -1.2f));
	}
	FILE* F = fopen(argv[1], "wb");
	if (!F) return 2;
	Write(F, B.Bufs, CM_Num);
	Write(F, B.SoftBufs, CM_Num);
	fclose(F);
	printf("треугольников: %d\n", B.CountTriangles());
	return 0;
}

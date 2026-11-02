// Тени Парижа — текстуры квартала из TPTexGen (как в игре, с выветриванием) для превью: двоичный файл для render_city
// (альбедо sRGB, нормали, ORM по материалам; Flat — пропускаются) и лист образцов PPM (альбедо | нормали | ORM).
// Запуск: dump_tex tex.bin sheet.ppm [размер]
#include "TPCityMaterials.h"
#include <cstdio>
#include <cstdlib>

int main(int argc, char** argv)
{
	if (argc < 3) { printf("dump_tex tex.bin sheet.ppm [размер]\n"); return 1; }
	const int32 N = argc > 3 ? atoi(argv[3]) : 512;
	FILE* F = fopen(argv[1], "wb");
	if (!F) return 2;
	// лист: по строке на материал с текстурой, 3 образца по S×S (уменьшенные мип-уровнем)
	const int32 S = 128;
	TArray<TArray<uint8>> Rows;
	int32 Bad = 0;
	for (int32 I = 0; I < TPCity::CM_Num; ++I)
	{
		const TPCityMat::FMatDesc& D = TPCityMat::Get(I);
		if (D.Tex == TPTexGen::EType::Flat) continue;
		TPTexGen::FSet Set;
		TPTexGen::Generate(Set, D.Tex, N, TPCityMat::Palette(D), TPCityMat::TexSeed(I), D.Rough, D.Metal, true);
		const int32 Hdr[2] = { I, Set.N };
		fwrite(Hdr, sizeof(int32), 2, F);
		const TArray<FColor>* Maps[3] = { &Set.Albedo[0], &Set.Normal[0], &Set.ORM[0] };
		for (int32 K = 0; K < 3; ++K)
			for (const FColor& C : *Maps[K]) { const uint8 B[4] = { C.R, C.G, C.B, C.A }; fwrite(B, 1, 4, F); }
		// проверка: нормали единичные, ORM в пределах, нет «пустых» текстур
		for (const FColor& C : Set.Normal[0])
		{
			const float X = C.R / 127.5f - 1.f, Y = C.G / 127.5f - 1.f, Z = C.B / 127.5f - 1.f;
			const float L = X * X + Y * Y + Z * Z;
			if (L < 0.9f || L > 1.1f || Z < 0.f) { ++Bad; break; }
		}
		// мип-уровень S×S
		int32 Lv = 0; for (int32 Q = Set.N; Q > S; Q >>= 1) ++Lv;
		TArray<uint8> Row; Row.SetNum(S * S * 3 * 3);
		for (int32 K = 0; K < 3; ++K)
		{
			const TArray<FColor>& M = K == 0 ? Set.Albedo[Lv] : (K == 1 ? Set.Normal[Lv] : Set.ORM[Lv]);
			for (int32 Y = 0; Y < S; ++Y) for (int32 X = 0; X < S; ++X)
			{
				const FColor C = M[Y * S + X];
				uint8* P = &Row[(Y * S * 3 + K * S + X) * 3];
				P[0] = C.R; P[1] = C.G; P[2] = C.B;
			}
		}
		Rows.Add(Row);
		printf("материал %2d: %d×%d, средний цвет %.3f %.3f %.3f\n", I, Set.N, Set.N, Set.Average.R, Set.Average.G, Set.Average.B);
	}
	fclose(F);
	// лист: две колонки материалов
	const int32 Cols = 2, NR = (Rows.Num() + 1) / 2, W = S * 3 * Cols + 8, H = NR * (S + 4);
	FILE* P = fopen(argv[2], "wb");
	if (!P) return 3;
	fprintf(P, "P6\n%d %d\n255\n", W, H);
	TArray<uint8> Img; Img.SetNum(W * H * 3);
	for (int32 I = 0; I < Img.Num(); ++I) Img[I] = 40;
	for (int32 R = 0; R < Rows.Num(); ++R)
	{
		const int32 OX = (R % Cols) * (S * 3 + 8), OY = (R / Cols) * (S + 4);
		for (int32 Y = 0; Y < S; ++Y)
			for (int32 X = 0; X < S * 3; ++X)
				for (int32 C = 0; C < 3; ++C) Img[((OY + Y) * W + OX + X) * 3 + C] = Rows[R][(Y * S * 3 + X) * 3 + C];
	}
	fwrite(Img.GetData(), 1, Img.Num(), P);
	fclose(P);
	printf("ошибок нормалей: %d\n", Bad);
	return Bad ? 4 : 0;
}

// Тени Парижа — процедурные бесшовные PBR-текстуры (порт legacy/lutece/gfx/lu_texgen.c).
// Чистые вычисления без UObject: результат — массивы FColor (B8G8R8A8) для альбедо (sRGB), нормалей (DirectX, как в UE) и ORM.
// Владелец модуля: агент «город».
#pragma once

#include "CoreMinimal.h"

namespace TPTexGen
{
	/** вид материала */
	enum class EType : uint8
	{
		StoneBlocks, Plaster, Cobble, Slate, RoofTiles, Wood, Brick, Flagstone, Dirt, Cloth, Metal, Shutter,
		Leaves,   // листва: ячейки-листья, дыры в альфе (для маскированного материала)
		Flat      // ровный цвет (стекло, краска, вода)
	};

	/** линейный цвет */
	struct FRGB
	{
		float R = 0.f, G = 0.f, B = 0.f;
		FRGB() {}
		FRGB(float InR, float InG, float InB) : R(InR), G(InG), B(InB) {}
	};

	/** палитра: два линейных цвета и разброс яркости */
	struct FPalette
	{
		FRGB C0, C1;
		float Var = 0.f;
	};

	/** набор текстур одного материала; Mips[0] — полный размер N×N, далее до 1×1 */
	struct FSet
	{
		int32 N = 0;
		TArray<TArray<FColor>> Albedo;   // sRGB, альфа — маска (листва)
		TArray<TArray<FColor>> Normal;   // R = X (+U), G = Y (+V, DirectX), B = Z
		TArray<TArray<FColor>> ORM;      // R = затенение, G = шероховатость, B = металл
		FRGB Average;                    // средний линейный цвет альбедо (для запасного материала)
	};

	/**
	 * Сгенерировать набор. N — степень двойки (для Flat достаточно 4).
	 * FlatRough/FlatMetal используются для Flat; для металла металличность = 1.
	 * bMips — построить цепочку мип-уровней.
	 */
	void Generate(FSet& Out, EType Type, int32 N, const FPalette& Pal, uint32 Seed, float FlatRough = 0.8f, float FlatMetal = 0.f, bool bMips = true);

	/** бесшовный шум с периодом P (в клетках), примерно −1..1 */
	float Noise(float X, float Y, int32 P, uint32 Seed);
	float Fbm(float X, float Y, int32 P, int32 Octaves, uint32 Seed);
	/** ячейки Вороного: расстояние до ближайшего центра (F1), до второго (F2), хеш ячейки */
	void Voronoi(float X, float Y, int32 P, uint32 Seed, float& F1, float& F2, uint32& Id);
}

// Тени Парижа — таблица материалов квартала (порт lu_city_materials): вид процедурной текстуры, палитра, шероховатость,
// металл, повтор UV, вид материала Unreal, сила выветривания и тёмная полоса у земли. Чистые данные без UObject —
// общие для ATPCityGenerator и проверок/превью (Tools/CityTest). Владелец модуля: агент «город».
#pragma once

#include "CoreMinimal.h"
#include "TPTexGen.h"
#include "TPCityBuilder.h"

namespace TPCityMat
{
	/** вид материала Unreal для материала квартала */
	enum class EMatKind : uint8 { Opaque, Masked, Emissive };

	/** описание материала: линейные цвета палитры, разброс, шероховатость, металл, повтор UV */
	struct FMatDesc
	{
		TPTexGen::EType Tex;
		float C0[3];
		float C1[3];
		float Var;
		float Rough;
		float Metal;
		float UVScale;
		EMatKind Kind;
		float Weather;     // выветривание текстуры 0..1 (подтёки, лишайник, лужи, ржавчина)
		int32 GroundGrime; // 1 — стены: в материале темнеют у земли (брызги и сырость, параметр GroundGrime)
	};

	/** порядок — как TPCity::EMat */
	inline const FMatDesc& Get(int32 I)
	{
		static const FMatDesc Descs[TPCity::CM_Num] = {
		{ TPTexGen::EType::StoneBlocks, { .62f, .56f, .45f }, { .74f, .68f, .56f }, .12f, .80f, 0.f, 1.f, EMatKind::Opaque, 0.8f, 1 }, // камень (известняк)
		{ TPTexGen::EType::Plaster,     { .70f, .66f, .58f }, { .80f, .76f, .68f }, .10f, .90f, 0.f, 1.f, EMatKind::Opaque, 0.8f, 1 }, // штукатурка
		{ TPTexGen::EType::Brick,       { .42f, .17f, .10f }, { .55f, .25f, .15f }, .15f, .85f, 0.f, 1.f, EMatKind::Opaque, 0.6f, 1 }, // кирпич
		{ TPTexGen::EType::Slate,       { .13f, .15f, .19f }, { .20f, .22f, .27f }, .20f, .50f, 0.f, 1.f, EMatKind::Opaque, 0.4f, 0 }, // шифер
		{ TPTexGen::EType::RoofTiles,   { .45f, .17f, .08f }, { .60f, .27f, .13f }, .20f, .70f, 0.f, 1.f, EMatKind::Opaque, 0.5f, 0 }, // черепица
		{ TPTexGen::EType::Wood,        { .35f, .23f, .14f }, { .50f, .34f, .20f }, .15f, .75f, 0.f, 1.f, EMatKind::Opaque, 0.6f, 0 }, // дерево
		{ TPTexGen::EType::Shutter,     { .80f, .80f, .80f }, { .90f, .90f, .90f }, .05f, .70f, 0.f, 1.f, EMatKind::Opaque, 0.3f, 0 }, // ставни (цвет в вершинах)
		{ TPTexGen::EType::Metal,       { .05f, .05f, .06f }, { .08f, .08f, .09f }, .10f, .45f, 1.f, 1.f, EMatKind::Opaque, 0.5f, 0 }, // кованое железо
		{ TPTexGen::EType::Flat,        { .90f, .90f, .90f }, { .90f, .90f, .90f }, 0.f,  .08f, .2f, 1.f, EMatKind::Opaque, 0.0f, 0 }, // стекло
		{ TPTexGen::EType::Cobble,      { .30f, .29f, .27f }, { .45f, .43f, .40f }, .20f, .80f, 0.f, .5f, EMatKind::Opaque, 0.8f, 0 }, // булыжник
		{ TPTexGen::EType::Flagstone,   { .50f, .47f, .42f }, { .62f, .58f, .52f }, .10f, .80f, 0.f, .35f, EMatKind::Opaque, 0.6f, 0 }, // плиты
		{ TPTexGen::EType::Cloth,       { .90f, .90f, .90f }, { 1.f,  1.f,  1.f  }, .05f, .95f, 0.f, 2.f, EMatKind::Masked, 0.0f, 0 }, // ткань (двусторонняя)
		{ TPTexGen::EType::Flat,        { 1.f,  1.f,  1.f  }, { 1.f,  1.f,  1.f  }, 0.f,  .60f, 0.f, 1.f, EMatKind::Opaque, 0.0f, 0 }, // крашеное дерево (переплёты)
		{ TPTexGen::EType::Dirt,        { .60f, .55f, .45f }, { .75f, .70f, .55f }, .20f, .95f, 0.f, 1.f, EMatKind::Opaque, 0.0f, 0 }, // земля / сено
		{ TPTexGen::EType::Leaves,      { .75f, .80f, .70f }, { 1.f,  1.f,  1.f  }, .15f, .70f, 0.f, 1.f, EMatKind::Masked, 0.0f, 0 }, // листва (цвет в вершинах)
		{ TPTexGen::EType::Wood,        { .25f, .20f, .16f }, { .35f, .28f, .22f }, .10f, .90f, 0.f, 2.f, EMatKind::Opaque, 0.0f, 0 }, // кора
		{ TPTexGen::EType::Flat,        { 1.f,  .80f, .50f }, { 1.f,  .80f, .50f }, 0.f,  .30f, 0.f, 1.f, EMatKind::Emissive, 0.0f, 0 }, // стекло фонаря
		{ TPTexGen::EType::Flat,        { .12f, .20f, .22f }, { .12f, .20f, .22f }, 0.f,  .05f, 0.f, 1.f, EMatKind::Opaque, 0.0f, 0 }, // вода
		};
		return Descs[FMath::Clamp(I, 0, (int32)TPCity::CM_Num - 1)];
	}

	/** палитра для TPTexGen::Generate */
	inline TPTexGen::FPalette Palette(const FMatDesc& D)
	{
		TPTexGen::FPalette P;
		P.C0 = TPTexGen::FRGB(D.C0[0], D.C0[1], D.C0[2]);
		P.C1 = TPTexGen::FRGB(D.C1[0], D.C1[1], D.C1[2]);
		P.Var = D.Var;
		P.Weather = D.Weather;
		return P;
	}

	/** зерно генератора текстуры материала (как в оригинале) */
	inline uint32 TexSeed(int32 I) { return 1000u + (uint32)I * 17u; }
}

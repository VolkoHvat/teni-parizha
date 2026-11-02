// Тени Парижа — библиотека архитектурных элементов квартала 1789 года: профили обломов (карнизы, тяги, подоконники,
// наличники, базы и капители пилястр, цоколь), выдавливание профиля по пути с усовой (под углом) стыковкой,
// проёмы с плоской, лучковой и полуциркульной перемычкой, стена вокруг проёма, откосы, наличники и архивольты,
// замковые камни, ленточный и угловой руст, пилястры, переплёты (рама, импост, горбыльки, веер в арке),
// ставни, сухарики, кронштейны балконов.
// Чистая геометрия без UObject: всё пишется в FMeshBuf в системе Lutèce (метры, Y вверх), детали фасада — в рамке
// FFacadeFrame (u — вдоль фасада, v — вверх, d — наружу). Лицевые стороны ориентируются по ожидаемой нормали, поэтому
// функции не зависят от того, «левая» рамка или «правая». Этап 9 запечёт эти же функции в модули UStaticMesh/HISM.
// Владелец модуля: агент «город».
#pragma once

#include "CoreMinimal.h"
#include "TPCityBuilder.h"

namespace TPCity
{
	/**
	 * Профиль облома: точки (o, h), обход против часовой стрелки вокруг тела (тело слева при движении по точкам).
	 * o — вынос от направляющей (от стены или от края проёма), h — высота (для горизонтальных обломов) или глубина
	 * наружу от стены (для наличников). Выдавливаются только отрезки между соседними точками; замыкающий отрезок
	 * (последняя → первая точка) — это прилегание к стене, он нужен только для торцов.
	 */
	struct FProfile
	{
		const FV2* Pts = nullptr;
		int32 Num = 0;
	};

	enum class EProfile : uint8
	{
		CrownCornice,     // венчающий карниз: полка, сухариковый пояс, слезник, гусёк (0.45 × 0.45 м)
		StringCourse,     // межэтажная тяга с отливом (0.20 × 0.20 м)
		Sill,             // подоконник с отливом (0.10 × 0.12 м)
		Architrave,       // наличник (ширина 0.16, глубина 0.06 м) — o в плоскости стены, h наружу
		PilasterBase,     // база пилястры: плинт и вал (0.16 × 0.06 м)
		PilasterCapital,  // капитель пилястры: выкружка и абака (0.20 × 0.06 м)
		Plinth,           // цоколь с откосом (0.55 × 0.08 м)
		Sash,             // рама переплёта 6 × 6 см внутрь проёма (o < 0), h — к улице от плоскости стекла
		Console,          // кронштейн балкона в разрезе (вынос 0.75, высота 0.5 м)
		Num
	};

	FProfile GetProfile(EProfile P);

	/** плоскость выдавливания: путь — в осях (AxX, AxY) от Origin; вынос o — влево от направления пути, h — вдоль AxH */
	struct FSweepBasis
	{
		FV3 Origin, AxX, AxY, AxH;
		FSweepBasis() {}
		FSweepBasis(const FV3& InO, const FV3& InX, const FV3& InY, const FV3& InH) : Origin(InO), AxX(InX), AxY(InY), AxH(InH) {}
	};

	struct FSweepOpts
	{
		float ScaleO = 1.f, ScaleH = 1.f;   // масштаб профиля
		float OffsetH = 0.f;                // сдвиг профиля вдоль h
		float UVS = 0.5f;                   // плотность UV (на метр)
		bool bClosed = false;               // замкнутый путь (без торцов)
		bool bCapStart = false, bCapEnd = false;
		bool bPlanarUV = false;             // UV по (длина пути, h) — швы текстуры совпадают с геометрией руста
		FColor Col = FColor(255, 255, 255, 255);
	};

	/** выдавить профиль по пути (усовые стыки в изломах, плоские нормали, торцы — триангуляция многоугольника профиля) */
	void SweepProfile(FMeshBuf& M, const FSweepBasis& B, const FV2* Path, int32 NPath, const FV2* Prof, int32 NProf, const FSweepOpts& O);
	inline void SweepProfile(FMeshBuf& M, const FSweepBasis& B, const TArray<FV2>& Path, const FProfile& P, const FSweepOpts& O)
	{
		SweepProfile(M, B, Path.GetData(), Path.Num(), P.Pts, P.Num, O);
	}

	/** триангуляция простого многоугольника (против часовой стрелки) отсечением ушей; индексы — в Out тройками */
	void TriangulatePolygon(const FV2* P, int32 N, TArray<int32>& Out);

	/** четырёхугольник с заданной ожидаемой нормалью: обход разворачивается, если нужно (A-B-C-D по контуру) */
	void OrientedQuad(FMeshBuf& M, const FV3& A, const FV3& B, const FV3& C, const FV3& D, FV2 UA, FV2 UB, FV2 UC, FV2 UD, const FV3& Expected);

	/** коробка в рамке фасада; Faces — маска граней: 1 перед (+d), 2 зад, 4 верх, 8 низ, 16 левый (−u), 32 правый (+u) */
	enum EBoxFace : int32 { BF_Front = 1, BF_Back = 2, BF_Top = 4, BF_Bottom = 8, BF_Left = 16, BF_Right = 32, BF_NoBack = 61, BF_All = 63 };
	void FacadeBox(FMeshBuf& M, const FFacadeFrame& F, float U0, float U1, float V0, float V1, float D0, float D1, float UVS, int32 Faces = BF_NoBack);

	/** проём с перемычкой: плоской (Rise = 0), лучковой (Rise < W/2) или полуциркульной (Rise = W/2) */
	struct FOpening
	{
		float UC = 0.f;     // середина по u
		float V0 = 0.f;     // низ
		float W = 1.f;      // ширина
		float H = 2.f;      // высота до пят арки (для плоской перемычки — вся высота)
		float Rise = 0.f;   // стрела подъёма арки

		float U0() const { return UC - W * 0.5f; }
		float U1() const { return UC + W * 0.5f; }
		float Spring() const { return V0 + H; }
		float Top() const { return V0 + H + Rise; }
		bool IsArched() const { return Rise > 1e-3f; }
		/** радиус и центр окружности арки (центр — на оси проёма) */
		float ArcRadius() const;
		float ArcCenterV() const;
		/** верх проёма над точкой U (для U вне проёма — пята) */
		float TopAt(float U) const;
		/** число отрезков дуги */
		int32 ArcSegments() const;
		/** точки дуги слева направо (включая пяты) */
		void ArcPoints(TArray<FV2>& Out) const;
		/** замкнутый контур по часовой стрелке (вид снаружи): низ-лево → вверх → по дуге → вниз; снаружи — слева */
		void Outline(TArray<FV2>& Out) const;
		/** контур без нижней стороны (для дверей): от низа слева до низа справа */
		void OutlineOpen(TArray<FV2>& Out) const;
	};

	// ---- элементы фасада (рамка F: u, v, d) ----

	/** горизонтальный облом вдоль фасада от U0 до U1 на высоте V (торцы по желанию) */
	void Moulding(FMeshBuf& M, const FFacadeFrame& F, float U0, float U1, float V, EProfile P, float Scale, bool bCaps, FColor Col, float UVS = 0.5f);
	/** облом по замкнутому контуру здания в плане (X0..X1, Z0..Z1) на высоте Y — с усовыми углами */
	void MouldingLoop(FMeshBuf& M, float X0, float Z0, float X1, float Z1, float Y, EProfile P, float Scale, FColor Col, float UVS = 0.5f);
	/** стена прямоугольника [WU0, WU1] × [WV0, WV1] за вычетом проёма (WV1 ≥ верха проёма) */
	void WallAroundOpening(FMeshBuf& M, const FFacadeFrame& F, const FOpening& O, float WU0, float WU1, float WV0, float WV1, float UVS);
	/** откосы проёма: от плоскости стены вглубь на Depth (смотрят внутрь проёма) */
	void Reveal(FMeshBuf& M, const FFacadeFrame& F, const FOpening& O, float Depth, float UVS, bool bBottom);
	/** заполнение проёма (стекло, дверь) на глубине D (D < 0 — в нише), смотрит наружу */
	void OpeningFill(FMeshBuf& M, const FFacadeFrame& F, const FOpening& O, float D, float UVS);
	/** наличник/архивольт по контуру проёма; bBottom — и снизу (окно), иначе только бока и верх (дверь, арка) */
	void Surround(FMeshBuf& M, const FFacadeFrame& F, const FOpening& O, float Scale, bool bBottom, FColor Col, float UVS = 0.5f);
	/** замковый камень над серединой проёма (трапеция, расширяется кверху) */
	void Keystone(FMeshBuf& M, const FFacadeFrame& F, const FOpening& O, float Height, float Proj, FColor Col, float UVS = 0.5f);
	/**
	 * переплёт в плоскости стекла D: рама по контуру, средник (две створки), горбыльки (Rows рядов стёкол на створку,
	 * Cols — стёкол в ряду створки), импост на пяте арки или фрамуга у высоких окон, веер в арке. Дверь (bDoor) — только рама.
	 */
	void Sash(FMeshBuf& M, const FFacadeFrame& F, const FOpening& O, float D, int32 Rows, int32 Cols, bool bDoor, FColor Col);
	/** шаг ряда текстуры камня (TPTexGen StoneBlocks: 4 ряда на повтор при плотности 0.45/м) — руст кладётся по нему */
	constexpr float StoneCourse = 1.f / (4.f * 0.45f);
	/** ленточный руст: камни высотой Course от уровня Base, вынос Proj, фаска Chamfer; торцы по желанию */
	void Rustication(FMeshBuf& M, const FFacadeFrame& F, float U0, float U1, float V0, float V1, float Base, float Course, float Proj, float Chamfer, bool bCapL, bool bCapR, float UVS);
	/** руст вокруг проёма в прямоугольнике стены (пазухи арки — полосами по дуге) */
	void RusticatedWall(FMeshBuf& M, const FFacadeFrame& F, const FOpening* O, float WU0, float WU1, float WV0, float WV1, float Base, float Course, float UVS);
	/** пилястра: база, ствол, капитель; ширина W, вынос Depth */
	void Pilaster(FMeshBuf& M, const FFacadeFrame& F, float UC, float W, float Depth, float V0, float V1, FColor Col, float UVS = 0.5f);
	/** ставня в собственной рамке SF (u — от петли, v — вверх, d — толщина ±T); bDetail — обвязка и средняя перекладина */
	void Shutter(FMeshBuf& M, const FFacadeFrame& SF, float W, float H, float T, bool bDetail);
	/** сухарики под карнизом: зубцы шириной W с шагом Step */
	void Dentils(FMeshBuf& M, const FFacadeFrame& F, float U0, float U1, float V0, float V1, float Proj, float W, float Step, float UVS = 0.5f);
	/** кронштейн балкона (профиль EProfile::Console шириной W, верх на VTop) */
	void Console(FMeshBuf& M, const FFacadeFrame& F, float UC, float W, float VTop, float Scale, FColor Col, float UVS = 0.5f);

	/** проверки для тестов: число точек профиля и признак «h не убывает» (нужен для торцов горизонтальных обломов) */
	bool ProfileIsCCW(const FProfile& P);
}

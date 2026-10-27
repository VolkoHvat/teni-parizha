// Тени Парижа — построитель геометрии квартала (порт legacy/lutece/world/lu_city.c и build_level из samples/sandbox.c).
// Работает в системе координат Lutèce: метры, ось Y вверх, правая тройка. Перевод в Unreal (см, Z вверх, левая тройка)
// делается одним местом при выгрузке в UProceduralMeshComponent (ATPCityGenerator): (x, y, z) → (x·100, z·100, y·100).
// Перестановка осей — отражение, оно же переводит правую тройку в левую, поэтому порядок индексов сохраняется:
// треугольник, лицевой (против часовой стрелки снаружи) в Lutèce, остаётся лицевым в Unreal; нормали переводятся той же
// перестановкой. Внутри построителя лицевая сторона — та, куда смотрит Cross(B − A, C − A) в правой тройке Lutèce.
// Владелец модуля: агент «город».
#pragma once

#include "CoreMinimal.h"

namespace TPCity
{
	/** материалы квартала (как LU_CM_* в оригинале) */
	enum EMat : int32
	{
		CM_Stone, CM_Plaster, CM_Brick, CM_Slate, CM_Tiles, CM_Wood, CM_Shutter, CM_Iron,
		CM_Glass, CM_Cobble, CM_Flag, CM_Cloth, CM_Trim, CM_Dirt, CM_Leaves, CM_Bark, CM_Lamp, CM_Water, CM_Num
	};
	enum ERoof : int32 { Roof_Mansard, Roof_Gable, Roof_Flat };

	struct FV2
	{
		float X = 0.f, Y = 0.f;
		FV2() {}
		FV2(float InX, float InY) : X(InX), Y(InY) {}
	};

	struct FV3
	{
		float X = 0.f, Y = 0.f, Z = 0.f;
		FV3() {}
		FV3(float InX, float InY, float InZ) : X(InX), Y(InY), Z(InZ) {}
		FV3 operator+(const FV3& B) const { return FV3(X + B.X, Y + B.Y, Z + B.Z); }
		FV3 operator-(const FV3& B) const { return FV3(X - B.X, Y - B.Y, Z - B.Z); }
		FV3 operator*(float S) const { return FV3(X * S, Y * S, Z * S); }
		FV3 operator-() const { return FV3(-X, -Y, -Z); }
	};

	/** буфер вершин одной секции (ячейка × материал) */
	struct FMeshBuf
	{
		TArray<FV3> Pos;
		TArray<FV3> Nrm;
		TArray<FV2> UV;
		TArray<FColor> Col;
		TArray<int32> Idx;
		FColor Color = FColor(255, 255, 255, 255);   // текущий цвет вершин

		int32 Vertex(const FV3& P, const FV3& N, const FV2& T);
		void Tri(int32 A, int32 B, int32 C) { Idx.Add(A); Idx.Add(B); Idx.Add(C); }
		int32 NumVerts() const { return Pos.Num(); }
		bool IsEmpty() const { return Idx.Num() == 0; }
	};

	/** параметры дома */
	struct FBuilding
	{
		float X0 = 0.f, Z0 = 0.f, X1 = 0.f, Z1 = 0.f;   // след на земле
		float BaseY = 0.f;                               // уровень земли
		int32 Floors = 3;
		float FloorH = 3.1f, GroundH = 3.6f;
		int32 WallMat = CM_Stone;                        // CM_Stone / CM_Plaster / CM_Brick
		FColor Tint = FColor(255, 255, 255, 255);
		FColor ShutterCol = FColor(0, 0, 0, 0);          // альфа 0 — без ставней
		FColor TrimCol = FColor(235, 230, 220, 255);
		int32 Roof = Roof_Mansard;
		int32 Shops = 0;                                 // битовая маска сторон с лавками: 1 −Z, 2 +X, 4 +Z, 8 −X
		int32 BalconyFloor = 0;                          // этаж с балконом (0 — нет)
		int32 StreetSides = 0;                           // стороны на улицу (детальные фасады)
		uint32 Seed = 0;
	};

	/** генератор псевдослучайных чисел оригинала (LCG, 64 бита) */
	struct FRng
	{
		uint64 S = 0;
		explicit FRng(uint64 InS) : S(InS) {}
		uint32 U32() { S = S * 6364136223846793005ULL + 1442695040888963407ULL; return (uint32)(S >> 33); }
		float F() { return (float)(U32() & 0xFFFFFFu) / 16777216.f; }
		float Range(float A, float B) { return A + (B - A) * F(); }
	};

	/** рамка фасада: начало, «вправо», нормаль наружу; локальные u — вдоль, v — вверх, d — наружу */
	struct FFacadeFrame
	{
		FV3 O, R, N;
		FFacadeFrame() {}
		FFacadeFrame(const FV3& InO, const FV3& InR, const FV3& InN) : O(InO), R(InR), N(InN) {}
		FV3 P(float U, float V, float D) const { return O + R * U + FV3(0.f, V, 0.f) + N * D; }
	};

	class FBuilder
	{
	public:
		/** область в метрах (XZ) и размер ячейки */
		void Init(float MinX, float MinZ, float MaxX, float MaxZ, float InChunk);
		/** весь квартал (кварталы 3×2, переулки, площадь с фонтаном, лавки, деревья, телеги с сеном, ящики, фонари) */
		void BuildLevel(int32 Seed);

		// --- примитивы оригинала ---
		FMeshBuf& Mb(const FV3& At, int32 Mat);
		void Building(const FBuilding& B);
		void Ground(int32 Mat, float X0, float Z0, float X1, float Z1, float Y, float UVS, bool bCollide);
		void Box(int32 Mat, const FV3& Mn, const FV3& Mx, float UVS, FColor Col, bool bCollide);
		void Lamp(const FV3& P);
		void Tree(const FV3& P, float H, uint32 Seed);
		void HayCart(const FV3& P, float Yaw);
		void Stall(const FV3& P, float Yaw, FColor Awning);
		void Fountain(const FV3& P, float R);
		void Crates(const FV3& P, int32 Count, uint32 Seed);

		/** высота мостовой/тротуара/площади в точке (для точек прогулок и фонарей) */
		float GroundHeight(float X, float Z) const;
		/** пересекает ли капсула человека (0.4..1.6 м над точкой, радиус R) что-нибудь твёрдое */
		bool CapsuleBlocked(const FV3& P, float R) const;
		/** число треугольников во всех видимых буферах */
		int32 CountTriangles() const;

		// --- результат ---
		int32 CW = 0, CH = 0;
		float Chunk = 24.f;
		FV3 Origin;
		TArray<FMeshBuf> Bufs;       // [ячейка * CM_Num + материал]
		TArray<FMeshBuf> Coll;       // [ячейка] — невидимая геометрия столкновений (как lu_phys в оригинале)
		TArray<FMeshBuf> SoftBufs;   // [материал] — мягкие предметы (телеги с сеном)
		FMeshBuf SoftColl;
		TArray<FV3> LampPts;         // точка света фонаря (≈4 м над землёй)
		TArray<FV3> HayPts;          // верх копны сена
		TArray<FV3> WalkPts;         // тротуары и площадь, свободные от построек
		FV3 PlayerStart;
		int32 NumBuildings = 0;

	private:
		struct FSolid { FV3 Mn, Mx; };
		struct FFlat { float X0, Z0, X1, Z1, Y; };
		TArray<FSolid> Solids;       // для проверки точек прогулок
		TArray<FFlat> Flats;         // приподнятые площадки (тротуары, площадь)
		bool bSoft = false;          // писать в мягкие буферы

		int32 ChunkIndex(const FV3& At) const;
		FMeshBuf& CollBuf(const FV3& At);
		void PhysBox(const FV3& Mn, const FV3& Mx, bool bGround = false);
		void PhysTri(const FV3& A, const FV3& B, const FV3& C);
		void PhysObb(const FV3& Center, float Yaw, const FV3& Half);

		void Quad(FMeshBuf& M, const FV3& A, const FV3& B, const FV3& C, const FV3& D, FV2 UA, FV2 UB, FV2 UC, FV2 UD, FColor Col);
		void FrameRect(int32 Mat, const FFacadeFrame& F, float U0, float U1, float V0, float V1, float D, float UVS, FColor Col);
		/** коробка в рамке фасада (без задней грани, как в оригинале; bBack — с задней гранью для отстоящих от стены деталей) */
		void FrameBox(int32 Mat, const FFacadeFrame& F, float U0, float U1, float V0, float V1, float D0, float D1, float UVS, FColor Col, bool bCollide, bool bBack = false);
		void RoofQuad(int32 Mat, const FV3& A, const FV3& B, const FV3& C, const FV3& D, float UVS);
		void RoofTri(int32 Mat, const FV3& A, const FV3& B, const FV3& C, float UVS);
		/** горизонтальная грань проёма (верх ниши смотрит вниз, низ — вверх) на высоте V от глубины D0 до D1 */
		void Soffit(int32 Mat, const FFacadeFrame& F, float U0, float U1, float V, float D0, float D1, float UVS, FColor Col, bool bFaceUp);
		void Window(const FBuilding& B, const FFacadeFrame& F, int32 WM, float UC, float V0, float W, float H, bool bDetail, bool bFrench, bool bDoor, FRng& Rng);
		void Facade(const FBuilding& B, int32 Side, FRng& Rng);
		void Chimney(const FV3& Base, float H, FColor Tint);
		void RoofOf(const FBuilding& B, FRng& Rng);
	};

	// --- примитивы буфера (как lu_mb_*) ---
	void MbQuadBox(FMeshBuf& M, const FV3& A, const FV3& C, const FV3& D, const FV3& E, FV2 UV0, FV2 UV1);
	void MbBox(FMeshBuf& M, const FV3& Mn, const FV3& Mx, float S);
	/** тело вращения вокруг оси Y; Profile — (радиус, высота); UVPerMeter — плотность UV (0 — как в оригинале: u 0..1 по кругу) */
	void MbLathe(FMeshBuf& M, const FV3& Base, const FV2* Profile, int32 Count, int32 Seg, float UVPerMeter = 0.f);
	void MbCylinder(FMeshBuf& M, const FV3& Base, float R0, float R1, float H, int32 Seg, bool bCaps);
	void MbSphere(FMeshBuf& M, const FV3& C, const FV3& R, int32 Seg, int32 Ring, float UVScale = 1.f);
	void MbTube(FMeshBuf& M, const FV3& A, const FV3& C, float R0, float R1, int32 Seg);
	/** повернуть (ось, угол) и сдвинуть вершины, начиная с From */
	void MbTransform(FMeshBuf& M, int32 From, const FV3& Axis, float Angle, const FV3& Move);
}

// Тени Парижа — сенсоры паркура: поиск стен, уступов, карнизов и препятствий трассировками по любой геометрии.
// Владелец модуля: агент «паркур». Перенос lu_probe_wall / lu_find_ledge / lu_find_grab / lu_find_vault /
// lu_ledge_continues из legacy/lutece/phys/lu_phys.c (метры, Y вверх → сантиметры, Z вверх).
#pragma once

#include "CoreMinimal.h"
#include "TPTypes.h"

class UWorld;
class AActor;
struct FHitResult;

/** Результат одного луча сенсора. */
struct FTPSensorHit
{
	bool bHit = false;
	FVector Point = FVector::ZeroVector;
	FVector Normal = FVector::ZeroVector;
	float Distance = 0.f;
	/** поверхность помечена тегом "TPNoClimb" (актор или компонент) */
	bool bNoClimb = false;
	/** поверхность помечена тегом "TPSoft" (стог сена и т. п.) */
	bool bSoft = false;
};

/**
 * Набор сенсоров паркура. Не UObject: создаётся внутри компонента и перед использованием
 * получает мир и персонажа через Setup(). Все трассировки — по каналу ECC_Visibility,
 * сам персонаж и любые другие Pawn игнорируются. Все высоты — над ногами (низ капсулы).
 */
class TENIPARIZHA_API FTPTraversalSensors
{
public:
	/** «пола нет» для FloorAt */
	static constexpr float NoFloor = -1.e9f;

	static const FName TagNoClimb;
	static const FName TagSoft;

	void Setup(UWorld* InWorld, const AActor* InSelf, bool bInDebug);
	bool IsReady() const { return World != nullptr; }

	/** луч из From по направлению Dir (нормализуется) длиной MaxDist */
	bool Ray(const FVector& From, const FVector& Dir, float MaxDist, FTPSensorHit& Out) const;

	/** пересекается ли вертикальная капсула (или сфера, если HalfHeight <= Radius) с блокирующей геометрией */
	bool Blocked(const FVector& Center, float Radius, float HalfHeight) const;

	/** стена перед персонажем: луч на высоте Height над ногами по горизонтали Dir; расстояние или -1 */
	float ProbeWall(const FVector& Feet, const FVector& Dir, float Height, float MaxDist, FVector* OutNormal = nullptr, bool* bOutNoClimb = nullptr) const;

	/** высота пола под точкой (луч вниз из P.Z + Up длиной Up + Down); NoFloor, если пола нет */
	float FloorAt(const FVector& P, float Up, float Down, bool* bOutSoft = nullptr) const;

	/** уступ впереди: кромка на высоте [HMin, HMax] над ногами, передняя грань не дальше Reach */
	bool FindLedge(const FVector& Feet, const FVector& Dir, float HMin, float HMax, float Reach, FTPLedge& Out) const;

	/** уступ у рук в полёте: Hands — точка рук, HandsAboveFeet — высота рук над ногами в висе */
	bool FindGrab(const FVector& Hands, const FVector& Facing, float HandsAboveFeet, float Reach, FTPLedge& Out) const;

	/**
	 * Препятствие для перемаха высотой 40–130 см. OutDepth > 0 — глубина препятствия (за ним свободно, перемахнуть);
	 * OutDepth == 0 — препятствие широкое, но на него можно встать (запрыгнуть сверху).
	 */
	bool FindVault(const FVector& Feet, const FVector& Dir, float Reach, FTPLedge& OutLedge, float& OutDepth) const;

	/** продолжается ли кромка вбок (Side = ±1) на расстояние Dist — для перебежки по карнизу в висе */
	bool LedgeContinues(const FTPLedge& L, float Side, float Dist, FTPLedge& Out) const;

	/** мягкая поверхность (тег "TPSoft") у ног: проверяет пересечение с любыми объектами, в т. ч. триггерами */
	bool IsSoftAt(const FVector& Feet, float Radius) const;

	/** помечен ли актор или компонент попадания тегом */
	static bool HasSurfaceTag(const FHitResult& Hit, FName Tag);

	/** отладочная точка (только при bDebug) */
	void DebugPoint(const FVector& P, const FColor& Color, float Size = 8.f) const;
	void DebugLine(const FVector& A, const FVector& B, const FColor& Color) const;

private:
	bool Trace(const FVector& A, const FVector& B, FHitResult& Hit) const;

	// слабые ссылки: структура не отражается, сборщик мусора её не видит
	TWeakObjectPtr<UWorld> World;
	TWeakObjectPtr<const AActor> Self;
	bool bDebug = false;
};

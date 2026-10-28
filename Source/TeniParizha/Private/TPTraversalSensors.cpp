// Тени Парижа — сенсоры паркура (перенос из legacy/lutece/phys/lu_phys.c, единицы — сантиметры, Z вверх).
#include "TPTraversalSensors.h"
#include "Engine/World.h"
#include "Engine/EngineTypes.h"
#include "WorldCollision.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Components/PrimitiveComponent.h"
#include "DrawDebugHelpers.h"

const FName FTPTraversalSensors::TagNoClimb(TEXT("TPNoClimb"));
const FName FTPTraversalSensors::TagSoft(TEXT("TPSoft"));

void FTPTraversalSensors::Setup(UWorld* InWorld, const AActor* InSelf, bool bInDebug)
{
	World = InWorld;
	Self = InSelf;
	bDebug = bInDebug;
}

bool FTPTraversalSensors::HasSurfaceTag(const FHitResult& Hit, FName Tag)
{
	if (const AActor* A = Hit.GetActor())
	{
		if (A->ActorHasTag(Tag)) return true;
	}
	if (const UPrimitiveComponent* C = Hit.GetComponent())
	{
		if (C->ComponentHasTag(Tag)) return true;
	}
	return false;
}

void FTPTraversalSensors::DebugPoint(const FVector& P, const FColor& Color, float Size) const
{
	if (bDebug && World.IsValid()) DrawDebugSphere(World.Get(), P, Size, 8, Color, false, -1.f, 0, 1.f);
}

void FTPTraversalSensors::DebugLine(const FVector& A, const FVector& B, const FColor& Color) const
{
	if (bDebug && World.IsValid()) DrawDebugLine(World.Get(), A, B, Color, false, -1.f, 0, 0.6f);
}

/* Одна трассировка с пропуском других персонажей (Pawn): они не должны давать уступов и стен. */
bool FTPTraversalSensors::Trace(const FVector& A, const FVector& B, FHitResult& Hit) const
{
	if (!World.IsValid()) return false;
	FCollisionQueryParams Params(FName(TEXT("TPTraversal")), false, Self.Get());
	for (int32 Attempt = 0; Attempt < 4; ++Attempt)
	{
		if (!World.Get()->LineTraceSingleByChannel(Hit, A, B, ECC_Visibility, Params)) return false;
		AActor* HitActor = Hit.GetActor();
		if (HitActor && HitActor->IsA<APawn>())
		{
			Params.AddIgnoredActor(HitActor);
			continue;
		}
		return true;
	}
	return false;
}

bool FTPTraversalSensors::Ray(const FVector& From, const FVector& Dir, float MaxDist, FTPSensorHit& Out) const
{
	Out = FTPSensorHit();
	const FVector D = Dir.GetSafeNormal();
	if (D.IsNearlyZero() || MaxDist <= 0.f) return false;
	const FVector End = From + D * MaxDist;
	FHitResult Hit;
	const bool bHit = Trace(From, End, Hit);
	if (bHit)
	{
		Out.bHit = true;
		if (Hit.bStartPenetrating)
		{
			// луч начался внутри геометрии: считаем попаданием в начале луча, нормаль — навстречу
			Out.Point = From;
			Out.Normal = -D;
			Out.Distance = 0.f;
		}
		else
		{
			Out.Point = Hit.ImpactPoint;
			Out.Normal = Hit.ImpactNormal.GetSafeNormal();
			Out.Distance = static_cast<float>(Hit.Distance);
		}
		Out.bNoClimb = HasSurfaceTag(Hit, TagNoClimb);
		Out.bSoft = HasSurfaceTag(Hit, TagSoft);
	}
	DebugLine(From, bHit ? Out.Point : End, bHit ? FColor::Red : FColor::Green);
	return bHit;
}

bool FTPTraversalSensors::Blocked(const FVector& Center, float Radius, float HalfHeight) const
{
	if (!World.IsValid()) return false;
	const FCollisionShape Shape = HalfHeight > Radius ? FCollisionShape::MakeCapsule(Radius, HalfHeight) : FCollisionShape::MakeSphere(Radius);
	FCollisionQueryParams Params(FName(TEXT("TPTraversalOverlap")), false, Self.Get());
	TArray<FOverlapResult> Overlaps;
	World.Get()->OverlapMultiByChannel(Overlaps, Center, FQuat::Identity, ECC_Visibility, Shape, Params);
	bool bBlocked = false;
	for (const FOverlapResult& O : Overlaps)
	{
		if (!O.bBlockingHit) continue;
		const AActor* A = O.GetActor();
		if (A && A->IsA<APawn>()) continue;
		bBlocked = true;
		break;
	}
	if (bDebug && Radius > 10.f)
	{
		DrawDebugCapsule(World.Get(), Center, FMath::Max(HalfHeight, Radius), Radius, FQuat::Identity, bBlocked ? FColor::Red : FColor::Cyan, false, -1.f, 0, 0.6f);
	}
	return bBlocked;
}

/* lu_probe_wall */
float FTPTraversalSensors::ProbeWall(const FVector& Feet, const FVector& Dir, float Height, float MaxDist, FVector* OutNormal, bool* bOutNoClimb) const
{
	const FVector D = FVector(Dir.X, Dir.Y, 0.f).GetSafeNormal();
	if (D.IsNearlyZero()) return -1.f;
	FTPSensorHit H;
	if (!Ray(Feet + FVector(0.f, 0.f, Height), D, MaxDist, H)) return -1.f;
	if (FMath::Abs(H.Normal.Z) > 0.5f) return -1.f;          // не стена (пол, скат, потолок)
	if (OutNormal) *OutNormal = FVector(H.Normal.X, H.Normal.Y, 0.f).GetSafeNormal();
	if (bOutNoClimb) *bOutNoClimb = H.bNoClimb;
	return H.Distance;
}

/* lu_floor_at */
float FTPTraversalSensors::FloorAt(const FVector& P, float Up, float Down, bool* bOutSoft) const
{
	FTPSensorHit H;
	if (!Ray(FVector(P.X, P.Y, P.Z + Up), FVector::DownVector, Up + Down, H)) return NoFloor;
	if (bOutSoft) *bOutSoft = H.bSoft;
	return static_cast<float>(H.Point.Z);
}

/* lu_find_ledge: идём вперёд и сверху вниз ищем первую горизонтальную площадку нужной высоты
   (так находятся и выступающие карнизы), затем проверяем переднюю грань и свободное место над кромкой. */
bool FTPTraversalSensors::FindLedge(const FVector& Feet, const FVector& Dir, float HMin, float HMax, float Reach, FTPLedge& Out) const
{
	Out = FTPLedge();
	if (!World.IsValid()) return false;
	const FVector D = FVector(Dir.X, Dir.Y, 0.f).GetSafeNormal();
	if (D.IsNearlyZero()) return false;
	const float FeetZ = static_cast<float>(Feet.Z);

	for (float S = 12.f; S <= Reach + 35.f; S += 8.f)
	{
		const FVector P = Feet + D * S;
		const FVector O(P.X, P.Y, FeetZ + HMax + 30.f);
		if (Blocked(O, 4.f, 4.f)) continue;                    // начало луча внутри стены

		FTPSensorHit Top;
		if (!Ray(O, FVector::DownVector, HMax - HMin + 50.f, Top)) continue;
		if (Top.Normal.Z < 0.65) continue;                      // не площадка
		const float TopZ = static_cast<float>(Top.Point.Z);
		if (TopZ < FeetZ + HMin || TopZ > FeetZ + HMax) continue;

		// передняя грань: горизонтальный луч чуть ниже кромки, из-за спины персонажа
		const FVector St = FVector(Feet.X, Feet.Y, TopZ - 7.f) - D * 30.f;
		FTPSensorHit Face;
		if (!Ray(St, D, S + 60.f, Face) || FMath::Abs(Face.Normal.Z) > 0.5f) continue;
		if (Face.Distance <= 0.f) continue;                     // луч начался внутри геометрии
		if (Face.Distance - 30.f > Reach) continue;
		if (Face.bNoClimb || Top.bNoClimb) return false;        // за эту кромку хвататься нельзя

		// над кромкой свободно
		FTPSensorHit Above;
		if (Ray(FVector(St.X, St.Y, TopZ + 22.f), D, Face.Distance + 25.f, Above)) continue;

		const FVector WN = FVector(Face.Normal.X, Face.Normal.Y, 0.f).GetSafeNormal();
		if (WN.IsNearlyZero()) continue;

		Out.bValid = true;
		Out.WallNormal = WN;
		Out.WallPoint = Face.Point;
		Out.Edge = FVector(Face.Point.X, Face.Point.Y, TopZ);
		// вправо, если смотреть на стену (взгляд = -WN): Right = Up x Fwd = WN x Up
		Out.EdgeDir = FVector::CrossProduct(WN, FVector::UpVector).GetSafeNormal();
		Out.Height = TopZ - FeetZ;

		// можно ли встать наверху: пол чуть дальше кромки и свободная капсула
		const FVector SP = Out.Edge - WN * 50.f;
		float FY = FloorAt(FVector(SP.X, SP.Y, TopZ + 80.f), 0.f, 140.f);
		if (FY < TopZ - 60.f || FY > TopZ + 70.f) FY = TopZ;
		Out.bCanStand = !Blocked(FVector(SP.X, SP.Y, FY + 100.f), 30.f, 80.f);

		DebugPoint(Out.Edge, Out.bCanStand ? FColor::Yellow : FColor::Orange, 6.f);
		return true;
	}
	return false;
}

/* lu_find_grab: сначала прямо вперёд, затем немного по сторонам (±~29°) */
bool FTPTraversalSensors::FindGrab(const FVector& Hands, const FVector& Facing, float HandsAboveFeet, float Reach, FTPLedge& Out) const
{
	FVector F = Facing.GetSafeNormal2D();
	if (F.IsNearlyZero()) F = FVector::ForwardVector;
	const FVector Feet = Hands - FVector(0.f, 0.f, HandsAboveFeet);
	static const float Angles[3] = { 0.f, 28.6f, -28.6f };
	for (const float A : Angles)
	{
		const FVector D = F.RotateAngleAxis(A, FVector::UpVector);
		if (FindLedge(Feet, D, HandsAboveFeet - 30.f, HandsAboveFeet + 30.f, Reach, Out)) return true;
	}
	return false;
}

/* lu_find_vault (+ запрыгивание на широкое невысокое препятствие) */
bool FTPTraversalSensors::FindVault(const FVector& Feet, const FVector& Dir, float Reach, FTPLedge& OutLedge, float& OutDepth) const
{
	OutDepth = 0.f;
	if (!FindLedge(Feet, Dir, 40.f, 130.f, Reach, OutLedge)) return false;
	const FVector Into = -OutLedge.WallNormal;
	const float TopZ = static_cast<float>(OutLedge.Edge.Z);

	// где кончается верх препятствия
	float Depth = 0.f;
	for (float T = 20.f; T <= 160.f; T += 15.f)
	{
		const FVector P = OutLedge.Edge + Into * T;
		const float FY = FloorAt(FVector(P.X, P.Y, TopZ + 30.f), 0.f, 50.f);
		if (FY < TopZ - 20.f) { Depth = T; break; }
	}
	if (Depth <= 0.f)
	{
		// препятствие широкое: перемахнуть нельзя, но можно запрыгнуть сверху
		return OutLedge.bCanStand;
	}
	// место приземления за препятствием свободно
	const FVector Land = OutLedge.Edge + Into * (Depth + 60.f);
	if (Blocked(FVector(Land.X, Land.Y, Feet.Z + 105.f), 30.f, 83.f)) return OutLedge.bCanStand;
	OutDepth = Depth;
	return true;
}

/* lu_ledge_continues */
bool FTPTraversalSensors::LedgeContinues(const FTPLedge& L, float Side, float Dist, FTPLedge& Out) const
{
	const FVector P = L.Edge + L.EdgeDir * (Side * Dist);
	const FVector Feet = FVector(P.X, P.Y, L.Edge.Z - L.Height) + L.WallNormal * 50.f;
	if (!FindLedge(Feet, -L.WallNormal, L.Height - 35.f, L.Height + 35.f, 90.f, Out)) return false;
	return FMath::Abs(Out.Edge.Z - L.Edge.Z) < 30.f;
}

bool FTPTraversalSensors::IsSoftAt(const FVector& Feet, float Radius) const
{
	if (!World.IsValid()) return false;
	FCollisionQueryParams Params(FName(TEXT("TPTraversalSoft")), false, Self.Get());
	TArray<FOverlapResult> Overlaps;
	World.Get()->OverlapMultiByObjectType(Overlaps, Feet + FVector(0.f, 0.f, 20.f), FQuat::Identity,
		FCollisionObjectQueryParams(FCollisionObjectQueryParams::AllObjects), FCollisionShape::MakeSphere(Radius), Params);
	for (const FOverlapResult& O : Overlaps)
	{
		const AActor* A = O.GetActor();
		if (A && A->ActorHasTag(TagSoft)) return true;
		const UPrimitiveComponent* C = O.GetComponent();
		if (C && C->ComponentHasTag(TagSoft)) return true;
	}
	return false;
}

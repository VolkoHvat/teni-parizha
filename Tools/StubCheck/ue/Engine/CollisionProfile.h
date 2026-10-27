#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Engine/EngineTypes.h"
struct FCollisionResponseTemplate { FName Name; TEnumAsByte<ECollisionEnabled::Type> CollisionEnabled; TEnumAsByte<ECollisionChannel> ObjectType; FName ObjectTypeName; FCollisionResponseContainer ResponseToChannels; };
class UCollisionProfile : public UObject
{
public:
	typedef UObject Super; typedef UCollisionProfile ThisClass; static UClass* StaticClass();
	UCollisionProfile(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	static UCollisionProfile* Get();
	static const FName NoCollision_ProfileName, BlockAll_ProfileName, PhysicsActor_ProfileName, BlockAllDynamic_ProfileName, Pawn_ProfileName, Vehicle_ProfileName, DefaultProjectile_ProfileName, CustomCollisionProfileName;
	bool GetProfileTemplate(FName ProfileName, FCollisionResponseTemplate& ProfileData) const;
	ECollisionChannel ConvertToCollisionChannel(bool TraceType, int32 Index) const;
	EObjectTypeQuery ConvertToObjectType(ECollisionChannel CollisionChannel) const;
	ETraceTypeQuery ConvertToTraceType(ECollisionChannel CollisionChannel) const;
};

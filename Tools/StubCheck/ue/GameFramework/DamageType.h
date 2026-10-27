#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "UObject/Object.h"
class UDamageType : public UObject
{
public:
	typedef UObject Super; typedef UDamageType ThisClass; static UClass* StaticClass();
	UDamageType(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	uint32 bCausedByWorld : 1; uint32 bScaleMomentumByMass : 1; uint32 bRadialDamageVelChange : 1;
	float DamageImpulse; float DestructibleImpulse; float DestructibleDamageSpreadScale; float DamageFalloff;
};

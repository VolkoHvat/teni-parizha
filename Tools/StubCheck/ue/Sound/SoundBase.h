#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "UObject/Object.h"
class USoundClass; class USoundAttenuation; class USoundConcurrency;
class USoundBase : public UObject
{
public:
	typedef UObject Super; typedef USoundBase ThisClass; static UClass* StaticClass();
	USoundBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	USoundClass* SoundClassObject; USoundAttenuation* AttenuationSettings; float Duration; float MaxDistance; float Priority; uint8 bDebug : 1; uint8 bOverrideConcurrency : 1;
	virtual float GetDuration(); virtual bool IsPlayable() const; virtual float GetMaxDistance() const; virtual bool IsLooping();
};

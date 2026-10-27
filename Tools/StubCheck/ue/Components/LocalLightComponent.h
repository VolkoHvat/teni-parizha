#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "Components/LightComponent.h"
class ULocalLightComponent : public ULightComponent
{
public:
	typedef ULightComponent Super; typedef ULocalLightComponent ThisClass; static UClass* StaticClass();
	ULocalLightComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	ELightUnits IntensityUnits;
	float AttenuationRadius;
	void SetAttenuationRadius(float NewRadius);
	void SetIntensityUnits(ELightUnits NewIntensityUnits);
	static float GetUnitsConversionFactor(ELightUnits SrcUnits, ELightUnits TargetUnits, float CosHalfConeAngle = -1);
};

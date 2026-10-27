#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "Components/PointLightComponent.h"
class USpotLightComponent : public UPointLightComponent
{
public:
	typedef UPointLightComponent Super; typedef USpotLightComponent ThisClass; static UClass* StaticClass();
	USpotLightComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	float InnerConeAngle; float OuterConeAngle;
	void SetInnerConeAngle(float NewInnerConeAngle); void SetOuterConeAngle(float NewOuterConeAngle);
};

#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "Components/LocalLightComponent.h"
class UPointLightComponent : public ULocalLightComponent
{
public:
	typedef ULocalLightComponent Super; typedef UPointLightComponent ThisClass; static UClass* StaticClass();
	UPointLightComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	float Radius_DEPRECATED; uint32 bUseInverseSquaredFalloff : 1; float LightFalloffExponent; float SourceRadius; float SoftSourceRadius; float SourceLength;
	void SetUseInverseSquaredFalloff(bool bNewValue);
	void SetLightFalloffExponent(float NewLightFalloffExponent);
	void SetSourceRadius(float bNewValue);
	void SetSoftSourceRadius(float bNewValue);
	void SetSourceLength(float NewValue);
};

#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
class ULightComponentBase : public USceneComponent
{
public:
	typedef USceneComponent Super; typedef ULightComponentBase ThisClass; static UClass* StaticClass();
	ULightComponentBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	FGuid LightGuid; float Brightness_DEPRECATED; float Intensity; FColor LightColor;
	uint32 bAffectsWorld : 1; uint32 CastShadows : 1; uint32 CastStaticShadows : 1; uint32 CastDynamicShadows : 1; uint32 bAffectTranslucentLighting : 1;
	uint32 bTransmission : 1; uint32 bCastVolumetricShadow : 1; uint32 bCastDeepShadow : 1; uint32 CastRaytracedShadow : 1; uint32 bAffectReflection : 1; uint32 bAffectGlobalIllumination : 1;
	float IndirectLightingIntensity; float VolumetricScatteringIntensity; float SamplesPerPixel; float DeepShadowLayerDistribution;
	void SetCastShadows(bool bNewValue);
	FLinearColor GetLightColor() const;
	void SetCastVolumetricShadow(bool bNewValue); void SetCastDeepShadow(bool bNewValue); void SetAffectReflection(bool bNewValue); void SetAffectGlobalIllumination(bool bNewValue);
	void SetCastRaytracedShadow(bool bNewValue); void SetSamplesPerPixel(int32 NewValue);
	virtual void UpdateLightGUIDs();
	bool HasStaticLighting() const; bool HasStaticShadowing() const;
};

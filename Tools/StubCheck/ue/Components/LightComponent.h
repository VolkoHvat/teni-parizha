#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "Components/LightComponentBase.h"
class UMaterialInterface; class UTextureLightProfile;
class ULightComponent : public ULightComponentBase
{
public:
	typedef ULightComponentBase Super; typedef ULightComponent ThisClass; static UClass* StaticClass();
	ULightComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	float Temperature; float MaxDrawDistance; float MaxDistanceFadeRange;
	uint32 bUseTemperature : 1; uint32 bUseRayTracedDistanceFieldShadows : 1; uint32 bCastShadowsFromCinematicObjectsOnly : 1; uint32 bAffectDynamicIndirectLighting : 1;
	uint32 bForceCachedShadowsForMovablePrimitives : 1; uint32 bUseIESBrightness : 1; uint32 bEnableLightShaftBloom : 1;
	int32 ShadowMapChannel_DEPRECATED; float MinRoughness; float SpecularScale; float ShadowResolutionScale; float ShadowBias; float ShadowSlopeBias; float ShadowSharpen;
	float ContactShadowLength; uint32 ContactShadowLengthInWS : 1; float IESBrightnessScale; float BloomScale; float BloomThreshold; FColor BloomTint;
	UTextureLightProfile* IESTexture; UMaterialInterface* LightFunctionMaterial; FVector LightFunctionScale; float LightFunctionFadeDistance; float DisabledBrightness;
	FLightingChannels LightingChannels;
	void SetIntensity(float NewIntensity);
	void SetIndirectLightingIntensity(float NewIntensity);
	void SetVolumetricScatteringIntensity(float NewIntensity);
	void SetLightColor(FLinearColor NewLightColor, bool bSRGB = true);
	void SetTemperature(float NewTemperature);
	void SetUseTemperature(bool bNewValue);
	void SetLightFunctionMaterial(UMaterialInterface* NewLightFunctionMaterial);
	void SetLightFunctionScale(FVector NewLightFunctionScale);
	void SetLightFunctionFadeDistance(float NewLightFunctionFadeDistance);
	void SetLightFunctionDisabledBrightness(float NewValue);
	void SetAffectDynamicIndirectLighting(bool bNewValue);
	void SetAffectTranslucentLighting(bool bNewValue);
	void SetTransmission(bool bNewValue);
	void SetEnableLightShaftBloom(bool bNewValue);
	void SetBloomScale(float NewValue); void SetBloomThreshold(float NewValue); void SetBloomTint(FColor NewValue);
	void SetIESTexture(UTextureLightProfile* NewValue); void SetIESBrightnessScale(float NewValue);
	void SetShadowBias(float NewValue); void SetShadowSlopeBias(float NewValue); void SetSpecularScale(float NewValue);
	void SetForceCachedShadowsForMovablePrimitives(bool bNewValue);
	void SetLightingChannels(bool bChannel0, bool bChannel1, bool bChannel2);
	float ComputeLightBrightness() const; void SetLightBrightness(float InBrightness);
	virtual float GetUniformPenumbraSize() const;
};

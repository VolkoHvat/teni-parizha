#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
class UTextureCube;
struct FExponentialHeightFogData { float FogDensity; float FogHeightFalloff; float FogHeightOffset; };
class UExponentialHeightFogComponent : public USceneComponent
{
public:
	typedef USceneComponent Super; typedef UExponentialHeightFogComponent ThisClass; static UClass* StaticClass();
	UExponentialHeightFogComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	float FogDensity; float FogHeightFalloff; FExponentialHeightFogData SecondFogData; FLinearColor FogInscatteringColor;
	UTextureCube* InscatteringColorCubemap; float InscatteringColorCubemapAngle; FLinearColor InscatteringTextureTint;
	float FullyDirectionalInscatteringColorDistance; float NonDirectionalInscatteringColorDistance;
	float DirectionalInscatteringExponent; float DirectionalInscatteringStartDistance; FLinearColor DirectionalInscatteringColor;
	float FogMaxOpacity; float StartDistance; float FogCutoffDistance;
	bool bEnableVolumetricFog; float VolumetricFogScatteringDistribution; FColor VolumetricFogAlbedo; FLinearColor VolumetricFogEmissive;
	float VolumetricFogExtinctionScale; float VolumetricFogDistance; float VolumetricFogStaticLightingScatteringIntensity; bool bOverrideLightColorsWithFogInscatteringColors;
	void SetFogDensity(float Value); void SetFogInscatteringColor(FLinearColor Value); void SetInscatteringColorCubemap(UTextureCube* Value);
	void SetInscatteringColorCubemapAngle(float Value); void SetFullyDirectionalInscatteringColorDistance(float Value); void SetNonDirectionalInscatteringColorDistance(float Value);
	void SetInscatteringTextureTint(FLinearColor Value); void SetDirectionalInscatteringExponent(float Value); void SetDirectionalInscatteringStartDistance(float Value);
	void SetDirectionalInscatteringColor(FLinearColor Value); void SetFogHeightFalloff(float Value); void SetFogMaxOpacity(float Value); void SetStartDistance(float Value);
	void SetFogCutoffDistance(float Value); void SetVolumetricFog(bool bNewValue); void SetVolumetricFogScatteringDistribution(float NewValue);
	void SetVolumetricFogExtinctionScale(float NewValue); void SetVolumetricFogAlbedo(FColor NewValue); void SetVolumetricFogEmissive(FLinearColor NewValue); void SetVolumetricFogDistance(float NewValue);
};

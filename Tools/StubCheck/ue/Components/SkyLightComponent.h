#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "Components/LightComponentBase.h"
class UTextureCube;
enum ESkyLightSourceType { SLS_CapturedScene, SLS_SpecifiedCubemap, SLS_MAX };
class USkyLightComponent : public ULightComponentBase
{
public:
	typedef ULightComponentBase Super; typedef USkyLightComponent ThisClass; static UClass* StaticClass();
	USkyLightComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	TEnumAsByte<ESkyLightSourceType> SourceType; UTextureCube* Cubemap; float SourceCubemapAngle; int32 CubemapResolution; float SkyDistanceThreshold;
	bool bCaptureEmissiveOnly; bool bLowerHemisphereIsBlack; FLinearColor LowerHemisphereColor; bool bRealTimeCapture;
	float OcclusionMaxDistance; float Contrast; float OcclusionExponent; float MinOcclusion; FColor OcclusionTint; TEnumAsByte<EOcclusionCombineMode> OcclusionCombineMode;
	void RecaptureSky();
	void SetIntensity(float NewIntensity);
	void SetIndirectLightingIntensity(float NewIntensity);
	void SetVolumetricScatteringIntensity(float NewIntensity);
	void SetLightColor(FLinearColor NewLightColor);
	void SetCubemap(UTextureCube* NewCubemap);
	void SetCubemapBlend(UTextureCube* SourceCubemap, UTextureCube* DestinationCubemap, float InBlendFraction);
	void SetLowerHemisphereColor(const FLinearColor& InLowerHemisphereColor);
	void SetOcclusionTint(const FColor& InTint); void SetOcclusionContrast(float InOcclusionContrast); void SetOcclusionExponent(float InOcclusionExponent); void SetMinOcclusion(float InMinOcclusion);
	void SetSourceCubemapAngle(float NewValue);
	static void UpdateSkyCaptureContents(UWorld* WorldToUpdate);
};

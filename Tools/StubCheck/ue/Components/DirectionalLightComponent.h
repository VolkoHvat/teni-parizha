#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "Components/LightComponent.h"
class UDirectionalLightComponent : public ULightComponent
{
public:
	typedef ULightComponent Super; typedef UDirectionalLightComponent ThisClass; static UClass* StaticClass();
	UDirectionalLightComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	uint32 bEnableLightShaftOcclusion : 1; float OcclusionMaskDarkness; float OcclusionDepthRange; FVector LightShaftOverrideDirection;
	float WholeSceneDynamicShadowRadius_DEPRECATED; float DynamicShadowDistanceMovableLight; float DynamicShadowDistanceStationaryLight;
	int32 DynamicShadowCascades; float CascadeDistributionExponent; float CascadeTransitionFraction; float ShadowDistanceFadeoutFraction;
	uint32 bUseInsetShadowsForMovableObjects : 1; int32 FarShadowCascadeCount; float FarShadowDistance; float DistanceFieldShadowDistance;
	float LightSourceAngle; float LightSourceSoftAngle; float ShadowAmount; float TraceDistance;
	uint32 bAtmosphereSunLight : 1; int32 AtmosphereSunLightIndex; FLinearColor AtmosphereSunDiskColorScale; uint32 bPerPixelAtmosphereTransmittance : 1;
	uint32 bCastShadowsOnClouds : 1; uint32 bCastShadowsOnAtmosphere : 1; uint32 bCastCloudShadows : 1;
	float CloudShadowStrength; float CloudShadowOnAtmosphereStrength; float CloudShadowOnSurfaceStrength; float CloudShadowDepthBias; float CloudShadowExtent; float CloudShadowMapResolutionScale; float CloudShadowRaySampleCountScale;
	FLinearColor CloudScatteredLuminanceScale; uint32 bUsedAsAtmosphereSunLight_DEPRECATED : 1;
	void SetDynamicShadowDistanceMovableLight(float NewValue);
	void SetDynamicShadowDistanceStationaryLight(float NewValue);
	void SetDynamicShadowCascades(int32 NewValue);
	void SetCascadeDistributionExponent(float NewValue);
	void SetCascadeTransitionFraction(float NewValue);
	void SetShadowDistanceFadeoutFraction(float NewValue);
	void SetEnableLightShaftOcclusion(bool bNewValue);
	void SetOcclusionMaskDarkness(float NewValue);
	void SetLightShaftOverrideDirection(FVector NewValue);
	void SetShadowAmount(float NewValue);
	void SetAtmosphereSunLight(bool bNewValue);
	void SetAtmosphereSunLightIndex(int32 NewValue);
	void SetForwardShadingPriority(int32 NewValue);
};

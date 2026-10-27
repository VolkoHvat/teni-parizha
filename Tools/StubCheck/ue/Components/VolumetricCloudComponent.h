// Есть в 4.27, но по правилам проекта запрещён (тяжело для GTX 760) — линтер выдаёт ошибку.
#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
class UMaterialInterface;
class UVolumetricCloudComponent : public USceneComponent
{
public:
	typedef USceneComponent Super; typedef UVolumetricCloudComponent ThisClass; static UClass* StaticClass();
	UVolumetricCloudComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	float LayerBottomAltitude; float LayerHeight; float TracingStartMaxDistance; float TracingMaxDistance; float PlanetRadius; FColor GroundAlbedo; UMaterialInterface* Material;
	void SetLayerBottomAltitude(float NewValue); void SetLayerHeight(float NewValue); void SetTracingStartMaxDistance(float NewValue); void SetTracingMaxDistance(float NewValue);
	void SetPlanetRadius(float NewValue); void SetGroundAlbedo(FColor NewValue); void SetMaterial(UMaterialInterface* NewValue);
	void SetViewSampleCountScale(float NewValue); void SetReflectionSampleCountScale(float NewValue); void SetShadowViewSampleCountScale(float NewValue);
	void SetShadowReflectionSampleCountScale(float NewValue); void SetShadowTracingDistance(float NewValue); void SetStopTracingTransmittanceThreshold(float NewValue);
	void SetSkyLightCloudBottomOcclusion(float NewValue); void SetbUsePerSampleAtmosphericLightTransmittance(bool NewValue);
};

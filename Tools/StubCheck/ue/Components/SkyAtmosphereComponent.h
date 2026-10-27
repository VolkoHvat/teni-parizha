#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
class UDirectionalLightComponent;
enum class ESkyAtmosphereTransformMode : uint8 { PlanetTopAtAbsoluteWorldOrigin, PlanetTopAtComponentTransform, PlanetCenterAtComponentTransform };
class USkyAtmosphereComponent : public USceneComponent
{
public:
	typedef USceneComponent Super; typedef USkyAtmosphereComponent ThisClass; static UClass* StaticClass();
	USkyAtmosphereComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	ESkyAtmosphereTransformMode TransformMode; float BottomRadius; FColor GroundAlbedo; float AtmosphereHeight; float MultiScatteringFactor;
	float RayleighScatteringScale; FLinearColor RayleighScattering; float RayleighExponentialDistribution;
	float MieScatteringScale; FLinearColor MieScattering; float MieAbsorptionScale; FLinearColor MieAbsorption; float MieAnisotropy; float MieExponentialDistribution;
	float OtherAbsorptionScale; FLinearColor OtherAbsorption; FLinearColor SkyLuminanceFactor; float AerialPespectiveViewDistanceScale; float HeightFogContribution;
	float TransmittanceMinLightElevationAngle; float AerialPerspectiveStartDepth; float TraceSampleCountScale;
	void SetRayleighScatteringScale(float NewValue); void SetRayleighScattering(FLinearColor NewValue); void SetRayleighExponentialDistribution(float NewValue);
	void SetMieScatteringScale(float NewValue); void SetMieScattering(FLinearColor NewValue); void SetMieAbsorptionScale(float NewValue); void SetMieAbsorption(FLinearColor NewValue);
	void SetMieAnisotropy(float NewValue); void SetMieExponentialDistribution(float NewValue);
	void SetOtherAbsorptionScale(float NewValue); void SetOtherAbsorption(FLinearColor NewValue);
	void SetSkyLuminanceFactor(FLinearColor NewValue); void SetAerialPespectiveViewDistanceScale(float NewValue); void SetHeightFogContribution(float NewValue);
	void OverrideAtmosphereLightDirection(int32 AtmosphereLightIndex, const FVector& LightDirection);
	FLinearColor GetAtmosphereTransmitanceOnGroundAtPlanetTop(UDirectionalLightComponent* DirectionalLight);
};

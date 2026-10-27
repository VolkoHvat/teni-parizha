// Макет Engine/Scene.h (UE 4.27): FPostProcessSettings (основные поля).
#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
class UTexture; class UTextureCube; class UMaterialInterface;
enum EAutoExposureMethod { AEM_Histogram, AEM_Basic, AEM_Manual, AEM_MAX };
enum EBloomMethod { BM_SOG, BM_FFT, BM_MAX };
enum class EReflectionsType : uint8 { ScreenSpace, RayTracing };
enum class ETranslucencyType : uint8 { Raster, RayTracing };
enum EDepthOfFieldMethod { DOFM_BokehDOF, DOFM_Gaussian, DOFM_CircleDOF, DOFM_MAX };
struct FWeightedBlendables { TArray<struct FWeightedBlendable> Array; };
struct FWeightedBlendable { float Weight; UObject* Object; FWeightedBlendable(); FWeightedBlendable(float InWeight, UObject* InObject); };
struct FPostProcessSettings
{
	FPostProcessSettings();
#define TP_PP(T, N) uint8 bOverride_##N : 1; T N;
	TP_PP(float, WhiteTemp) TP_PP(float, WhiteTint)
	TP_PP(FVector4, ColorSaturation) TP_PP(FVector4, ColorContrast) TP_PP(FVector4, ColorGamma) TP_PP(FVector4, ColorGain) TP_PP(FVector4, ColorOffset)
	TP_PP(FVector4, ColorSaturationShadows) TP_PP(FVector4, ColorContrastShadows) TP_PP(FVector4, ColorGammaShadows) TP_PP(FVector4, ColorGainShadows) TP_PP(FVector4, ColorOffsetShadows)
	TP_PP(FVector4, ColorSaturationMidtones) TP_PP(FVector4, ColorContrastMidtones) TP_PP(FVector4, ColorGammaMidtones) TP_PP(FVector4, ColorGainMidtones) TP_PP(FVector4, ColorOffsetMidtones)
	TP_PP(FVector4, ColorSaturationHighlights) TP_PP(FVector4, ColorContrastHighlights) TP_PP(FVector4, ColorGammaHighlights) TP_PP(FVector4, ColorGainHighlights) TP_PP(FVector4, ColorOffsetHighlights)
	TP_PP(float, ColorCorrectionShadowsMax) TP_PP(float, ColorCorrectionHighlightsMin) TP_PP(float, BlueCorrection) TP_PP(float, ExpandGamut)
	TP_PP(float, FilmSlope) TP_PP(float, FilmToe) TP_PP(float, FilmShoulder) TP_PP(float, FilmBlackClip) TP_PP(float, FilmWhiteClip)
	TP_PP(FLinearColor, SceneColorTint) TP_PP(float, SceneFringeIntensity) TP_PP(float, ChromaticAberrationStartOffset)
	TP_PP(TEnumAsByte<EBloomMethod>, BloomMethod) TP_PP(float, BloomIntensity) TP_PP(float, BloomThreshold) TP_PP(float, BloomSizeScale)
	TP_PP(float, Bloom1Size) TP_PP(float, Bloom2Size) TP_PP(float, Bloom3Size) TP_PP(float, Bloom4Size) TP_PP(float, Bloom5Size) TP_PP(float, Bloom6Size)
	TP_PP(FLinearColor, Bloom1Tint) TP_PP(FLinearColor, Bloom2Tint) TP_PP(FLinearColor, Bloom3Tint) TP_PP(FLinearColor, Bloom4Tint) TP_PP(FLinearColor, Bloom5Tint) TP_PP(FLinearColor, Bloom6Tint)
	TP_PP(float, BloomDirtMaskIntensity) TP_PP(FLinearColor, BloomDirtMaskTint)
	TP_PP(FLinearColor, AmbientCubemapTint) TP_PP(float, AmbientCubemapIntensity)
	TP_PP(TEnumAsByte<EAutoExposureMethod>, AutoExposureMethod) TP_PP(float, AutoExposureLowPercent) TP_PP(float, AutoExposureHighPercent)
	TP_PP(float, AutoExposureMinBrightness) TP_PP(float, AutoExposureMaxBrightness) TP_PP(float, AutoExposureSpeedUp) TP_PP(float, AutoExposureSpeedDown)
	TP_PP(float, AutoExposureBias) TP_PP(float, HistogramLogMin) TP_PP(float, HistogramLogMax) TP_PP(uint32, AutoExposureApplyPhysicalCameraExposure)
	TP_PP(float, LensFlareIntensity) TP_PP(FLinearColor, LensFlareTint) TP_PP(float, LensFlareBokehSize) TP_PP(float, LensFlareThreshold)
	TP_PP(float, VignetteIntensity) TP_PP(float, GrainJitter) TP_PP(float, GrainIntensity)
	TP_PP(float, AmbientOcclusionIntensity) TP_PP(float, AmbientOcclusionStaticFraction) TP_PP(float, AmbientOcclusionRadius) TP_PP(uint32, AmbientOcclusionRadiusInWS)
	TP_PP(float, AmbientOcclusionFadeDistance) TP_PP(float, AmbientOcclusionFadeRadius) TP_PP(float, AmbientOcclusionPower) TP_PP(float, AmbientOcclusionBias)
	TP_PP(float, AmbientOcclusionQuality) TP_PP(float, AmbientOcclusionMipBlend) TP_PP(float, AmbientOcclusionMipScale) TP_PP(float, AmbientOcclusionMipThreshold)
	TP_PP(float, IndirectLightingIntensity) TP_PP(FLinearColor, IndirectLightingColor)
	TP_PP(float, ScreenSpaceReflectionIntensity) TP_PP(float, ScreenSpaceReflectionQuality) TP_PP(float, ScreenSpaceReflectionMaxRoughness)
	TP_PP(EReflectionsType, ReflectionsType)
	TP_PP(float, MotionBlurAmount) TP_PP(float, MotionBlurMax) TP_PP(float, MotionBlurTargetFPS) TP_PP(float, MotionBlurPerObjectSize)
	TP_PP(float, DepthOfFieldFocalDistance) TP_PP(float, DepthOfFieldFstop) TP_PP(float, DepthOfFieldMinFstop) TP_PP(float, DepthOfFieldSensorWidth)
	TP_PP(float, DepthOfFieldDepthBlurRadius) TP_PP(float, DepthOfFieldDepthBlurAmount) TP_PP(float, DepthOfFieldFocalRegion) TP_PP(float, DepthOfFieldNearTransitionRegion)
	TP_PP(float, DepthOfFieldFarTransitionRegion) TP_PP(float, DepthOfFieldScale) TP_PP(float, DepthOfFieldNearBlurSize) TP_PP(float, DepthOfFieldFarBlurSize)
	TP_PP(float, ScreenPercentage)
	TP_PP(UTexture*, ColorGradingLUT) TP_PP(float, ColorGradingIntensity) TP_PP(float, LPVIntensity)
#undef TP_PP
	FWeightedBlendables WeightedBlendables;
	void AddBlendable(TScriptInterface<class IBlendableInterface> InBlendableObject, float InWeight);
	void RemoveBlendable(TScriptInterface<class IBlendableInterface> InBlendableObject);
};
struct FMinimalViewInfo { FVector Location; FRotator Rotation; float FOV; float DesiredFOV; float OrthoWidth; float AspectRatio; uint32 bConstrainAspectRatio : 1; float PostProcessBlendWeight; FPostProcessSettings PostProcessSettings; };

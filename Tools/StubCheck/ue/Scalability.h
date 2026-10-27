#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
namespace Scalability
{
	struct FQualityLevels
	{
		float ResolutionQuality; int32 ViewDistanceQuality; int32 AntiAliasingQuality; int32 ShadowQuality; int32 PostProcessQuality; int32 TextureQuality; int32 EffectsQuality; int32 FoliageQuality; int32 ShadingQuality;
		FQualityLevels(); void SetFromSingleQualityLevel(int32 Value); int32 GetSingleQualityLevel() const; void SetBenchmarkFallback(); void SetDefaults();
	};
	void SetQualityLevels(const FQualityLevels& QualityLevels, bool bForce = false);
	FQualityLevels GetQualityLevels();
	FQualityLevels BenchmarkQualityLevels(uint32 WorkScale = 10, float CPUMultiplier = 1.0f, float GPUMultiplier = 1.0f);
}

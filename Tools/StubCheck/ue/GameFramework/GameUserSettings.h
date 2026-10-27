// Макет GameFramework/GameUserSettings.h (UE 4.27): настройки качества (Scalability) из кода.
#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "UObject/Object.h"
namespace EWindowMode { enum Type { Fullscreen, WindowedFullscreen, Windowed, NumWindowModes }; }
class UGameUserSettings : public UObject
{
public:
	typedef UObject Super; typedef UGameUserSettings ThisClass; static UClass* StaticClass();
	UGameUserSettings(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	static UGameUserSettings* GetGameUserSettings();
	virtual void ApplySettings(bool bCheckForCommandLineOverrides); virtual void ApplyNonResolutionSettings(); void ApplyResolutionSettings(bool bCheckForCommandLineOverrides);
	virtual void SaveSettings(); virtual void LoadSettings(bool bForceReload = false); virtual void ValidateSettings(); virtual void ResetToCurrentSettings(); virtual void SetToDefaults();
	void SetScreenResolution(FIntPoint Resolution); FIntPoint GetScreenResolution() const; FIntPoint GetDesktopResolution() const; FIntPoint GetDefaultResolution();
	void SetFullscreenMode(EWindowMode::Type InFullscreenMode); EWindowMode::Type GetFullscreenMode() const;
	void SetVSyncEnabled(bool bEnable); bool IsVSyncEnabled() const;
	void SetFrameRateLimit(float NewLimit); float GetFrameRateLimit() const;
	void SetOverallScalabilityLevel(int32 Value); int32 GetOverallScalabilityLevel() const;
	void SetResolutionScaleValueEx(float NewScaleValue); void SetResolutionScaleNormalized(float NewScaleNormalized); void GetResolutionScaleInformationEx(float& CurrentScaleNormalized, float& CurrentScaleValue, float& MinScaleValue, float& MaxScaleValue) const;
	void SetViewDistanceQuality(int32 Value); int32 GetViewDistanceQuality() const;
	void SetShadowQuality(int32 Value); int32 GetShadowQuality() const;
	void SetAntiAliasingQuality(int32 Value); int32 GetAntiAliasingQuality() const;
	void SetTextureQuality(int32 Value); int32 GetTextureQuality() const;
	void SetVisualEffectQuality(int32 Value); int32 GetVisualEffectQuality() const;
	void SetPostProcessingQuality(int32 Value); int32 GetPostProcessingQuality() const;
	void SetFoliageQuality(int32 Value); int32 GetFoliageQuality() const;
	void SetShadingQuality(int32 Value); int32 GetShadingQuality() const;
	void RunHardwareBenchmark(int32 WorkScale = 10, float CPUMultiplier = 1.0f, float GPUMultiplier = 1.0f);
	void ApplyHardwareBenchmarkResults();
	bool IsDirty() const;
};

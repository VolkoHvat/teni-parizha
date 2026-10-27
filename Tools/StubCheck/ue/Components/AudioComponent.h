#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
class USoundBase; class USoundAttenuation;
enum class EAudioFaderCurve : uint8 { Linear, Logarithmic, SCurve, Sin, Count };
class UAudioComponent : public USceneComponent
{
public:
	typedef USceneComponent Super; typedef UAudioComponent ThisClass; static UClass* StaticClass();
	UAudioComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	USoundBase* Sound; float VolumeMultiplier; float PitchMultiplier; uint8 bAllowSpatialization : 1; uint8 bOverrideAttenuation : 1; uint8 bIsUISound : 1; uint8 bAutoDestroy : 1; uint8 bStopWhenOwnerDestroyed : 1;
	USoundAttenuation* AttenuationSettings;
	void SetSound(USoundBase* NewSound);
	virtual void Play(float StartTime = 0.f); virtual void Stop();
	bool IsPlaying() const;
	virtual void FadeIn(float FadeInDuration, float FadeVolumeLevel = 1.f, float StartTime = 0.f, const EAudioFaderCurve FadeCurve = EAudioFaderCurve::Linear);
	virtual void FadeOut(float FadeOutDuration, float FadeVolumeLevel, const EAudioFaderCurve FadeCurve = EAudioFaderCurve::Linear);
	void AdjustVolume(float AdjustVolumeDuration, float AdjustVolumeLevel, const EAudioFaderCurve FadeCurve = EAudioFaderCurve::Linear);
	void SetVolumeMultiplier(float NewVolumeMultiplier); void SetPitchMultiplier(float NewPitchMultiplier);
	void SetFloatParameter(FName InName, float InFloat); void SetIntParameter(FName InName, int32 InInt); void SetBoolParameter(FName InName, bool InBool);
	void SetPaused(bool bPause);
};

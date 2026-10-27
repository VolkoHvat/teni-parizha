#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Camera/CameraTypes.h"
class APlayerController; class UCameraShakeBase;
enum class ECameraShakePlaySpace : uint8;
class APlayerCameraManager : public AActor
{
public:
	typedef AActor Super; typedef APlayerCameraManager ThisClass; static UClass* StaticClass();
	APlayerCameraManager(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	APlayerController* PCOwner; float DefaultFOV; float DefaultOrthoWidth; float DefaultAspectRatio; FName CameraStyle;
	float ViewPitchMin; float ViewPitchMax; float ViewYawMin; float ViewYawMax; float ViewRollMin; float ViewRollMax;
	FVector GetCameraLocation() const; FRotator GetCameraRotation() const; float GetFOVAngle() const;
	void GetCameraViewPoint(FVector& OutCamLoc, FRotator& OutCamRot) const;
	virtual void SetFOV(float NewFOV); void UnlockFOV();
	void StartCameraFade(float FromAlpha, float ToAlpha, float Duration, FLinearColor Color, bool bShouldFadeAudio = false, bool bHoldWhenFinished = false);
	void StopCameraFade(); void SetManualCameraFade(float InFadeAmount, FLinearColor Color, bool bInFadeAudio);
	UCameraShakeBase* StartCameraShake(TSubclassOf<UCameraShakeBase> ShakeClass, float Scale = 1.f, ECameraShakePlaySpace PlaySpace = (ECameraShakePlaySpace)0, FRotator UserPlaySpaceRot = FRotator::ZeroRotator);
	void StopAllCameraShakes(bool bImmediately = true);
	virtual void UpdateCamera(float DeltaTime);
	virtual void ProcessViewRotation(float DeltaTime, FRotator& OutViewRotation, FRotator& OutDeltaRot);
};

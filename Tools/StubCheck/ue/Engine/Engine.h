// Макет Engine/Engine.h (UE 4.27): UEngine, GEngine, шрифты по умолчанию, экранные сообщения.
#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Engine/EngineTypes.h"
class UFont; class UGameViewportClient; class UGameUserSettings; class APlayerController; class UWorld; class UTexture2D; class UMaterial;
enum class EGetWorldErrorMode { ReturnNull, LogAndReturnNull, Assert };
struct FWorldContext { UWorld* World() const; TEnumAsByte<EWorldType::Type> WorldType; class UGameInstance* OwningGameInstance; };
class UEngine : public UObject
{
public:
	typedef UObject Super; typedef UEngine ThisClass; static UClass* StaticClass();
	UEngine(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	static UFont* GetTinyFont(); static UFont* GetSmallFont(); static UFont* GetMediumFont(); static UFont* GetLargeFont(); static UFont* GetSubtitleFont();
	static UFont* GetAdditionalFont(int32 AdditionalFontIndex);
	UGameViewportClient* GameViewport;
	uint32 bSmoothFrameRate : 1; uint32 bUseFixedFrameRate : 1; float FixedFrameRate; uint32 bEnableOnScreenDebugMessages : 1;
	float NearClipPlane;
	void AddOnScreenDebugMessage(uint64 Key, float TimeToDisplay, FColor DisplayColor, const FString& DebugMessage, bool bNewerOnTop = true, const FVector2D& TextScale = FVector2D::UnitVector);
	void AddOnScreenDebugMessage(int32 Key, float TimeToDisplay, FColor DisplayColor, const FString& DebugMessage, bool bNewerOnTop = true, const FVector2D& TextScale = FVector2D::UnitVector);
	void ClearOnScreenDebugMessages(); void RemoveOnScreenDebugMessage(uint64 Key);
	UWorld* GetWorldFromContextObject(const UObject* Object, EGetWorldErrorMode ErrorMode) const;
	UWorld* GetWorldFromContextObjectChecked(const UObject* Object) const;
	const TArray<FWorldContext>& GetWorldContexts() const;
	APlayerController* GetFirstLocalPlayerController(const UWorld* InWorld);
	UGameUserSettings* GetGameUserSettings();
	virtual float GetMaxTickRate(float DeltaTime, bool bAllowFrameRateSmoothing = true) const;
	virtual bool Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar);
	TArray<FString> DeferredCommands;
	bool IsEditor();
};
extern UEngine* GEngine;
extern bool GIsEditor;
extern bool GIsPlayInEditorWorld;

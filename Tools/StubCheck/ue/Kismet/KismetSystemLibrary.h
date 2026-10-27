#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/EngineTypes.h"
class APlayerController;
namespace EQuitPreference { enum Type { Quit, Background }; }
class UKismetSystemLibrary : public UBlueprintFunctionLibrary
{
public:
	typedef UBlueprintFunctionLibrary Super; typedef UKismetSystemLibrary ThisClass; static UClass* StaticClass();
	static void PrintString(const UObject* WorldContextObject, const FString& InString = FString(TEXT("Hello")), bool bPrintToScreen = true, bool bPrintToLog = true, FLinearColor TextColor = FLinearColor(0.0f, 0.66f, 1.0f), float Duration = 2.f);
	static void ExecuteConsoleCommand(const UObject* WorldContextObject, const FString& Command, class APlayerController* SpecificPlayer = nullptr);
	static void QuitGame(const UObject* WorldContextObject, class APlayerController* SpecificPlayer, TEnumAsByte<EQuitPreference::Type> QuitPreference, bool bIgnorePlatformRestrictions);
	static float GetGameTimeInSeconds(const UObject* WorldContextObject);
	static FString GetDisplayName(const UObject* Object); static FString GetObjectName(const UObject* Object); static FString GetPathName(const UObject* Object);
	static bool IsValid(const UObject* Object); static bool IsValidClass(UClass* Class);
	static FString GetPlatformUserName(); static FString GetEngineVersion(); static FString GetGameName(); static FString GetProjectDirectory(); static FString GetProjectSavedDirectory();
	static bool LineTraceSingle(const UObject* WorldContextObject, const FVector Start, const FVector End, ETraceTypeQuery TraceChannel, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, FHitResult& OutHit, bool bIgnoreSelf, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);
	static bool SphereTraceSingle(const UObject* WorldContextObject, const FVector Start, const FVector End, float Radius, ETraceTypeQuery TraceChannel, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, FHitResult& OutHit, bool bIgnoreSelf, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);
	static bool CapsuleTraceSingle(const UObject* WorldContextObject, const FVector Start, const FVector End, float Radius, float HalfHeight, ETraceTypeQuery TraceChannel, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, FHitResult& OutHit, bool bIgnoreSelf, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);
	static bool SphereOverlapActors(const UObject* WorldContextObject, const FVector SpherePos, float SphereRadius, const TArray<TEnumAsByte<EObjectTypeQuery>>& ObjectTypes, UClass* ActorClassFilter, const TArray<AActor*>& ActorsToIgnore, TArray<class AActor*>& OutActors);
	static void SetSuppressViewportTransitionMessage(const UObject* WorldContextObject, bool bState);
	static int32 GetRenderingDetailMode(); static int32 GetRenderingMaterialQualityLevel();
	static void ControlScreensaver(bool bAllowScreenSaver);
};

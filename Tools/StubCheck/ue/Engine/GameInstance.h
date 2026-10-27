#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "UObject/Object.h"
class ULocalPlayer; class APlayerController; class UGameViewportClient; class UEngine; class FTimerManager; class UWorld;
class UGameInstance : public UObject
{
public:
	typedef UObject Super; typedef UGameInstance ThisClass; static UClass* StaticClass();
	UGameInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void Init(); virtual void Shutdown(); virtual void OnStart(); virtual void StartGameInstance();
	virtual UWorld* GetWorld() const override;
	UEngine* GetEngine() const; UGameViewportClient* GetGameViewportClient() const;
	template<class TSubsystemClass> TSubsystemClass* GetSubsystem() const { static_assert(sizeof(TSubsystemClass) > 0, "GetSubsystem<T>: неполный тип"); return nullptr; }
	template<class TSubsystemClass> static TSubsystemClass* GetSubsystem(const UGameInstance* GameInstance) { return nullptr; }
	APlayerController* GetFirstLocalPlayerController(const UWorld* World = nullptr) const;
	ULocalPlayer* GetFirstGamePlayer() const;
	const TArray<ULocalPlayer*>& GetLocalPlayers() const; int32 GetNumLocalPlayers() const; ULocalPlayer* GetLocalPlayerByIndex(const int32 Index) const;
	FTimerManager& GetTimerManager() const;
	virtual void ReturnToMainMenu();
};

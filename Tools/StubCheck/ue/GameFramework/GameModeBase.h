// Макет GameFramework/GameModeBase.h (UE 4.27)
#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
class APawn; class AHUD; class APlayerController; class AGameStateBase; class APlayerState; class ASpectatorPawn; class AGameSession; class AController;
class AGameModeBase : public AActor
{
public:
	typedef AActor Super; typedef AGameModeBase ThisClass; static UClass* StaticClass();
	AGameModeBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	FString OptionsString;
	TSubclassOf<AGameSession> GameSessionClass; TSubclassOf<AGameStateBase> GameStateClass; TSubclassOf<APlayerController> PlayerControllerClass;
	TSubclassOf<APlayerState> PlayerStateClass; TSubclassOf<AHUD> HUDClass; TSubclassOf<APawn> DefaultPawnClass; TSubclassOf<ASpectatorPawn> SpectatorClass;
	TSubclassOf<APlayerController> ReplaySpectatorPlayerControllerClass;
	AGameSession* GameSession; AGameStateBase* GameState; FText DefaultPlayerName;
	uint32 bUseSeamlessTravel : 1; uint32 bStartPlayersAsSpectators : 1; uint32 bPauseable : 1;
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage);
	virtual void InitGameState();
	virtual void StartPlay();
	virtual bool HasMatchStarted() const; virtual bool HasMatchEnded() const;
	virtual void PostLogin(APlayerController* NewPlayer);
	virtual void Logout(AController* Exiting);
	virtual void RestartPlayer(AController* NewPlayer);
	virtual void RestartPlayerAtPlayerStart(AController* NewPlayer, AActor* StartSpot);
	virtual void RestartPlayerAtTransform(AController* NewPlayer, const FTransform& SpawnTransform);
	virtual APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot);
	virtual APawn* SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform);
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player);
	virtual AActor* FindPlayerStart_Implementation(AController* Player, const FString& IncomingName);
	AActor* FindPlayerStart(AController* Player, const FString& IncomingName = TEXT(""));
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController);
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer);
	virtual bool ClearPause();
	virtual bool IsPaused() const;
	virtual int32 GetNumPlayers(); virtual int32 GetNumSpectators();
	virtual void ResetLevel();
	virtual void ReturnToMainMenuHost();
	virtual bool PlayerCanRestart_Implementation(APlayerController* Player);
	template<class T> T* GetGameState() const { return nullptr; }
};

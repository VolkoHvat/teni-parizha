#include "TPGameMode.h"
#include "TeniParizha.h"
#include "TPCharacter.h"
#include "TPHUD.h"
#include "TPCityGenerator.h"
#include "TPEnvironment.h"
#include "EngineUtils.h"

ATPGameMode::ATPGameMode()
{
	DefaultPawnClass = ATPCharacter::StaticClass();
	HUDClass = ATPHUD::StaticClass();
}

void ATPGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
}

void ATPGameMode::StartPlay()
{
	UWorld* W = GetWorld();
	// пустой уровень → создаём квартал и окружение, чтобы игра запускалась «из коробки»
	bool bCity = false, bEnv = false;
	for (TActorIterator<ATPCityGenerator> It(W); It; ++It) bCity = true;
	for (TActorIterator<ATPEnvironment> It(W); It; ++It) bEnv = true;
	FActorSpawnParameters P; P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (!bCity) { if (ATPCityGenerator* C = W->SpawnActor<ATPCityGenerator>(FVector::ZeroVector, FRotator::ZeroRotator, P)) C->Generate(); UE_LOG(LogTeni, Log, TEXT("Создан процедурный квартал")); }
	if (!bEnv) W->SpawnActor<ATPEnvironment>(FVector::ZeroVector, FRotator::ZeroRotator, P);
	Super::StartPlay();
}

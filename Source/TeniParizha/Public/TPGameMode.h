// Тени Парижа — режим игры: герой, интерфейс; если уровень пустой — сам создаёт квартал и небо.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TPGameMode.generated.h"

UCLASS()
class TENIPARIZHA_API ATPGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	ATPGameMode();
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void StartPlay() override;
};

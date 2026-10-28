// Тени Парижа — сохранение прогресса: глава, медали Лиги крыш, способности, альбом «Лица квартала».
// Владелец модуля: агент «жители, сюжет и интерфейс».
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "TPSaveGame.generated.h"

UCLASS()
class TENIPARIZHA_API UTPSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** имя слота сохранения */
	static FString SlotName() { return TEXT("TeniParizha_Story"); }
	static constexpr int32 CurrentVersion = 1;

	UPROPERTY() int32 Version = CurrentVersion;
	/** 0 — пролог, 1..4 — главы, 5 — после финала */
	UPROPERTY() int32 Chapter = 0;
	UPROPERTY() bool bEnding = false;
	/** открытые способности (битовая маска) */
	UPROPERTY() int32 Abilities = 0;
	/** медали по гонкам: 0 — нет, 1 — бронза, 2 — серебро, 3 — золото */
	UPROPERTY() TArray<int32> Medals;
	UPROPERTY() TArray<float> BestTimes;
	/** альбом: 0 — незнаком, 1 — знаком, 2 — портрет написан */
	UPROPERTY() TArray<uint8> Album;
	UPROPERTY() float TimeOfDay = 9.5f;
};
